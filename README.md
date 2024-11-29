# Spider2 - Project
![BuildStatus](https://github.com/Vitrix-Software-s-r-o/spider2/actions/workflows/build-run-tests.yml/badge.svg)

The aim of this project is to create library that to implement HTTP API servers in composable manner in modern C++ using Boost.Asio and Boost.Beast with full coroutines support.

We support following features:
 - easy routing for Boost.Beast
 - sendfile | TrasferFile endpoints with byte ranges
 - API parsing for HTTP query strings
 - serialization | deserialization using Boost.JSON
 - possibility to plug-in custom serialization | parsing library for api requests 

Check out the [documention](https://spider2.vitrix.cz/)

License: Boost Sofware License - Version 1
