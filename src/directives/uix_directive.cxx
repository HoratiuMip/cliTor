#pragma once /*
# FILE: directives/uix_directive.cxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#include "uix_directive.hxx"

CLITOR_NAMESPACE {

rgh::ret_t UIX_Directive::up( 
    IN   const up_args_t&   args_
) {
//# Load or reload the context.
    ASSERT_OR( not _ctx ) this->down();
    _ctx = std::make_shared< _ctx_t >();
    ASSERT_OR( _ctx ) return ERR_BADALLOC;

//# Launch the UIX thread.
    _ctx->imm_th = std::jthread( &rgh::Immersive::main, _ctx->imm.get(), 0, nullptr, rgh::Immersive::config_t{
        .ctx        = nullptr,
        .title      = args_.title,
        .width      = args_.width,
        .height     = args_.height,
        .srf_bgn_as = args_.bgnas,

        .init_cb = [ this, fs = args_.font_scale, styler = args_.styler ] ( const auto& C ) {
            DIRECTIVE_BRIDGE_SELF_SHIFT;

        //# Styile the theme and configure the graphics framework.
            if( styler ) styler();
            _ctx->imm->imgui.io->FontGlobalScale = fs;
            _ctx->imm->disengage_face_culling();   

        //# Notify active docks to load their UIX stuff.
            BRIDGE_LOGI( "uix up: notifying docks..." );
            for( auto& [ id, dock ] : *DBSS._dock_tbl.control() ) {
                dock->_uix_pack = dock->dock_uix_begin();
            }
            BRIDGE_LOGI( "uix up: docks notified." );

            return OK;
        },
        .loop_cb = [ this ] ( const auto& C ) { 
            return _uix_frame( C );
        },
        .exit_cb = [ this ] ( const auto& C ) { 
            DIRECTIVE_BRIDGE_SELF_SHIFT;

            if( DBSS._config.uix_bound ) DBSS.push( [ DBSS ] { DBSS.daemon_stop(); } );
            return OK;
        }
    } );
}

void down( void ) {
    auto ctx = std::move( _ctx ); ASSERT_OR( ctx ) return;

    DIRECTIVE_BRIDGE_SELF_SHIFT;

//# Notify active docks to unload their UIX stuff.
    BRIDGE_LOGI( "uix down: notifying docks..." );
    for( auto& [ id, dock ] : *DBSS._dock_tbl.control() ) {
        dock->dock_uix_end();
    }
    BRIDGE_LOGI( "uix down: docks notified." );
    
    ctx->imm->sig_main_exit();
}

// //# Place the dock in focus.
// status_t uix_focus(
//     IN   const _dock_entry_t&   dken_
// ) {
// //# Assert that UIX is up.
//     auto uix = _uix; ASSERT_OR( uix ) return ERR_NO_RESOLVE;
// //# Store the dock entry into the focus reference.
//     uix->focus.store( &dken_, std::memory_order_relaxed );
//     return OK;
// }
// //# Place the dock indexed by the given ID in focus.
// status_t uix_focus(
//     IN   const std::string&   id_
// ) {
// //# Acquire watch over the dock table and set the UIX focus.
//     auto dock_tbl = _dock_tbl.watch(); ASSERT_OR( dock_tbl ) return ERR_BUSY;
// //# Index the dock.
//     auto itr = dock_tbl->find( id_ ); ASSERT_OR( itr != dock_tbl->end() ) return ERR_NOT_FOUND;
// //# Place in focus.
//     uix_focus( itr->second );
//     return OK;
// }

// //# Remove the currently focused or specified dock, if any.
// status_t uix_unfocus( 
//     IN   const Dock*   dock_ = nullptr
// ) {
// //# Assert that UIX is up.
//     auto uix = _uix; ASSERT_OR( uix ) return ERR_NO_RESOLVE;
// //# Check if the current focused dock is the requested one.
//     const auto* crt_focus = _uix->focus.load( std::memory_order_relaxed );
//     ASSERT_OR( crt_focus ) return OK;
//     if( dock_ && dock_ != crt_focus->ref.get() ) return OK;
// //# Drop focus.
//     _uix->focus.store( nullptr, std::memory_order_relaxed );
//     return OK;
// }

// bool uix_is_up() { return _uix.use_count() > 0; }

// std::shared_ptr< rgh::Immersive > uix_imm_strong() { 
//     auto uix = _uix; 
//     ASSERT_OR( uix ) return nullptr;
//     return uix->imm; 
// }

// rgh::Immersive* uix_imm_weak() { return _uix->imm.get(); }
// operator rgh::Immersive*() { return uix_imm_weak(); }

// protected:
// status_t _uix_frame( const rgh::Immersive::frame_cb_args_t& args_ ) {
//     _uix->imm->clear();

//     const auto* viewport = ImGui::GetMainViewport();
//     ImGui::SetNextWindowPos( viewport->WorkPos );
//     ImGui::SetNextWindowSize( viewport->WorkSize );

//     const auto* focus = _uix->focus.load( std::memory_order_relaxed );
//     if( not focus ) [[likely]] {
//         ImGui::Begin( CLITOR_VERSION_STR, nullptr, 
//             ImGuiWindowFlags_NoDecoration |
//             ImGuiWindowFlags_NoMove |
//             ImGuiWindowFlags_NoResize |  
//             ImGuiWindowFlags_NoSavedSettings
//         );

//         if( ImGui::BeginTable( "##tbl-proxy-dock-split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV ) ) {
//             ImGui::TableSetupColumn( "##proxy-zone", ImGuiTableColumnFlags_WidthFixed, 196 );
//             ImGui::TableSetupColumn( "##dock-zone", ImGuiTableColumnFlags_WidthStretch );

//             ImGui::TableNextColumn(); {
//                 ImGui::Separator();

//                 static constexpr const char* const ANIM_STRS[ 5 ] = {
//                     "   (O)   ",
//                     "  (( ))  ",
//                     " ((   )) ",
//                     "((  .  ))",
//                     "(   o   )"
//                 };
//                 const char* crt_anim_str = ANIM_STRS[ static_cast< int >( args_.t*5 ) % 5 ];
//                 ImGui::TextUnformatted( crt_anim_str, crt_anim_str+9 );

//                 ImGui::SeparatorText( "Proxy Zone" );
                
//                 for( auto& [ id, pxen ] : *_proxy_tbl.watch() ) {
//                     ASSERT_OR( not id.starts_with( "#" ) ) continue;

//                     ImGui::PushID( &*id.cbegin(), &*id.cend() );
//                         ImGuiTreeNodeFlags col_hdr_flags = ImGuiTreeNodeFlags_DefaultOpen;

//                         const bool uix_frm_ovr = pxen->_static_fields.uix.has_basic_uix_frame_overridden;
//                         if( not uix_frm_ovr ) col_hdr_flags |= ImGuiTreeNodeFlags_Bullet;

//                         const bool proxy_header_expanded = ImGui::CollapsingHeader( id.c_str(), col_hdr_flags );
//                         bool proxy_will_auto_install = ImGui::IsItemHovered() and ( ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) or rgh::Immersive::ctrl( ImGuiKey_T ) );
        
//                         if( proxy_header_expanded and uix_frm_ovr ) {
//                             pxen->proxy_uix_frame( { args_ } );
//                         }

//                         if( proxy_will_auto_install ) {
//                             push( [ this, pxid = id ] { proxy_pass( pxid, "install" ); } );
//                         }
//                     ImGui::PopID();
//                 }
//             }

//             ImGui::TableNextColumn(); {
//                 auto dock_tbl = _dock_tbl.watch(); ASSERT_OR( dock_tbl ) return ERR_BUSY;

//                 const ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll |
//                                                         ImGuiTabBarFlags_AutoSelectNewTabs   |
//                                                         ImGuiTabBarFlags_DrawSelectedOverline;

//                 if( ImGui::BeginTabBar( "##tabs-dock", tab_bar_flags ) ) {
//                     for( auto& [ id, dock ] : *dock_tbl ) {
//                         ASSERT_OR( not id.starts_with( '#' ) ) continue;

//                         ImGui::PushID( &*id.cbegin(), &*id.cend() );
//                             bool tab_open = true;

//                             const ImGuiTabItemFlags tab_item_flags = ImGuiTabItemFlags_None;

//                             if( ImGui::BeginTabItem( _dock_id_c_str( id ), dock->dock_uix_persistent() ? nullptr : &tab_open, tab_item_flags ) ) {
//                                 if( rgh::Immersive::was_dbl_clk() ) {
//                                     uix_focus( dock );
//                                 }

//                                 ImGui::BeginChild( "##dock_frame", ImVec2{ 0, -ImGui::GetFrameHeightWithSpacing() }, ImGuiChildFlags_Border );
//                                     dock->dock_uix_frame( { args_, dock->_uix_pack.get() } );
//                                 ImGui::EndChild(); ImGui::EndTabItem();

//                                 if( rgh::Immersive::ctrl( ImGuiKey_W ) ) tab_open = false;
//                             }
                            
//                             if( not tab_open ) { push( [ this, id ] { uninstall_dock( id ); } ); }
//                         ImGui::PopID();
//                     }

//                     ImGui::EndTabBar();
//                 }
//             }
//             ImGui::EndTable();
//         }

//         ImGui::Separator();
//     } else {
//         bool focused = true;
        
//         auto dock_tbl = _dock_tbl.watch(); ASSERT_OR( dock_tbl ) return ERR_BUSY;
//                 focus    = _uix->focus.load( std::memory_order_relaxed );

//         ImGui::Begin( _dock_id_c_str( focus->ref->dock_id() ), &focused, 
//             ImGuiWindowFlags_NoMove          |
//             ImGuiWindowFlags_NoResize        |  
//             ImGuiWindowFlags_NoSavedSettings |
//             ImGuiWindowFlags_NoCollapse
//         );

//         focus->ref->dock_uix_frame( { args_, focus->ref->_uix_pack.get() } );

//         if( not focused ) uix_unfocus();
//     }

//     ImGui::End();
//     return daemon_is_started() ? OK : ERR_TERMINATED;
// }

void UIX_Directive::styler_dark_cyberpunk( void ) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ScrollbarSize = 13.0f;
    style.GrabMinSize = 10.0f;

    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 0.62f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.20f, 0.40f, 0.35f, 1.00f);

    colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.04f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.02f, 0.02f, 0.04f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.02f, 0.02f, 0.04f, 0.98f);

    colors[ImGuiCol_Border] = ImVec4(1.00f, 0.00f, 0.25f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 0.00f, 0.25f, 0.20f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 0.00f, 0.25f, 0.20f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.00f, 0.25f, 0.40f);

    colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.02f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.02f, 0.02f, 0.04f, 1.00f);

    colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.04f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.00f, 0.93f, 0.04f, 0.60f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 0.93f, 0.04f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.93f, 0.04f, 1.00f);

    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.93f, 0.04f, 1.00f); 
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.00f, 0.25f, 0.80f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.00f, 0.25f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.00f, 1.00f, 0.62f, 0.20f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 1.00f, 0.62f, 0.50f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 0.62f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 0.00f, 0.25f, 0.30f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.00f, 0.25f, 0.50f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.00f, 0.25f, 1.00f);

    colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 0.00f, 0.25f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.80f, 0.00f, 0.20f, 1.00f);

    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.93f, 0.04f, 0.30f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.25f, 1.00f);

    colors[ImGuiCol_Separator] = ImVec4(1.00f, 0.93f, 0.04f, 0.80f);

    colors[ImGuiCol_TableBorderStrong] = ImVec4(1.00f, 0.93f, 0.04f, 0.80f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(1.00f, 0.93f, 0.04f, 0.64f);
}

void UIX_Directive::styler_light_industrial( void ) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowPadding     = ImVec2(14.0f, 14.0f);
    style.FramePadding      = ImVec2(10.0f, 6.0f); 
    style.CellPadding       = ImVec2(8.0f, 5.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 16.0f;
    style.GrabMinSize       = 14.0f;

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.5f;
    style.TabBorderSize     = 1.0f;

    colors[ImGuiCol_Text]                  = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.97f, 0.97f, 0.96f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

    colors[ImGuiCol_Border]                = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.16f, 0.16f, 0.17f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.95f, 0.45f, 0.05f, 0.80f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);

    colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.95f, 0.45f, 0.05f, 0.15f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.95f, 0.45f, 0.05f, 0.28f);

    colors[ImGuiCol_TitleBg]               = ImVec4(0.86f, 0.86f, 0.84f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.86f, 0.86f, 0.84f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.86f, 0.86f, 0.84f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.84f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.90f, 0.90f, 0.89f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.95f, 0.45f, 0.05f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);

    colors[ImGuiCol_CheckMark]             = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);

    colors[ImGuiCol_Button]                = ImVec4(0.82f, 0.82f, 0.80f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.95f, 0.45f, 0.05f, 0.85f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);

    colors[ImGuiCol_Header]                = ImVec4(0.16f, 0.16f, 0.17f, 0.32f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.95f, 0.45f, 0.05f, 0.35f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.95f, 0.45f, 0.05f, 0.55f);

    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.86f, 0.86f, 0.84f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.55f, 0.55f, 0.56f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.16f, 0.16f, 0.17f, 0.04f);

    colors[ImGuiCol_Tab]                   = ImVec4(0.82f, 0.82f, 0.80f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.95f, 0.45f, 0.05f, 0.85f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.82f, 0.82f, 0.80f, 0.70f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.95f, 0.45f, 0.05f, 0.55f);

    colors[ImGuiCol_PlotLines]             = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.80f, 0.35f, 0.02f, 1.00f);

    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.95f, 0.45f, 0.05f, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.95f, 0.45f, 0.05f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.95f, 0.45f, 0.05f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.95f, 0.45f, 0.05f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.16f, 0.16f, 0.17f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.16f, 0.16f, 0.17f, 0.45f);
}

}//# CLITOR_NAMESPACE