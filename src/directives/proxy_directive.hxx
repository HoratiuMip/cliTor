#pragma once /*
# FILE: directives/proxy_directive.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Proxy related stuff.
*/
#include "gateway_directive.hxx"
#include "dock_directive.hxx"

#include <rgh/gep/dispenser.hpp>
#include <rgh/osp/immersive.hpp>

#define JUNCTION_PROXY_INSTALL(t, ...) \
    static struct _junction_proxy_installer_##t##_t_ { \
        _junction_proxy_installer_##t##_t_( void ) { \
            BridgE.install_proxy( rgh::HVec< t >::make( __VA_ARGS__ ) ); \
        } \
    } _junction_proxy_installer_##t##_; 

class Proxy {
public: friend class Bridge;
        friend class Proxy_Directive;

protected:
/**
 * @brief Fields and configs populated by the bridge once, when the proxy is installed.
 */
    struct _static_fields_t {
        struct _uix {
        /**
         * @brief Whether this proxy has overridden the basic UIX frame.
         */
            bool   has_basic_uix_frame_overridden   = false;
        } uix;
    } _static_fields;

public: 
/**
 * @brief Required function returning the name of the proxy.
 * @note The macro JUNCTION_PROXY_GET_NAME is provided to override this function.
 */
    virtual std::string_view proxy_get_name() const = 0;
#define JUNCTION_PROXY_GET_NAME \
    virtual std::string_view proxy_get_name() const override { return JUNCTION_NAME; }
    
/**
 * @brief Optional function returning the address of the proxy if it is itself a dock. Useful when you need
 *          a "single dock" application, so your proxy can double as a dock.
 * @note The macro JUNCTION_PROXY_IS_DOCK is provided to override this function.
 */
    virtual Dock* proxy_as_dock() { return nullptr; }
#define JUNCTION_PROXY_IS_DOCK \
    virtual Dock* proxy_as_dock() override { return static_cast< Dock* >( this ); }

/**
 * @brief Optional function called to pass text commands to the proxy.
 * @note The macro JUNCTION_PROXY_PASS_FNC_SIG is provided to replace the function signature.
 */
    virtual ret_t proxy_pass( std::string line_ ) { return ERR_NOT_IMPL; };
#define JUNCTION_PROXY_PASS_FNC_SIG \
    virtual ret_t proxy_pass( std::string line_ ) override
    
/**
 * @brief Optional function called after the bridge initialized. Any calls to the bridge from your proxy must
 *          be done DURING or AFTER this function is called by the bridge.
 * @note The macro JUNCTION_PROXY_INIT_PROC_FNC_SIG is provided to replace the function signature.
 */
    virtual ret_t proxy_init_proc() { return OK; }
#define JUNCTION_PROXY_INIT_PROC_FNC_SIG \
    virtual ret_t proxy_init_proc() override

#pragma region UIX
//# For now we keep it down to pure basic ImGui calls, no advanced graphics which
//#   would require the proxy to have a UIX pack. 
    struct proxy_uix_frame_args_t : rgh::Immersive::frame_cb_args_t {};

/**
 * @brief Optional function called to draw the this proxy's zone.
 * @note The macro JUNCTION_PROXY_UIX_FRAME_FNC_SIG is provided to replace the function signature.
 */
    virtual ret_t proxy_uix_frame( const proxy_uix_frame_args_t& C ) { return ERR_NOT_IMPL; }
#define JUNCTION_PROXY_UIX_FRAME_FNC_SIG \
    virtual ret_t proxy_uix_frame( const proxy_uix_frame_args_t& C ) override
#pragma endregion UIX
};

class Proxy_Directive : virtual public Gateway {
public: friend class Bridge;

protected:
    struct _proxy_key_t {
        std::string   name   = {};

        struct less {
            using is_transparent = void;

            inline bool operator () ( const _proxy_key_t& lhs_, const _proxy_key_t& rhs_ ) const { return lhs_.name < rhs_.name; }
            inline bool operator () ( const _proxy_key_t& lhs_, std::string rhs_ ) const { return lhs_.name < rhs_; }
            inline bool operator () ( const _proxy_key_t& lhs_, std::string_view rhs_ ) const { return lhs_.name < rhs_; }
            inline bool operator () ( const _proxy_key_t& lhs_, const char* rhs_ ) const { return lhs_.name < rhs_; }
        };
    };
    struct _proxy_entry_t {
        rgh::HVec< Proxy >   ref   = nullptr;

        inline auto operator->() const { return ref.operator->(); }
    };

protected:
    rgh::Dispenser< std::map< std::string, _proxy_entry_t, _proxy_key_t::less > >   _proxy_tbl   = { rgh::DispenserMode_Lock };

protected:
    void _proxy_entry_populate_static_fields(
        IN   _proxy_entry_t&   pxen_
    );

public:
    ret_t install_proxy(
        IN   rgh::HVec< Proxy >&&   proxy_
    );

    status_t uninstall_proxy(
        IN   std::string_view   pxnm_
    );

    status_t proxy_pass(
        IN   std::string_view   pxnm_,
        IN   std::string        line_
    );
#pragma endregion PROXY
};