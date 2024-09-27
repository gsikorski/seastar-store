#include "handlers.h"

namespace store {

seastar::future<std::unique_ptr<seastar::http::reply>>
get_handler::handle(const seastar::sstring &path,
                    std::unique_ptr<seastar::http::request> req,
                    std::unique_ptr<seastar::http::reply> rep) {
  const auto key = req->get_query_param("key");
  rep->done("json");
  if (key.empty()) {
    rep->_content = co_await warehouse.get_all();
    co_return std::move(rep);
  }
  auto result = co_await warehouse.get(key);
  if (not result)
    rep->set_status(seastar::httpd::reply::status_type::not_found);
  else
    rep->_content = *result;
  co_return std::move(rep);
}

seastar::future<std::unique_ptr<seastar::http::reply>>
put_handler::handle(const seastar::sstring &path,
                    std::unique_ptr<seastar::http::request> req,
                    std::unique_ptr<seastar::http::reply> rep) {
  const auto key = req->get_query_param("key");
  auto value = req->get_query_param("value");
  rep->done("json");
  if (key.empty()) {
    rep->set_status(seastar::httpd::reply::status_type::bad_request);
    co_return std::move(rep);
  }
  co_await warehouse.update(key, std::move(value));
  co_return std::move(rep);
}

seastar::future<std::unique_ptr<seastar::http::reply>>
delete_handler::handle(const seastar::sstring &path,
                       std::unique_ptr<seastar::http::request> req,
                       std::unique_ptr<seastar::http::reply> rep) {
  const auto key = req->get_query_param("key");
  rep->done("json");
  if (key.empty()) {
    rep->set_status(seastar::httpd::reply::status_type::bad_request);
    co_return std::move(rep);
  }
  co_await warehouse.remove(key);
  co_return std::move(rep);
}

} // namespace store