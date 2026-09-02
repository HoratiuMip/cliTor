#pragma once /*
# FILE: directives/proxy_directive.cxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#include "proxy_directive.hxx"

CLITOR_NAMESPACE {

void Proxy_Directive::_proxy_entry_populate_static_fields(
        IN   _proxy_entry_t&   pxent_
) {
//# Check if the proxy implements the basic uix frame function. 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpmf-conversions"
    pxent_->_static_fields.uix.has_basic_uix_frame_overridden = RGH_ILL_HAS_OVERRIDDEN( pxent_.ref.get(), Proxy::proxy_uix_frame );
#pragma GCC diagnostic pop
}

ret_t Proxy_Directive::install_proxy(
    IN   rgh::HVec< Proxy >&&   proxy_
) {
//# Validate proxy.
    ASSERT_OR( proxy_ ) { BRIDGE_LOGE( "bridge: install proxy: null proxy." ); return ERR_BADARG; }

    auto pxnm = proxy_->proxy_get_name();
    ASSERT_OR( not pxnm.empty() ) { BRIDGE_LOGE( "bridge: install proxy: empty name." ); return ERR_BADARG; }

//# Acquire control over the proxy table and install it if it does not already exist.
    auto pxtbl = _proxy_tbl.control();
    ASSERT_OR( pxtbl ) { BRIDGE_LOGE( "bridge: install proxy (\"{}\"): bad table control.", pxnm ); return ERR_BUSY; }

    auto& pxent = ( *pxtbl )[ pxnm ];
    ASSERT_OR( not pxent.ref ) {
        pxtbl.release();
        BRIDGE_LOGE( "bridge: install proxy (\"{}\"): already installed.", pxnm );
        return ERR_WOULD_OVRWR; 
    }

    pxent.ref = std::move( proxy_ );
    _proxy_entry_populate_static_fields( pxent );

    pxtbl.release();
    BRIDGE_LOGI( "bridge: installed proxy: \"{}\".", pxnm );
    return OK;
}

status_t Proxy_Directive::uninstall_proxy(
    IN   std::string_view   pxnm_
) {
//# Acquire control over the proxy table and remove the matching entry, if any.
    auto pxtbl = _proxy_tbl.control();
    ASSERT_OR( pxtbl ) { BRIDGE_LOGE( "bridge: uninstall proxy ({}): bad table control.", pxnm_ ); return ERR_BUSY; }

    pxtbl->erase( pxnm_ );
    pxtbl.release();

    BRIDGE_LOGI( "bridge: uninstalled proxy: \"{}\".", pxnm_ );
    return OK;
}

status_t Proxy_Directive::proxy_pass(
    IN   std::string_view   pxnm_,
    IN   std::string        line_
) {
//# Acquire watch over the proxy table and pass the command line.
    auto pxtbl = _proxy_tbl.watch();
    ASSERT_OR( pxtbl ) { BRIDGE_LOGE( "bridge: proxy pass ({}): bad table lock.", pxnm_ ); return ERR_BUSY; }

    auto pxitr = pxtbl->find( pxnm_ ); 
    ASSERT_OR( pxitr != pxtbl->end() ) { BRIDGE_LOGE( "bridge: proxy pass: no such proxy: {}.", pxnm_ ); return ERR_NOT_FOUND; }

    auto pxref = pxitr->second.ref;
    pxtbl.release();
    ASSERT_OR( pxref ) { BRIDGE_LOGE( "bridge: proxy pass: bad proxy lock: {}.", pxnm_ ); return ERR_CORRUPTED; }

    return pxref->proxy_pass( std::move( line_ ) );
}

}//# CLITOR_NAMESPACE