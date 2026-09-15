#pragma once /*
# FILE: directives/bridge_directive.cxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#include "gateway_directive.hxx"
#include "../bridge.hxx"

#include <spdlog/sinks/stdout_color_sinks.h>

CLITOR_NAMESPACE {
extern Bridge BridgE;

std::shared_ptr< spdlog::logger > _bridge_logger = {};

};