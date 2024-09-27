#pragma once

#include <seastar/core/reactor.hh>
#include <string>
#include <unordered_map>

namespace store {

class permanent_store {
public:
  permanent_store();
  seastar::future<> update(const std::string &key, const std::string &value);
  seastar::future<> erase(const std::string &key);
  seastar::future<std::optional<std::string>> find_one(const std::string &key);
  seastar::future<std::string> find_all();
};

} // namespace store