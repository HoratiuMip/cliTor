#pragma once /*
# FILE: directives/general.cxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#include "general.hxx"

#include <spdlog/sinks/stdout_color_sinks.h>

CLITOR_NAMESPACE {

std::shared_ptr< spdlog::logger > _bridge_logger = {};

};