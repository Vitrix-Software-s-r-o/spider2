//
// Created by jhrub on 3/10/2024.
//

#pragma once

/**
 * @def SPIDER2_DEFAULT_FORMATTER_DECLARATION_HEADER_FILE
 * @brief Default formatter declaration header file.
 * You can provide your implementation to opt-out Boost.JSON.
 *
 * Use use_boost_json.h as an example.
 *
 */
#if !defined(SPIDER2_DEFAULT_SUPPORT_HEADER_FILE)
#define SPIDER2_DEFAULT_SUPPORT_HEADER_FILE "spider2/api/support/boost_json/use_boost_json.h"
#endif

#include SPIDER2_DEFAULT_SUPPORT_HEADER_FILE

namespace spider2::api
{
   using default_formatter_tag = SPIDER2_DEFAULT_FORMATTER;
}

#include SPIDER2_SUPPORT_IMPL_HEADER_FILE