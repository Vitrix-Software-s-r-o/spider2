#pragma once
#include "spider2/types/platform.h"

namespace spider2::base64
{
     auto encode(string_view data) ->string;
     auto decode(string_view data) ->optional<string>;
}