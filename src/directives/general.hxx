#pragma once /*
# FILE: directives/general.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: General.
*/
#include <rgh/brp/descriptor.hpp>

#include <spdlog/spdlog.h>

#define CLITOR_VERSION_MAJOR 1
#define CLITOR_VERSION_MINOR 0
#define CLITOR_VERSION_PATCH 0
#define CLITOR_VERSION_STR "cliTor-v1.0.0"

#define CLITOR_NAMESPACE namespace clitor

using rgh::ret_t;
using rgh::status_t;

CLITOR_NAMESPACE {

extern std::shared_ptr< spdlog::logger > _bridge_logger;
#define BRIDGE_LOGI( ... ) _bridge_logger->info( __VA_ARGS__ )
#define BRIDGE_LOGW( ... ) _bridge_logger->warn( __VA_ARGS__ )
#define BRIDGE_LOGE( ... ) _bridge_logger->error( __VA_ARGS__ )

};

/**
 * @brief Use this macro after your includes in your main junction file.
 * @example #include JUNCTION_HEADER( my_app, "MyApp" )
 */
#define JUNCTION_HEADER(namespace_name,junction_name) \
    <bridge.hpp> \
    static const char* const JUNCTION_NAME = junction_name; \
    namespace namespace_name {
/**
 * @brief Use this macro at the end of your main junction file.
 */
#define JUNCTION_FOOTER \
    };
    