#pragma once /*
# FILE: directives/proxy_directive.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Proxy related stuff.
*/
#include "descriptor_directive.hxx"
#include "dock_directive.hxx"

#include <memory>
#include <thread>

#include <rgh/osp/immersive.hpp>

CLITOR_NAMESPACE {

class UIX_Directive {
public: _DIRECTIVE_FRIENDS

protected:
    using _dock_entry_t = Dock_Directive::_dock_entry_t;

public:
    struct uix_up_args_t {
        const char*                     title        = CLITOR_VERSION_STR;
        int                             width        = 648;
        int                             height       = 480;
        float                           font_scale   = 1.26f;
        rgh::Immersive::Word_           bgnas        = rgh::Immersive::Default;
        std::function< void( void ) >   styler       = &UIX_Directive::styler_dark_cyberpunk;
    };

protected:
//# The UIX structure that gets created when the UIX is started.
    struct _ctx_t {
    //# Graphics framework.
        std::shared_ptr< rgh::Immersive >     imm      = std::make_shared< rgh::Immersive >();
    //# The thread that runs the graphics framework.
        std::jthread                          imm_th   = {};
    //# Reference to the focused dock. Safe to store in raw pointer since it is accessed under reference lock.
        std::atomic< const _dock_entry_t* >   focus    = {};
    };
    std::shared_ptr< _ctx_t >   _ctx   = nullptr;

public:
    rgh::ret_t uix_up( 
        IN   const uix_up_args_t&
    );
    void uix_down( void );

    rgh::ret_t uix_focus(
        IN   const _dock_entry_t&
    );
    rgh::ret_t uix_focus(
        IN   std::string_view
    );
    rgh::ret_t uix_unfocus( 
        IN   const Dock*   = nullptr
    );

    rgh::ret_t uix_decl_bound( 
        IN   const uix_up_args_t& 
    );

    bool uix_is_up( void ) { return _ctx.use_count() > 0; }
    std::shared_ptr< rgh::Immersive > uix_imm_strong( void ) { auto ctx = _ctx; return ctx ? ctx->imm : nullptr; }
    rgh::Immersive* uix_imm_weak( void ) { return _ctx->imm.get(); }
    operator rgh::Immersive* ( void ) { return uix_imm_weak(); }

protected:
    rgh::ret_t _uix_frame(
        IN   const rgh::Immersive::frame_cb_args_t&
    );
    
public:
//# Cyberpunk theme from: https://github.com/ocornut/imgui/issues/707.
    static void styler_dark_cyberpunk( void );
//# ASIMOV skin.
    static void styler_light_industrial( void );
};

}//#CLITOR_NAMESPACE