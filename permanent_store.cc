#include "permanent_store.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <seastar/core/smp.hh>

using json = nlohmann::json;

namespace store {

namespace {

auto load(const std::filesystem::path shard_file) {
  std::ifstream file(shard_file);
  return json::parse(file);
}

void store(const std::filesystem::path shard_file, const json &content) {
  std::ofstream file(shard_file);
  file << content;
}

} // namespace

permanent_store::permanent_store() {
  std::filesystem::create_directory("storage");
}

seastar::future<> permanent_store::update(const std::string &key,
                                          const std::string &value) {
  auto shard_id = std::hash<std::string>{}(key) % seastar::smp::count;
  return seastar::smp::submit_to(shard_id, [shard_id, key, value] {
    const auto fname = fmt::format("storage/{}.json", shard_id);
    auto loaded = std::filesystem::exists(fname) ? load(fname) : json{};
    loaded[key] = std::move(value);
    store(fname, loaded);
    return;
  });
}

seastar::future<> permanent_store::erase(const std::string &key) {
  auto shard_id = std::hash<std::string>{}(key) % seastar::smp::count;
  return seastar::smp::submit_to(shard_id, [shard_id, key] {
    const auto fname = fmt::format("storage/{}.json", shard_id);
    if (not std::filesystem::exists(fname))
      return;
    auto loaded = load(fname);
    loaded.erase(key);
    store(fname, loaded);
    return;
  });
}

seastar::future<std::optional<std::string>>
permanent_store::find_one(const std::string &key) {
  auto shard_id = std::hash<std::string>{}(key) % seastar::smp::count;
  return seastar::smp::submit_to(
      shard_id, [shard_id, key] -> seastar::future<std::optional<std::string>> {
        const auto fname = fmt::format("storage/{}.json", shard_id);
        if (not std::filesystem::exists(fname))
          co_return std::optional<std::string>{};
        auto loaded = load(fname);
        if (not loaded.contains(key))
          co_return std::optional<std::string>{};
        co_return loaded[key];
      });
}

seastar::future<std::string> permanent_store::find_all() {
  auto result = json::object();
  co_await seastar::parallel_for_each(
      std::views::iota(0u, seastar::smp::count),
      [&result](const auto shard_id) -> seastar::future<> {
        auto partial = co_await seastar::smp::submit_to(
            shard_id, [shard_id] -> seastar::future<json> {
              const auto fname = fmt::format("storage/{}.json", shard_id);
              co_return std::filesystem::exists(fname) ? load(fname)
                                                       : json::object();
            });
        for (auto it = partial.begin(); it != partial.end(); ++it)
          result[it.key()] = it.value();
        co_return;
      });
  co_return result.dump();
}

} // namespace store