#include "storage.h"
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <ranges>
#include <seastar/core/smp.hh>

using json = nlohmann::json;

namespace {
void evict(auto &cache) {
  auto lru =
      std::ranges::max_element(cache, [](const auto &left, const auto &right) {
        const auto &[lkey, litem] = left;
        const auto &[rkey, ritem] = right;
        return litem.touch > ritem.touch;
      });
  cache.erase(lru);
}
} // namespace

namespace store {

storage::storage(unsigned int size) : cache_size{size} {
  cache.resize(seastar::smp::count);
}

seastar::future<> storage::update(const std::string &key, std::string &&value) {
  auto shard_id = std::hash<std::string>{}(key) % seastar::smp::count;
  return seastar::smp::submit_to(
      shard_id,
      [this, shard_id, &key,
       value = std::move(value)] mutable -> seastar::future<> {
        auto &shard_cache = cache[shard_id];
        shard_cache[key] = {.value = value,
                            .touch = std::chrono::steady_clock::now()};
        if (shard_cache.size() > cache_size)
          evict(shard_cache);
        return permanent.update(shard_id, key, std::move(value));
      });
}

seastar::future<std::optional<std::string>>
storage::get(const std::string &key) {
  auto shard_id = std::hash<std::string>{}(key) % seastar::smp::count;
  return seastar::smp::submit_to(
      shard_id,
      [this, shard_id, &key] -> seastar::future<std::optional<std::string>> {
        auto &shard_cache = cache[shard_id];
        const auto found = shard_cache.find(key);
        if (found != shard_cache.end()) {
          auto &[_key, item] = *found;
          item.touch = std::chrono::steady_clock::now();
          co_return json({key, item.value}).dump();
        }
        const auto opt_value = co_await permanent.find_one(shard_id, key);
        if (!opt_value)
          co_return std::optional<std::string>{};
        shard_cache[key] = cached_t{.value = *opt_value,
                                    .touch = std::chrono::steady_clock::now()};
        if (shard_cache.size() > cache_size)
          evict(shard_cache);
        co_return std::optional<std::string>{json({key, *opt_value}).dump()};
      });
}

seastar::future<std::string> storage::get_all() { return permanent.find_all(); }

seastar::future<> storage::remove(const std::string &key) {
  auto shard_id = std::hash<std::string>{}(key) % seastar::smp::count;
  return seastar::smp::submit_to(shard_id,
                                 [this, shard_id, &key] -> seastar::future<> {
                                   auto &shard_cache = cache[shard_id];
                                   shard_cache.erase(key);
                                   return permanent.erase(shard_id, key);
                                 });
}

} // namespace store