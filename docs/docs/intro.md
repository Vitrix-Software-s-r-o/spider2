---
sidebar_position: 1
---

# Tutorial Intro

Let's discover **Spider2 in less than 5 minutes**. 
Spider2 is composable HTTP routing framework. It is designed to be simple and easy to use.

You can use Spider2 to create a simple web server, or to create a complex web application.

All examples are part of the source directory of the Spider2 project. You can find them in the `spider2-api/examples/tutorial` directory.

# Introduction Example
Here we create introductory example that demonstrates how to create a simple web server with three endpoints and how they are composed together.
```cpp title="intro.cpp - 1st let's create simple app"
auto make_simple_app()
{
   return begin_app() +
          on_get("/query/",
                 [](request const &r)
                 {
                    // this endpoint will return the query string as part of its response

                    auto query = r.get_query_string();
                    return response::return_string(
                        status::ok, std::format(R"(<doctype html><html><body><h1>Query: {}</h1></body></html>)", query),
                        "text/html");
                 }) +
          on_post("/body/",
                  [](request &r) -> await_response
                  {
                     // this endpoint will return the body of the request as part of its response

                     // read the body of the request
                     const auto ec = co_await r.read_body<http::string_body>();
                     if (auto *message = r.try_get_message<http::string_body>(); !ec && message != nullptr)
                     {
                        // get the body of the message (Beast message with string_body)
                        auto &message_body = message->body();

                        // return the response object
                        co_return response::return_string(
                            status::ok,
                            std::format(R"(<doctype html><html><body><h1>Body: {}</h1></body></html>)", message_body),
                            "text/plain");
                     }
                     else
                     {
                        // return a bad request response
                        co_return response::return_string(status::bad_request, "Failed to read body", "text/plain");
                     }
                  });
}

```

Key concepts:
- `begin_app()` - creates a new app and allows you to chain endpoints to it by operator `+`.
- `on_get()` - creates a new endpoint that will be triggered when the request method is GET and the path matches the provided path.
- `on_post()` - creates a new endpoint that will be triggered when the request method is POST and the path matches the provided path.
- `on_<known verb>` - creates a new endpoint that will be triggered when the request method is the provided verb and the path matches the provided path.
- `request` - represents the incoming request it contains message, preparsed query string and requested endpoint (path + method). HTTP message is a Beast message with a string body.
- `response` - represents the outgoing response it contains status, body and content type.
- `await_response` - is a type that allows you to return a response from an endpoint asynchronously. It is a coroutine type that allows you to use co_await to wait for async operations. 
Please read a [coroutines](https://en.cppreference.com/w/cpp/language/coroutines) documentation to understand how it works. And also how to capture variables in the coroutine lambda to be saved on coroutine frame [ms devblog](https://devblogs.microsoft.com/oldnewthing/20211103-00/?p=105870).
boost::asio::awaitable is a type that suspends always, variables must be captured in the lambda that is not a coroutine to be saved on the coroutine frame.


```cpp title="intro.cpp - 2st let's compose it together with the other endpoint"
   auto complete_app =
       begin_app() +
       on_get("/", [](request &r) { return response::return_string(status::ok, "Hello, World!", "text/plain"); }) +
       on_scope("/api", make_simple_app());
```
Key concepts:
- `on_scope()` - creates a new scope that will be triggered when the path starts by provided string. It allows you to compose multiple endpoints together.

```cpp title="intro.cpp - 3st let's run the server with the composed app"
   // namespace spider2::io a namespace alias for boost::asio
   auto io_loop = io::io_context{};
   auto stop_src = std::stop_source{};
   auto app = app_context_base{.token = stop_src.get_token()};

   std::cout << std::format("HTTP listening on {}:{}", bind_ip.to_string(), bind_port) << std::endl;
   run_web_app(
       io_loop,
       {
           .tcp_listen = {tcp::endpoint{bind_ip, bind_port}},
           .thread_pool_threads = 0,
           .io_threads = 1,
           .connection_keep_alive = std::chrono::seconds{30},
       },
       app,
       // we wrap the app in the processing pipeline where we add the cors middleware to allow cross origin requests
       (begin_middleware() | cors_middleware())(complete_app));

   // the app will run until the stop source is requested to stop (in this case that is forever)
```
Key concepts:
- `io::io_context` - is a class that provides the core I/O functionality for the Spider2. It is the boost::asio::io_context.
- `std::stop_source` - is a class that provides a way to stop the server.
- `app_context_base` - is a structure that holds the stop token. You can extend it with your own data.
- `run_web_app()` - is a function that starts the server. It takes the io_context, the server configuration, the app, and the processing pipeline as arguments.
- `begin_middleware()` - it starts the middleware processing pipeline, it allows you to chain middlewares together by operator `|`.
- `cors_middleware()` - is a middleware that allows cross-origin requests - here is just an example of any middleware.
