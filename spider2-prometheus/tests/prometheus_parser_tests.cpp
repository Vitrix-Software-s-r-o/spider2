#include <spider2/prometheus_parser.h>

#include <catch.hpp>

TEST_CASE("prometheus parser", "[parser]")
{
    using namespace spider2::prometheus;
    SECTION("parse")
    {
        auto result = metrics_value::parse("test 1.0 ");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].name == "test");
        REQUIRE(result[0].value == 1.0);
    }

    SECTION("parse with labels")
    {
        auto result = metrics_value::parse("test{label=\"test\"} 1.0 ");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].name == "test");
        REQUIRE(result[0].value == 1.0);
        REQUIRE(result[0].labels.value_or("") == "label=\"test\"");
    }

    SECTION("parse with labels multiple values")
    {
        auto result = metrics_value::parse("test0{label=\"test\"} 1.0\ntest1{label=\"test\"} 2.0");
        REQUIRE(result.size() == 2);
        REQUIRE(result[0].name == "test0");
        REQUIRE(result[0].value == 1.0);
        REQUIRE(result[0].labels.value_or("") == "label=\"test\"");

        REQUIRE(result[1].name == "test1");
        REQUIRE(result[1].value == 2.0);
        REQUIRE(result[1].labels.value_or("") == "label=\"test\"");
    }

    SECTION("parse with comments")
    {
        auto result = metrics_value::parse("# test 1.0\n");
        REQUIRE(result.size() == 0);
    }

    SECTION("parse with comments and metrics values")
    {
        auto result = metrics_value::parse("# This is a comment\n"
                                           "test0{label=\"test\"} 1.0\n"
                                           "# Another comment\n"
                                           "test1{label=\"test\"} 2.0\n"
                                           "# Final comment");
        REQUIRE(result.size() == 2);
        REQUIRE(result[0].name == "test0");
        REQUIRE(result[0].value == 1.0);
        REQUIRE(result[0].labels.value_or("") == "label=\"test\"");

        REQUIRE(result[1].name == "test1");
        REQUIRE(result[1].value == 2.0);
        REQUIRE(result[1].labels.value_or("") == "label=\"test\"");
    }
}
