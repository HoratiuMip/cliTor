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

#pragma region UIX
ret_t Gateway::uix_up( 
    IN   const uix_up_args_t&   C
) {
    ASSERT_OR( not _uix ) uix_down();
    _uix = std::make_shared< _uix_t >();

    _uix->imm_th = std::jthread( &rgh::Immersive::main, _uix->imm.get(), 0, nullptr, rgh::Immersive::config_t{
        .ctx        = nullptr,
        .title      = args_.title,
        .width      = args_.width,
        .height     = args_.height,
        .srf_bgn_as = args_.bgnas,
        .init_cb    = [ this, fs = args_.font_scale, styler = args_.styler ] ( const auto& args_ ) {
            if( styler ) styler();
            _uix->imm->imgui.io->FontGlobalScale = fs;
            _uix->imm->disengage_face_culling();   

        //# Notify active docks to load their UIX stuff.
            logger->info( "bridge: uix up: notifying docks..." );
            for( auto& [ id, dock ] : *_dock_tbl.control() ) {
                dock->_uix_pack = dock->dock_uix_begin();
            }
            logger->info( "bridge: uix up: docks notified." );

            return OK;
        },
        .loop_cb    = [ this ] ( const auto& args_ ) { 
            return _uix_frame( args_ );
        },
        .exit_cb    = [ this ] ( const auto& args_ ) { 
            if( _config.uix_bound ) push( [ this ] { daemon_stop(); } );
            return OK;
        }
    } );
}
#pragma endregion UIX

};