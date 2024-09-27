#pragma once

#include "storage.h"
#include <memory>
#include <seastar/core/reactor.hh>
#include <seastar/http/httpd.hh>

namespace store {

class get_handler : public seastar::httpd::handler_base {
  storage &warehouse;

public:
  get_handler(storage &wh) : warehouse{wh} {};
  virtual seastar::future<std::unique_ptr<seastar::http::reply>>
  handle(const seastar::sstring &path,
         std::unique_ptr<seastar::http::request> req,
         std::unique_ptr<seastar::http::reply> rep);
};

class put_handler : public seastar::httpd::handler_base {
  storage &warehouse;

public:
  put_handler(storage &wh) : warehouse{wh} {};
  virtual seastar::future<std::unique_ptr<seastar::http::reply>>
  handle(const seastar::sstring &path,
         std::unique_ptr<seastar::http::request> req,
         std::unique_ptr<seastar::http::reply> rep);
};

class delete_handler : public seastar::httpd::handler_base {
  storage &warehouse;

public:
  delete_handler(storage &wh) : warehouse{wh} {};
  virtual seastar::future<std::unique_ptr<seastar::http::reply>>
  handle(const seastar::sstring &path,
         std::unique_ptr<seastar::http::request> req,
         std::unique_ptr<seastar::http::reply> rep);
};

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

struct stop_signal {
  bool _caught = false;
  seastar::condition_variable _cond;
  void signaled() {
    if (_caught)
      return;
    _caught = true;
    _cond.broadcast();
  }
  stop_signal() {
    seastar::engine().handle_signal(SIGINT, [this] { signaled(); });
    seastar::engine().handle_signal(SIGTERM, [this] { signaled(); });
  }
  ~stop_signal() {
    // There's no way to unregister a handler yet, so register a no-op handler
    // instead.
    seastar::engine().handle_signal(SIGINT, [] {});
    seastar::engine().handle_signal(SIGTERM, [] {});
  }
  seastar::future<> wait() {
    return _cond.wait([this] { return _caught; });
  }
  bool stopping() const { return _caught; }
};

} // namespace store