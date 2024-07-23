#include <spider2/api/from_body/from_body.h>
#include <spider2/api/op_result/op_result.h>
#include <spider2/middleware/cors_middleware.h>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include <spider2/api.h>
#include <spider2/spider2.h>

#ifdef SPIDER2_API_USE_VITRIX_COMMONS
#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP
#endif

using namespace boost::program_options;
using namespace spider2;

struct some_record
{
   std::int64_t start_time;
   std::int64_t elapsed_time;
   std::string name;

   friend void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, some_record const &rec)
   {
      auto &obj = jv.emplace_object();
      obj["start_time"] = rec.start_time;
      obj["elapsed_time"] = rec.elapsed_time;
      obj["name"] = rec.name;
   }

   friend auto tag_invoke(boost::json::value_to_tag<some_record>, boost::json::value const &jv) -> some_record
   {
      auto &obj = jv.as_object();
      return some_record{obj.at("start_time").as_int64(), obj.at("elapsed_time").as_int64(),
                         obj.at("name").as_string().c_str()};
   }
};

class record_store
{
 public:
   auto store_record(std::string const &scope, some_record const &rec) -> api::result<api::op_result<>>
   {
      auto lk = std::lock_guard{mtx};
      auto &scope_records = records[scope];
      if (scope_records.contains(rec.name))
      {
         return api::op_result<>::make_validation_failed_result(
             {api::op_field_msg{api::op_msg{"Record with this name ({0}) already exists.", {rec.name}}, "name"}},
             {"Validation failed"});
      }

      if (scope_records.size() == 10)
      {
         return api::op_result<>::make_validation_failed_result({}, {"Storage is full. Please delete some records."});
      }

      scope_records[rec.name] = rec;

      return api::op_result<>::make_success_result({}, {"Operation successful"});
   }

   auto get_records(std::string const &scope) -> api::result<std::map<std::string, some_record>>
   {
      auto lk = std::lock_guard{mtx};
      if (auto it = records.find(scope); it != records.end())
      {
         return api::result{it->second};
      }
      return api::result<std::map<std::string, some_record>>{{}};
   }

   auto delete_record(std::string const &scope, std::string const &name) -> api::result<api::op_result<>>
   {
      auto lk = std::lock_guard{mtx};
      auto &scope_records = records[scope];
      if (auto it = scope_records.find(name); it != scope_records.end())
      {
         scope_records.erase(it);
         return api::op_result<>::make_success_result({}, {"Operation successful"});
      }
      return api::op_result<>::make_entity_not_found_result({}, {"Record not found"});
   }

   auto get_scopes() -> api::result<std::vector<std::string>>
   {
      auto lk = std::lock_guard{mtx};
      auto result = std::vector<std::string>{};
      result.reserve(records.size());
      for (auto const &[scope, _] : records)
      {
         result.push_back(scope);
      }
      return api::result{result};
   }

 private:
   std::unordered_map<std::string, std::map<std::string, some_record>> records;
   std::mutex mtx;
};

struct scope_id
{
   std::string name;
};

auto make_dynamic_scope_test_api(record_store &store)
{
   auto &&scoped_api =
       begin_app() + on_api_get("", [&](scope_id const &scope) { return store.get_records(scope.name); }) +
       on_api_post("", [&](scope_id const &scope, const api::from_body<some_record> &rec)
                   { return store.store_record(scope.name, rec.value); }) +
       on_api_delete("",
                     [&](scope_id const &scope, const api::from_query<api::query_args> &args) {
                        return store.delete_record(scope.name, args.value.as_value<string>("name").value_or(string{}));
                     });

   auto &&get_scopes_api = begin_app() + on_api_get("",
                                                    [&]()
                                                    {
                                                       // this gets scopes
                                                       return store.get_scopes();
                                                    });

   return begin_app() +
          on_scope("/",
                   [get_scopes_api = std::forward<decltype(get_scopes_api)>(get_scopes_api),
                    scoped_api = std::forward<decltype(scoped_api)>(scoped_api)](request &r) -> await_response
                   {
                      using scoped_api_t = decltype(scoped_api);
                      if (r.get_processing_endpoint().path.empty())
                      {
                         return get_scopes_api(r);
                      }
                      else
                      {
                         return [](request &r, scoped_api_t const &scoped_api) -> await_response
                         {
                            const auto scope = scope_id{.name = static_cast<string>(r.get_processing_endpoint().path)};
                            auto offset_handle = r.push_path_processing_offset(scope.name.size());
                            co_return (co_await scoped_api(r, scope));
                         }(r, scoped_api);
                      }
                   });
}

auto get_resources_directory(bool debug) -> boost::filesystem::path
{
   if (debug)
      return boost::filesystem::path{__FILE__}.parent_path();
   else
      return boost::dll::program_location().parent_path() / "resources";
}

int main(int argc, char **argv)
{

   auto desc = options_description("simple_api2");
   desc.add_options()("help", "produce help message")("port", value<std::uint16_t>()->default_value(3001), "port")(
       "host", value<std::string>()->default_value("127.0.0.1"), "bind host")("debug",
                                                                              "loads resources from source location");

   auto parsed = parse_command_line(argc, argv, desc);

   auto vm = variables_map{};
   store(parse_command_line(argc, argv, desc), vm);
   notify(vm);

   if (vm.count("help"))
   {
      std::cout << desc << std::endl;
      return 1;
   }

   auto io_loop = io::io_context{};

   auto stop_src = std::stop_source{};

   auto app = app_context_base{.token = stop_src.get_token()};

   auto state = record_store{};
   auto simple_app =
       begin_app() + on_scope("/api", make_dynamic_scope_test_api(state)) +
       on_scope("/", static_files(get_resources_directory(vm.count("debug") > 0), {.index_files = {"index.html"s}}));

   auto bind_ip = io::ip::address::from_string(vm.at("host").as<string>());
   auto bind_port = vm.at("port").as<std::uint16_t>();

   std::cout << std::format("HTTP listening on {}:{}", bind_ip.to_string(), bind_port) << std::endl;
   run_web_app(io_loop,
               {.tcp_listen = {tcp::endpoint{bind_ip, bind_port}}, .connection_keep_alive = std::chrono::seconds{30}},
               app, (begin_middleware() | cors_middleware())(simple_app));
}