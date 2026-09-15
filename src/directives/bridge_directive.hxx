#pragma once /*
# FILE: directives/bridge_directive.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: General.
*/
#include "../descriptor.hxx"
#include "dock_directive.hxx"
#include "proxy_directive.hxx"
#include "uix_directive.hxx"

#include <rgh/brp/descriptor.hpp>
#include <rgh/osp/immersive.hpp>

#include <spdlog/spdlog.h>

CLITOR_NAMESPACE {

extern std::shared_ptr< spdlog::logger > _bridge_logger;
#define BRIDGE_LOGI( fmt, ... ) _bridge_logger->info( "bridge:" fmt __VA_OPT__(,) __VA_ARGS__ )
#define BRIDGE_LOGW( fmt, ... ) _bridge_logger->warn( "bridge:" fmt __VA_OPT__(,) __VA_ARGS__ )
#define BRIDGE_LOGE( fmt, ... ) _bridge_logger->error( "bridge:" fmt __VA_OPT__(,) __VA_ARGS__ )

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

// #define JUNCTION_DOCK_UIX_REINTR_PACK( pack_t_ ) auto* pack = reinterpret_cast< pack_t_* >( args_.pack );

// #define JUNCTION_DOCK_STOP_OR_BRIDGE_STOP \
//     (this->dock_stop_signaled() or BridgE.status() != OK)

// #define JUNCTION_DOCK_IS_UIX_PERSISTENT \
//     virtual const bool dock_uix_persistent() const override { return true; }

// #define JUNCTION_DOCK_WITH_BRIDGE_IMM_AND_UIX_PACK( pack_t_ ) \
//     auto imm = BridgE.uix_imm_strong(); auto _raw_uix_pack = this->_uix_pack; auto uix_pack = reinterpret_cast< pack_t_* >( _raw_uix_pack.get() ); \
//     ASSERT_AND( imm and uix_pack )

CLITOR_NAMESPACE {

#define DIRECTIVE_BRIDGE_SELF_SHIFT auto& DBSS = static_cast< Bridge& >( *this );  

class Bridge : public rgh::bridge_t, 

               public rgh::Daemon, 
               public rgh::Thread_pool,

               public Dock_Directive, 
               public Proxy_Directive,
               public UIX_Directive
{
public: _DIRECTIVE_FRIENDS
};

}//# CLITOR_NAMESPACE
    