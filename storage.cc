#include "storage.h"
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <ranges>

using json = nlohmann::json;

namespace store {

void storage::evict() {
  auto lru =
      std::ranges::max_element(cache, [](const auto &left, const auto &right) {
        const auto &[lkey, litem] = left;
        const auto &[rkey, ritem] = right;
        return litem.touch > ritem.touch;
      });
  cache.erase(lru);
}

seastar::future<> storage::update(const std::string &key, std::string &&value) {
  cache[key] = {.value = value, .touch = std::chrono::steady_clock::now()};
  if (cache.size() > cache_size)
    evict();
  return permanent.update(key, std::move(value));
}

seastar::future<std::optional<std::string>>
storage::get(const std::string &key) {
  const auto found = cache.find(key);
  if (found != cache.end()) {
    auto &[_key, item] = *found;
    item.touch = std::chrono::steady_clock::now();
    co_return json({key, item.value}).dump();
  }
  const auto opt_value = co_await permanent.find_one(key);
  if (opt_value) {
    cache[key] = cached_t{.value = *opt_value,
                          .touch = std::chrono::steady_clock::now()};
    if (cache.size() > cache_size)
      evict();
  }
  co_return std::optional<std::string>{json({key, *opt_value}).dump()};
}

seastar::future<std::string> storage::get_all() { return permanent.find_all(); }

seastar::future<> storage::remove(const std::string &key) {
  cache.erase(key);
  return permanent.erase(key);
}

} // namespace store