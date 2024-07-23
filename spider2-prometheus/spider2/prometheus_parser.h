#pragma once

#include <boost/optional/optional.hpp>
#include <string>
#include <string_view>
#include <vector>
namespace spider2::prometheus
{
    struct metrics_value
    {
        double value = 0.0;
        std::string name;
        boost::optional<std::string> labels;

        static std::vector<metrics_value> parse(std::string_view metrics);
    };
} // namespace spider2::prometheus
