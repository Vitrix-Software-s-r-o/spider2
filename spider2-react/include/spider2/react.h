#pragma once

#include "spider2/handlers/static_files.h"

#include <boost/filesystem/fstream.hpp>
#include <format>
#include <spider2/proxy.h>
#include <spider2/types.h>

namespace spider2
{
   namespace react
   {
      using custom_script_generator_callback = std::function<std::string(request &)>;
      struct react_config
      {
         /// The endpoint of the react server
         tcp::endpoint endpoint = tcp::endpoint{io::ip::address_v4::loopback(), 3000};

         /// If true, the react app will be served from the react server
         bool debug_react = false;

         /// This generator is used to append custom scripts to the react index.html
         custom_script_generator_callback script_append_generator = [](request &) -> string
         { return "/* provide your own script_append_generator to load app context data */"; };

         /// The path to the react index.html file inside of the react app or the public folder in case of debug build
         boost::filesystem::path react_index_html_path;

         /// The public url of the react app in case of debug build - if not set, it will be generated from the request
         /// endpoint
         optional<std::string> public_url;
      };

      class react_app_template_state
      {

       public:
         explicit react_app_template_state(react_config cfg) : cfg_(std::move(cfg))
         {
            reload_template_if_changed();
         }

         [[nodiscard]] auto get_index_html(request &req) -> string
         {
            using namespace std::literals;
            constexpr auto custom_script_token = "{{{custom_script}}}"sv;

            auto template_copy = [this]()
            {
               std::lock_guard lock{mutex_};
               reload_template_if_changed();
               return template_;
            }();

            if (template_.empty())
            {
               return fmt::format(
                   "<!doctype html><h1>Configuration error</h1> <p>React app template not found at path {}.</p>",
                   cfg_.react_index_html_path.string());
            }

            const auto custom_script = cfg_.script_append_generator(req);
            boost::replace_first(template_copy, custom_script_token, custom_script);
            if (cfg_.debug_react)
            {
               boost::replace_first(
                   template_copy, "</head>",
                   fmt::format("<script defer src=\"http://{0}:{1}/static/js/bundle.js\"></script></head>",
                               cfg_.endpoint.address().to_string(), cfg_.endpoint.port()));

               if (cfg_.public_url.has_value())
               {
                  boost::replace_all(template_copy, "%PUBLIC_URL%", cfg_.public_url.value());
               }
               else
               {
                  const auto server_endpoint = req.get_request_server_tcp_endpoint();

                  boost::replace_all(
                      template_copy, "%PUBLIC_URL%",
                      fmt::format("http://{}:{}", server_endpoint.address().to_string(), server_endpoint.port()));
               }
            }

            return template_copy;
         }

       private:
         inline void reload_template_if_changed()
         {
            const auto last_change = last_change_;
            auto ec = error_code{};

            const auto last_write_time = boost::filesystem::last_write_time(cfg_.react_index_html_path, ec);
            if (!ec && last_change != last_write_time)
            {
               template_ = load_react_app_template();
               last_change_ = last_write_time;
            }
         }

         [[nodiscard]] inline auto load_react_app_template() const -> string
         {
            auto file = boost::filesystem::ifstream(cfg_.react_index_html_path);
            auto iterator = std::istreambuf_iterator<char>(file);
            return {iterator, std::istreambuf_iterator<char>()};
         }

       private:
         std::mutex mutex_;
         string template_ = {};
         time_t last_change_ = {};
         react_config cfg_;
      };

      inline bool should_proxy(request &req)
      {
         using namespace boost::algorithm;
         const auto ep = req.get_endpoint();
         return ep.path.ends_with(".hot-update.json") || ep.path.ends_with(".hot-update.js") ||
                ep.path.ends_with(".hot-update.js.map") || ep.path.starts_with("/static/");
      }

      inline auto enable_react(react_config const &cfg)
      {
         if (cfg.react_index_html_path.empty())
         {
            throw std::logic_error{"react_index_html_path must be set in react_config"};
         }

         return [_state = std::make_shared<react_app_template_state>(cfg),
                 _proxy_request = naive_proxy_pass({.target = {.endpoint = cfg.endpoint}}),
                 _static_files_handler = static_files(cfg.react_index_html_path.parent_path()),
                 debug_react = cfg.debug_react](request &_req) -> await_response
         {
            return [](bool debug_react, request &req, std::shared_ptr<react_app_template_state> state,
                      auto &proxy_request, auto &static_files_handler) -> await_response
            {
               if (debug_react && should_proxy(req))
               {
                  co_return (co_await proxy_request(req));
               }
               else if (const auto ep = req.get_endpoint(); ep.path == "/"sv || ep.path == "/index.html"sv)
               {
                  co_return response::return_string(http::status::ok, state->get_index_html(req));
               }
               else
               {
                  co_return (co_await static_files_handler(req));
               }
            }(debug_react, _req, _state, _proxy_request, _static_files_handler);
         };
      };

      /**
       * This function searches for the index_debug.html or index.html file in the parent directories of the executable
       * to facilitate the lookup of the react app index.html file in the public folder.
       *
       * @param exe_path the path to the executable to start the search from
       * @param relative_path from the executable to the index_debug.html or index.html file in public folder
       * @return
       */
      inline auto find_debug_react_index_html(boost::filesystem::path const &exe_path,
                                              string relative_path = "../web-ui/public/index.html")
          -> boost::filesystem::path
      {
         int max_depth = 10;

         while (--max_depth > 0)
         {
            relative_path = "../" + relative_path;

            auto lookup_path = exe_path / relative_path;
            if (boost::filesystem::exists(lookup_path))
            {
               return exe_path / relative_path;
            }
         }

         throw std::logic_error{fmt::format("unable to find {0} in parent directories", relative_path)};
      }
   } // namespace react

   using react_config = react::react_config;

} // namespace spider2