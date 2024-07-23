//
// Created by jhrub on 16.02.2023.
//
#include <catch.hpp>
#include <spider2/types.h>

using namespace spider2;

TEST_CASE("test mime_table", "[mime]") {
    SECTION("no user table provided")
    {
        auto table = mime_table{};
        REQUIRE(table.lookup(".exe") == "application/binary");
        REQUIRE(table.lookup(".html") == "text/html");
        REQUIRE(table.lookup(".unknown", "my/fallback") == "my/fallback");
    }


    SECTION("user table provided")
    {
        auto table = mime_table{string_map{{".exe", "application/windows"}}};
        REQUIRE(table.lookup(".exe") == "application/windows");
        REQUIRE(table.lookup(".html") == "text/html");
        REQUIRE(table.lookup(".unknown", "my/fallback") == "my/fallback");
    }
}