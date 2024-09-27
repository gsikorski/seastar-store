#pragma once

#include "permanent_store.h"
#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

namespace store {

class storage {
  struct cached_t {
    std::string value;
    std::chrono::steady_clock::time_point touch;
  };
  std::unordered_map<std::string, cached_t> cache;
  unsigned int cache_size;
  void evict();
  permanent_store permanent;

public:
  void set_cache_size(unsigned int size) { cache_size = size; }
  seastar::future<> update(const std::string &key, std::string &&value);
  seastar::future<std::optional<std::string>> get(const std::string &key);
  seastar::future<std::string> get_all();
  seastar::future<> remove(const std::string &key);
};

} // namespace store