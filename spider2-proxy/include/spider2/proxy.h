#pragma once

#include <format>
#include <spider2/types.h>

namespace spider2 {

class request_transformer {
public:
  virtual ~request_transformer() = default;
  virtual void transform_request(request& r, endpoint_base & ep, http::fields & headers) const = 0;
  virtual void transform_response(request& r, http::fields & headers) const = 0;
};


struct proxy_target {
  tcp::endpoint endpoint;
  request_transformer * transformer = nullptr;
};

struct proxy_config {
  proxy_target target;
  std::chrono::steady_clock::duration timeout = std::chrono::seconds{30};
};

/**
* @brief A naive proxy pass implementation
* It connects to the target server and forwards the request to it for every
* incoming request.
* @param cfg The proxy configuration
*/
inline auto naive_proxy_pass(proxy_config cfg) {
  return [=](request &_req) -> await_response {
    return [](request &req, proxy_config const &cfg) -> await_response {
      if (req.has_request_body()) {
        const auto ec = co_await req.read_body<http::string_body>();
        if (ec) {
          co_return response::return_string(http::status::bad_request,
                                            "Error reading request body");
        }
      }

      auto proxy_pass_message = [](request& r, auto &message,
                                   proxy_config const &cfg) -> await_response {

        if (cfg.target.transformer != nullptr){
           auto endpoint = r.get_endpoint();
            cfg.target.transformer->transform_request(r, endpoint, message);
            message.target(static_cast<std::string>(endpoint.path));
            message.method(endpoint.verb);
        }


        auto executor = co_await io::this_coro::executor;
        auto socket = tcp::socket{executor};
        auto deadline_timer = cancel_after_timeout(
             executor, [&] {
               auto ec = error_code{};
               socket.shutdown(io::socket_base::shutdown_both, ec);
               socket.close();
             }, cfg.timeout);

        const auto [ec] = co_await socket.async_connect(cfg.target.endpoint,
                                                        use_tuple_awaitable);
        if (!ec) {
          const auto [message_write_ec, written_bytes] =
              co_await http::async_write(socket, message, use_tuple_awaitable);
          if (!message_write_ec) {
            auto buffer = boost::beast::flat_buffer{};
            auto response_message = http::response<http::string_body>{};


            const auto [response_read_ec, read_bytes] =
                co_await http::async_read(socket, buffer, response_message,
                                          use_tuple_awaitable);
            if (!response_read_ec) {
               if (cfg.target.transformer != nullptr){
                         cfg.target.transformer->transform_response(r, response_message);
                }

              co_return response{std::move(response_message)};
            }
          }
        }
      };

      if (auto *message = req.try_get_message<http::string_body>();
          message != nullptr) {
        co_return (co_await proxy_pass_message(req, *message, cfg));
      } else if (auto *messege_with_empty_body = req.try_get_message<http::empty_body>();
                 messege_with_empty_body != nullptr) {
        co_return (co_await proxy_pass_message(req, *messege_with_empty_body, cfg));
      } else {
        co_return response::return_string(
            http::status::bad_request,
            "naive_proxy_pass: unsupported body type");
      }

      co_return response::return_string(http::status::bad_gateway,
                                        "naive_proxy_pass: Bad gateway");
    }(_req, cfg);
  };
};
} // namespace spider2