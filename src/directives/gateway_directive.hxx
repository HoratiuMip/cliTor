#pragma once /*
# FILE: directives/gateway_directive.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: General.
*/
#include <rgh/brp/descriptor.hpp>
#include <rgh/osp/immersive.hpp>

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

// #define JUNCTION_DOCK_UIX_REINTR_PACK( pack_t_ ) auto* pack = reinterpret_cast< pack_t_* >( args_.pack );

// #define JUNCTION_DOCK_STOP_OR_BRIDGE_STOP \
//     (this->dock_stop_signaled() or BridgE.status() != OK)

// #define JUNCTION_DOCK_IS_UIX_PERSISTENT \
//     virtual const bool dock_uix_persistent() const override { return true; }

// #define JUNCTION_DOCK_WITH_BRIDGE_IMM_AND_UIX_PACK( pack_t_ ) \
//     auto imm = BridgE.uix_imm_strong(); auto _raw_uix_pack = this->_uix_pack; auto uix_pack = reinterpret_cast< pack_t_* >( _raw_uix_pack.get() ); \
//     ASSERT_AND( imm and uix_pack )

CLITOR_NAMESPACE {

class Gateway {
#pragma region UIX
public:
    struct uix_up_args_t {
        const char*                     title        = CLITOR_VERSION_STR;
        int                             width        = 648;
        int                             height       = 480;
        float                           font_scale   = 1.26f;
        rgh::Immersive::Word_           bgnas        = rgh::Immersive::Default;
        std::function< void( void ) >   styler       = &Gateway::uix_styler_dark_cyberpunk;
    };

protected:
//# The UIX structure that gets created when the UIX is started.
    struct _uix_t {
    //# Graphics framework.
        std::shared_ptr< rgh::Immersive >     imm      = std::make_shared< rgh::Immersive >();
    //# The thread that runs the graphics framework.
        std::jthread                          imm_th   = {};
    //# Reference to the focused dock. Safe to store in raw pointer since it is accessed under reference lock.
        std::atomic< const _dock_entry_t* >   focus    = {};
    };
    std::shared_ptr< _uix_t >   _uix   = nullptr;

public:
    virtual ret_t uix_up( 
        IN   const uix_up_args_t&
    );
    void uix_down( void );

    status_t uix_focus(
        IN   const _dock_entry_t&   dken_
    );
    status_t uix_focus(
        IN   std::string_view   id_
    );
    status_t uix_unfocus( 
        IN   const Dock*   dock_ = nullptr
    );

    bool uix_is_up() { return _uix.use_count() > 0; }

    std::shared_ptr< rgh::Immersive > uix_imm_strong() { 
        auto uix = _uix; 
        ASSERT_OR( uix ) return nullptr;
        return uix->imm; 
    }

    rgh::Immersive* uix_imm_weak() { return _uix->imm.get(); }
    operator rgh::Immersive*() { return uix_imm_weak(); }
#pragma endregion UIX

#pragma region UIX_stylers
//# Cyberpunk theme from: https://github.com/ocornut/imgui/issues/707.
    static void uix_styler_dark_cyberpunk( void ) {
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
//# ASIMOV skin.
    static void uix_styler_light_industrial( void ) {
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
#pragma endregion UIX_stylers 

};

}//# CLITOR_NAMESPACE
    