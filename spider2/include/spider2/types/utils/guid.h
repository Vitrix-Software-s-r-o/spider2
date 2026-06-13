#pragma once
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

namespace spider2::guid
{

   template <class Mutex = std::mutex>
   struct uuid_generator
   {
    public:
      inline static std::uint32_t unsafe_seed()
      {
         auto time = static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
         auto thread_id = static_cast<std::uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

         return time ^ thread_id;
      }
      inline uuid_generator(std::uint32_t seed = uuid_generator::unsafe_seed()) : mt_(seed), gen_(mt_) {}

      boost::uuids::uuid generate_uuid()
      {
         std::lock_guard lk{mutex_};
         return gen_();
      }

    private:
      boost::random::mt19937 mt_;
      boost::uuids::basic_random_generator<boost::random::mt19937> gen_;
      std::mutex mutex_;
   };

   inline boost::uuids::uuid generate_unsafe_uuid()
   {
      static uuid_generator<> g_shared_generator;
      return g_shared_generator.generate_uuid();
   }

   inline std::string generate_unsafe_uuid_string()
   {
      return boost::lexical_cast<std::string>(generate_unsafe_uuid());
   }

   inline boost::uuids::uuid convert_uuid(std::array<std::uint8_t, 16> const &id)
   {
      static_assert(sizeof(std::array<std::uint8_t, 16>) == sizeof(boost::uuids::uuid), "uuid size must match");
      boost::uuids::uuid result;
      std::copy(id.begin(), id.end(), result.begin());

      return result;
   }

   inline std::array<std::uint8_t, 16> convert_to_std(boost::uuids::uuid id)
   {
      static_assert(sizeof(std::array<std::uint8_t, 16>) == sizeof(boost::uuids::uuid), "uuid size must match");
      std::array<std::uint8_t, 16> result;
      std::copy(id.begin(), id.end(), result.begin());

      return result;
   }

} // namespace spider2::guid