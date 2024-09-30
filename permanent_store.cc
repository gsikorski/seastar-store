#include "permanent_store.h"
#include <nlohmann/json.hpp>
#include <seastar/core/file.hh>
#include <seastar/core/smp.hh>

using json = nlohmann::json;

namespace store {

namespace {

constexpr static auto PAGE_SIZE = 4096;
constexpr static auto MAX_SHARD_SIZE = PAGE_SIZE * std::kilo::num;

auto read_file(const std::filesystem::path filename,
               seastar::temporary_buffer<char> &rbuf) {
  return seastar::with_file(
      seastar::open_file_dma(std::string{filename}, seastar::open_flags::ro),
      [&rbuf](seastar::file &file) {
        return file.dma_read(0, rbuf.get_write(), PAGE_SIZE);
      });
}

auto load(const std::filesystem::path shard_file) -> seastar::future<json> {
  auto rbuf =
      seastar::temporary_buffer<char>::aligned(PAGE_SIZE, MAX_SHARD_SIZE);
  co_await read_file(shard_file, rbuf);
  co_return json::parse(rbuf);
}

auto write_file(const std::filesystem::path filename,
                seastar::temporary_buffer<char> &wbuf, size_t buffer_size) {
  return seastar::with_file(
      seastar::open_file_dma(std::string{filename},
                             seastar::open_flags::rw |
                                 seastar::open_flags::create),
      [&wbuf, buffer_size](seastar::file &file) {
        return file.dma_write(0, wbuf.get(), MAX_SHARD_SIZE);
      });
};

seastar::future<> store(const std::filesystem::path shard_file,
                        const json &content) {
  auto wbuf =
      seastar::temporary_buffer<char>::aligned(PAGE_SIZE, MAX_SHARD_SIZE);
  const auto json_str = content.dump();
  assert(json_str.size() < MAX_SHARD_SIZE);
  std::fill(wbuf.get_write(), wbuf.get_write() + MAX_SHARD_SIZE, 0);
  std::ranges::copy(json_str, wbuf.get_write());
  co_await write_file(shard_file, wbuf, json_str.size());
}

} // namespace

permanent_store::permanent_store() {
  std::filesystem::create_directory("storage");
}

seastar::future<> permanent_store::update(const unsigned int shard_id,
                                          const std::string &key,
                                          const std::string &value) {
  const auto fname = fmt::format("storage/{}.json", shard_id);
  auto loaded = json{};
  if (std::filesystem::exists(fname))
    loaded = co_await load(fname);
  loaded[key] = std::move(value);
  co_await store(fname, loaded);
}

seastar::future<> permanent_store::erase(const unsigned int shard_id,
                                         const std::string &key) {
  const auto fname = fmt::format("storage/{}.json", shard_id);
  if (not std::filesystem::exists(fname))
    co_return;
  auto loaded = co_await load(fname);
  loaded.erase(key);
  co_await store(fname, loaded);
}

seastar::future<std::optional<std::string>>
permanent_store::find_one(const unsigned int shard_id, const std::string &key) {
  const auto fname = fmt::format("storage/{}.json", shard_id);
  if (not std::filesystem::exists(fname))
    co_return std::optional<std::string>{};
  auto loaded = co_await load(fname);
  if (not loaded.contains(key))
    co_return std::optional<std::string>{};
  co_return loaded[key];
}

seastar::future<std::string> permanent_store::find_all() {
  auto result = json::object();
  co_await seastar::parallel_for_each(
      std::views::iota(0u, seastar::smp::count),
      [&result](const auto shard_id) -> seastar::future<> {
        auto partial = co_await seastar::smp::submit_to(
            shard_id, [shard_id] -> seastar::future<json> {
              const auto fname = fmt::format("storage/{}.json", shard_id);
              co_return std::filesystem::exists(fname) ? co_await load(fname)
                                                       : json::object();
            });
        for (auto it = partial.begin(); it != partial.end(); ++it)
          result[it.key()] = it.value();
        co_return;
      });
  co_return result.dump();
}

} // namespace store