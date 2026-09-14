#pragma once /*
# FILE: directives/dock_directive.cxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#include "dock_directive.hxx"

void Dock_Directive::_dock_entry_set_id(
    IN   _dock_entry_t&   dken_,
    IN   std::string      id_
) {
    Dock& dock = *dken_.ref;

    dock._dock_id = std::move( id_ ); 
}

void Dock_Directive::_dock_entry_make_logger(
    IN   _dock_entry_t&   dken_
) {
    Dock& dock = *dken_.ref;

    dock._dock_logger = std::make_shared< spdlog::logger >( _dock_id_c_str( dock._dock_id ), get_logger_sink() );
    dock._dock_logger->set_pattern( RGH_SPDLOG_PATTERN );
}

void Dock_Directive::_dock_entry_load_uix(
    IN   _dock_entry_t&   dken_
) {
    // auto imm = uix_imm_strong();
    // ASSERT_OR( imm ) return;
    
    // Dock& dock = *dken_.ref;

    // dock._uix_pack = dock.dock_uix_begin();
}

void Dock_Directive::_dock_drop_hooks(
    IN   _dock_entry_t&   dken_
) {
    //uix_unfocus( dken_.ref.get() );
}

ret_t Dock_Directive::install_dock(
    IN   std::string           id_,
    IN   rgh::HVec< Dock >&&   dock_
) {
//# Validate dock.
    ASSERT_OR( dock_ ) { BRIDGE_LOGE( "bridge: install dock: null dock." ); return ERR_BADARG; }

    ASSERT_OR( not id_.empty() ) { BRIDGE_LOGE( "bridge: install dock: empty id." ); return ERR_BADARG; }

//# Install dock.
    auto dock_tbl = _dock_tbl.control();
    ASSERT_OR( dock_tbl ) { BRIDGE_LOGE( "bridge: install dock (\"{}\"): bad table control.", id_ ); return ERR_BUSY; }

    auto& dock = ( *dock_tbl )[ id_ ];
    ASSERT_OR( not dock.ref ) {
        dock_tbl.release();
        BRIDGE_LOGE( "bridge: install dock (\"{}\"): already installed.", id_ );
        return ERR_WOULD_OVRWR; 
    }
    
    dock.ref = std::move( dock_ );
    _dock_entry_set_id( dock, std::move( id_ ) );
    _dock_entry_make_logger( dock );
    _dock_entry_load_uix( dock );

    dock_tbl.release();

    BRIDGE_LOGI( "bridge: installed dock: \"{}\".", dock.ref->_dock_id );
    return OK;
}

ret_t Dock_Directive::uninstall_dock(
    IN   std::string_view   id_
) {
//# Acquire table control and remove the requested dock, if it exists.
    auto dock_tbl = _dock_tbl.control();
    ASSERT_OR( dock_tbl ) { BRIDGE_LOGE( "bridge: uninstall dock {}: bad control.", id_ ); return ERR_BUSY; }
//# Return success even if there was no matching dock.
    auto itr = dock_tbl->find( id_ ); ASSERT_OR( itr != dock_tbl->end() ) return OK;
//# Release any hooks referencing this dock entry.
    //_dock_drop_hooks( itr->second );
//# Trigger a UIX down for this dock.
    if( auto imm = uix_imm_strong(); imm ) {
        //itr->second.ref->dock_uix_end();
        itr->second.ref->_uix_pack.reset();
    }
//# Drop the entry.
    dock_tbl->erase( itr );
    dock_tbl.release();
    spdlog::drop( std::string{ id_.data(), id_.size() } );

    BRIDGE_LOGI( "bridge: uninstalled dock: {}.", id_ );
    return OK;
}

rgh::HVec< Dock > dock_by_id( 
    IN   std::string_view    id_,
    IN   int                 tol_ = 0
) {
//# Acquire the lock over the dock table, search for the entry and return it.
    auto dock_tbl = _dock_tbl.control();
    ASSERT_OR( dock_tbl ) { BRIDGE_LOGE( "bridge: dock by id: bad table lock." ); return nullptr; }

    if( tol_ == 0 ) {
        auto dock_itr = dock_tbl->find( id_ ); 
        ASSERT_OR( dock_itr != dock_tbl->end() ) {
            BRIDGE_LOGE( "bridge: dock by id: no such dock: {}.", id_ );
            return nullptr;
        }
        return dock_itr->second.ref;
    } else {
        _dock_entry_t* min_dxen = nullptr;
        auto           min_dist = std::numeric_limits< int >::max();

        for( auto& [ id, dxen ] : *dock_tbl ) {
            const auto dist = rgh::lev_dist( id, id_ );
            if( dist > tol_ or dist > min_dist ) continue;

            min_dxen = &dxen;
            min_dist = dist;
        }

        return min_dxen ? min_dxen->ref : nullptr; 
    }
    std::unreachable();
}