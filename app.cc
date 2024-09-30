#include "handlers.h"
#include "storage.h"
#include <seastar/core/thread.hh>

using namespace std::chrono_literals;
using namespace seastar;
namespace bpo = boost::program_options;

int main(int argc, char **argv) {
  app_template app;
  app.add_options()("cache-size", bpo::value<unsigned int>()->default_value(32),
                    "Cache size");

  return app.run(argc, argv, [&] {
    return async([&] {
      httpd::http_server_control server;
      server.start("storage").get();
      auto &&config = app.configuration();
      condition_variable exit_signal;
      auto warehouse = store::storage{config["cache-size"].as<unsigned int>()};
      auto getter = store::get_handler{warehouse};
      auto putter = store::put_handler{warehouse};
      auto deleter = store::delete_handler{warehouse};
      auto exiter = store::stop_signal{};
      server
          .set_routes(
              [&exit_signal, &getter, &putter, &deleter](httpd::routes &r) {
                r.add(httpd::operation_type::GET,
                      httpd::url("/values").remainder("key"), &getter);
                r.add(httpd::operation_type::PUT,
                      httpd::url("/values").remainder("key").remainder("value"),
                      &putter);
                r.add(httpd::operation_type::DELETE,
                      httpd::url("/values").remainder("key"), &deleter);
              })
          .get();
      server.listen(uint16_t(8080))
          .handle_exception([](auto ep) {
            std::cerr << format(
                "Could not start DB server on localhost:8080: {}\n", ep);
            return make_exception_future<>(ep);
          })
          .get();
      std::cout << "Listening on port 8080...\n";
      exiter.wait().get();
      return 0;
    });
  });
}
