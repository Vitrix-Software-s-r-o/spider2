//
// Created by jhrub on 20.02.2023.
//

#pragma once

#include <spider2/types.h>

namespace spider2::prometheus
{
   using shared_int_value_ptr = std::shared_ptr<std::atomic_int64_t>;

   struct gauge
   {
      constexpr static string_view type = "gauge";
      using value_type = std::int64_t;

      inline gauge()
         : value_ptr_(std::make_shared<std::atomic_int64_t>(0))
      {
      }

      inline explicit operator std::int64_t() const
      {
         return value_ptr_->load();
      }

      inline std::int64_t increment(std::int64_t value = 1)
      {
         return value_ptr_->operator+=(value);
      }

      inline std::int64_t decrement(std::int64_t value = 1)
      {
         return value_ptr_->operator-=(value);
      }

      inline void set(std::int64_t value = 1)
      {
         value_ptr_->operator=(value);
      }

      inline auto increment_scope(std::int64_t value = 1)
      {
         struct increment_scope
         {
            ~increment_scope()
            {
               value_ptr->operator-=(value);
            }

            shared_int_value_ptr value_ptr;
            std::int64_t value;
         };

         increment(value);
         return increment_scope{value_ptr_, value};
      }

   private:
      shared_int_value_ptr value_ptr_;
   };

   struct counter
   {
   public:
      constexpr static string_view type = "counter";
      using value_type = std::int64_t;

      inline counter()
         : value_ptr_(std::make_shared<std::atomic_int64_t>(0))
      {
      }

      inline explicit operator std::int64_t() const
      {
         return value_ptr_->load();
      }

      inline std::int64_t increment(std::uint32_t value = 1)
      {
         return value_ptr_->operator+=(static_cast<std::int64_t>(value));
      }

   private:
      shared_int_value_ptr value_ptr_;
   };

   template <class T>
   struct histogram_value
   {
      using bucket_type = T;

      vector<T> buckets_ = {};
      vector<std::int64_t> buckets_counts_ = {};
      std::optional<T> sum_ = {};
      std::int64_t count_ = 0;
   };

   template <class T>
   struct protected_histogram_value : histogram_value<T>
   {

      mutable std::mutex mutex_;

      explicit protected_histogram_value(vector<T> buckets, bool include_sum)
         : histogram_value<T>{std::move(buckets)}
      {
         this->buckets_counts_.resize(this->buckets_.size());
         if (include_sum)
         {
            this->sum_.emplace();
         }
      }

      inline void observe(T value, std::int64_t count = 1)
      {
         if constexpr (std::is_floating_point_v<T>)
         {
            if (std::isnan(value) || std::isinf(value))
            {
               return;
            }
         }

         auto lk = std::lock_guard{mutex_};
         for (std::size_t i = 0, len = this->buckets_.size(); i != len; ++i)
         {
            if (value <= this->buckets_[i])
            {
               this->buckets_counts_[i] += count;
            }
         }

         if (this->sum_.has_value())
         {
            this->sum_.value() += value * count;
         }
         this->count_ += count;
      }
   };

   template <class T>
   struct histogram
   {
   public:
      constexpr static string_view type = "histogram";
      using value_type = histogram_value<T>;

      inline explicit histogram(vector<T> buckets, bool include_sum = false)
         : value_ptr_(std::make_shared<protected_histogram_value<T>>(std::move(buckets), include_sum))
      {
      }

      inline explicit operator histogram_value<T>() const
      {
         const auto value_ptr = value_ptr_;
         auto lk = std::lock_guard{value_ptr->mutex_};

         return static_cast<histogram_value<T> const &>(*value_ptr);
      };

      inline void observe(T value, std::int64_t count = 1)
      {
         value_ptr_->observe(value, count);
      }

   private:
      std::shared_ptr<protected_histogram_value<T>> value_ptr_;
   };

   using metrics_type = std::variant<gauge, counter, histogram<std::int64_t>, histogram<double>>;

   struct metrics_label
   {
      string_view name;
      string_view help = {};
      string_view labels = {};
   };

   class metrics_manager
   {
   public:
      metrics_manager();
      ~metrics_manager();

      template <class T, class... Args>
      T create(metrics_label label, Args &&... args)
      {
         std::lock_guard lk{mutex_};
         metrics_.push_back(T{std::forward<Args>(args)...});
         labels_.push_back(label);

         return std::visit(overload{[](T &val) -> T
                                    {
                                       return val;
                                    },
                                    [](auto &) -> T
                                    {
                                       throw std::logic_error{"this cannot happen"};
                                    }},
                           metrics_.back());
      }

      string format_metrics() const;

   private:
      mutable std::mutex mutex_;
      std::deque<metrics_type> metrics_;
      std::vector<metrics_label> labels_;
   };

   inline auto create_prometheus_endpoint(metrics_manager const &manager)
   {
      using namespace spider2;
      return [&](request &req) -> await_response
      {
         return [](metrics_manager const &manager) -> await_response
         {
            co_return response::return_string(http::status::ok, manager.format_metrics(), "text/plain");
         }(manager);
      };
   };

} // namespace spider2::prometheus
