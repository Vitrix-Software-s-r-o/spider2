//
// Created by jhrub on 20.02.2023.
//
#include "spider2/prometheus.h"

namespace spider2::prometheus
{
    namespace
    {
        template <typename T>
        concept numeric_type = std::integral<T> || std::floating_point<T>;

        template <typename T>
        concept histogram_type =
            std::is_same_v<T, histogram_value<std::int64_t>> || std::is_same_v<T, histogram_value<double>>;

        void format_metrics_to_buffer(fmt::memory_buffer &buffer, const metrics_label &label, string_view type_name,
                                      numeric_type auto value)
        {
            if (!label.help.empty())
            {
                fmt::format_to(buffer, "# HELP {} {}\n", label.name, label.help);
            }
            fmt::format_to(buffer, "# TYPE {} {}\n", label.name, type_name);

            if (label.labels.empty())
            {
                fmt::format_to(buffer, "{} {}\n", label.name, value);
            }
            else
            {
                fmt::format_to(buffer, "{}{{{}}} {}\n", label.name, label.labels, value);
            }
        }

        /// @brief Formats the metrics for a histogram in the prometheus format
        template <histogram_type T>
        void format_metrics_to_buffer(fmt::memory_buffer &buffer, const metrics_label &label, string_view /*type_name*/,
                                      T const &value)
        {
            auto bucket_label = fmt::format("{}_bucket", label.name);
            for (std::size_t idx = 0, len = value.buckets_.size(); idx != len; ++idx)
            {
                auto &bucket = value.buckets_[idx];
                auto &bucket_count = value.buckets_counts_[idx];

                if (!label.help.empty())
                {
                    fmt::format_to(buffer, "# HELP {} {}\n", bucket_label, label.help);
                }
                fmt::format_to(buffer, "# TYPE {} {}\n", bucket_label, counter::type);


                if (std::numeric_limits<typename T::bucket_type>::max() == bucket)
                {
                    if (label.labels.empty())
                    {
                        fmt::format_to(buffer, "{}{{le=\"+Inf\"}} {}\n", bucket_label, bucket_count);
                    }
                    else
                    {
                        fmt::format_to(buffer, "{}{{le=\"+Inf\" {}} {}\n", bucket_label, label.labels, bucket_count);
                    }
                    break;
                }


                if (label.labels.empty())
                {
                    fmt::format_to(buffer, "{}{{le=\"{}\"}} {}\n", bucket_label, bucket, bucket_count);
                }
                else
                {
                    fmt::format_to(buffer, "{}{{le=\"{}\" {}} {}\n", bucket_label, label.labels, bucket, bucket_count);
                }
            }

            const auto count_label = fmt::format("{}_count", label.name);

            if (value.sum_.has_value())
            {
                const auto sum_label = fmt::format("{}_sum", label.name);

                format_metrics_to_buffer(buffer, metrics_label{sum_label, label.help, label.labels}, counter::type,
                                         value.sum_.value());
            }

            format_metrics_to_buffer(buffer, metrics_label{count_label, label.help, label.labels}, counter::type,
                                     value.count_);
        }

    } // namespace
} // namespace spider2::prometheus

spider2::prometheus::metrics_manager::metrics_manager() {}
spider2::prometheus::metrics_manager::~metrics_manager() {}

spider2::string spider2::prometheus::metrics_manager::format_metrics() const
{
    fmt::memory_buffer buffer;
    buffer.reserve(2048);

    std::lock_guard lg{mutex_};

    for (size_t i = 0, len = metrics_.size(); i != len; ++i)
    {
        auto &value_metrics = metrics_[i];
        auto &label = labels_[i];
        std::visit(
            [&](auto const &value) -> auto
            {
                auto metrics_value = static_cast<typename std::decay_t<decltype(value)>::value_type>(value);
                format_metrics_to_buffer(buffer, label, std::decay_t<decltype(value)>::type, metrics_value);
            },
            value_metrics);
    }

    return fmt::to_string(buffer);
}
