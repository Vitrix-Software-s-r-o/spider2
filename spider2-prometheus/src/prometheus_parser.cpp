#include "spider2/prometheus_parser.h"


#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/std_tuple.hpp>

#include <boost/spirit/home/x3.hpp>
#include <string>
#include <vector>


BOOST_FUSION_ADAPT_STRUCT(spider2::prometheus::metrics_value, name, labels, value);

using spider2::prometheus::metrics_value;
namespace x3 = boost::spirit::x3;

const auto name_parser = x3::lexeme[+(x3::char_ - '{' - ' ')];
const auto label_parser = x3::lexeme['{' >> *(x3::char_ - '}') >> '}'];
const auto value_parser = x3::double_;
const auto metric_parser = name_parser >> -label_parser >> value_parser;

const auto comment = x3::lexeme['#' >> *(x3::char_ - '\n') >> -x3::lit('\n')];
const auto skipper = x3::space | comment;

std::vector<metrics_value> metrics_value::parse(std::string_view metrics)
{
    std::vector<metrics_value> result;
    auto iter = metrics.begin();
    auto end = metrics.end();

    bool r = x3::phrase_parse(iter, end, *metric_parser, skipper, result);

    if (r && iter == end)
    {
        return result;
    }
    else
    {
        // handle parse error
        return {};
    }
}
