//
// Created by jhrub on 20.02.2023.
//
#include <spider2/prometheus.h>
#include <spider2/routing/router.h>

#include <catch.hpp>

TEST_CASE("prometheus class", "[prometheus]")
{
   namespace prom = spider2::prometheus;
   auto manager = prom::metrics_manager{};

   SECTION("counter")
   {
      auto counter = manager.create<prom::counter>({.name = "counter", .help = "counter help"});
      counter.increment();
      counter.increment();
      counter.increment();
      counter.increment(3);

      auto result = manager.format_metrics();
      REQUIRE(result == "# HELP counter counter help\n# TYPE counter counter\ncounter 6\n");
   }

   SECTION("gauge")
   {
      auto counter = manager.create<prom::gauge>({.name = "gauge", .help = "gauge help"});
      counter.increment();
      counter.increment();
      counter.increment();
      counter.decrement(3);
      counter.increment(1);

      auto result = manager.format_metrics();
      REQUIRE(result == "# HELP gauge gauge help\n# TYPE gauge gauge\ngauge 1\n");
   }

   SECTION("counter with labels")
   {
      auto counter =
         manager.create<prom::counter>({.name = "counter", .help = "counter help", .labels = "label=\"test\""});
      counter.increment();
      counter.increment();
      counter.increment();
      counter.increment(3);

      auto result = manager.format_metrics();
      REQUIRE(result == "# HELP counter counter help\n# TYPE counter counter\ncounter{label=\"test\"} 6\n");
   }

   SECTION("histogram<double>")
   {
      auto histogram = manager.create<prom::histogram<double>>(
         {.name = "histogram", .help = "histogram help"},
         std::vector<double>{1.5, 5.5, 10.5, std::numeric_limits<double>::max()}, true);

      histogram.observe(1.);
      histogram.observe(1.);
      histogram.observe(3.);
      histogram.observe(2.);
      histogram.observe(1.);
      histogram.observe(5.5);
      histogram.observe(9.);
      histogram.observe(10.);
      histogram.observe(11.);

      auto result = manager.format_metrics();
      REQUIRE(result ==
         R"(# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="1.5"} 3
# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="5.5"} 3
# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="10.5"} 2
# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="+Inf"} 1
# HELP histogram_sum histogram help
# TYPE histogram_sum counter
histogram_sum 43.5
# HELP histogram_count histogram help
# TYPE histogram_count counter
histogram_count 9
)");
   }

   SECTION("histogram<int64>")
   {
      auto histogram = manager.create<prom::histogram<std::int64_t>>(
         {.name = "histogram", .help = "histogram help"},
         std::vector<int64_t>{1, 5, 10, std::numeric_limits<int64_t>::max()}, true);

      histogram.observe(1);
      histogram.observe(1);
      histogram.observe(3);
      histogram.observe(2);
      histogram.observe(1);
      histogram.observe(5);
      histogram.observe(9);
      histogram.observe(10);
      histogram.observe(11);

      auto result = manager.format_metrics();
      REQUIRE(result ==
         R"(# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="1"} 3
# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="5"} 3
# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="10"} 2
# HELP histogram_bucket histogram help
# TYPE histogram_bucket counter
histogram_bucket{le="+Inf"} 1
# HELP histogram_sum histogram help
# TYPE histogram_sum counter
histogram_sum 43
# HELP histogram_count histogram help
# TYPE histogram_count counter
histogram_count 9
)");
   }

   SECTION("test bind to router")
   {
      using namespace spider2;

      auto counter = manager.create<prom::gauge>({.name = "gauge", .help = "gauge help"});
      auto app = begin_app() + on_get("/metrics", prometheus::create_prometheus_endpoint(manager));
      static_cast<void>(app);
   }
}
