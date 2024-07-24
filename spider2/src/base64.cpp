#include "base64.h"
#include <cpp-base64/base64.h>
#include <cpp-base64/base64.cpp>

auto spider2::base64::encode(spider2::string_view data) -> string
{
   return base64_encode(data);
}

auto spider2::base64::decode(string_view data) -> optional<string>
{
   return base64_decode(data);
}
