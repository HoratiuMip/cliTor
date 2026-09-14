#pragma once
/**
 * @file: src/bridge.hpp
 * @brief: Bridge structure connecting the root machine with the proxys and docks.
 * @authors: Vatca "Mip" Tudor-Horatiu
 */

#include <rgh/gep/dispenser.hpp>
#include <rgh/gep/fastcli.hpp>
#include <rgh/gep/daemon.hpp>

#include <rgh/osp/core.hpp>
#include <rgh/osp/thread_pool.hpp>
#include <rgh/osp/immersive.hpp>

#include <spdlog/sinks/base_sink.h>

#include "directives/gateway_directive.hxx"
#include "directives/dock_directive.hxx"
#include "directives/proxy_directive.hxx"

class Bridge : public rgh::bridge_t, public rgh::Daemon, public rgh::Thread_pool,
               public Dock_Directive, public Proxy_Directive  
{
public: friend class Gateway;

public:
    struct start_args_t {
        int      argc   = 0;
        char**   argv   = nullptr;
        int      wcnt   = 4;
    };

protected:
    struct _config_t {
        bool   uix_bound   = false;
    };

public:
    Bridge( void ) : rgh::bridge_t{ "cliTor" } {
        logger->info( "bridge: init ok." );
    }

protected:
    _config_t   _config   = {};                                  

    struct _specs_tbl_t {
        rgh::HVec< Proxy >   cli   = nullptr;
    } _specs_tbl;

public:
    status_t decl_uix_bound( 
        IN   const uix_up_args_t&   args_ 
    ) {
        ASSERT_OR( daemon_is_started() ) {
            logger->error( "bridge: decl uix bound: bad callsite." );
            return ERR_CALLSITE;
        }

        logger->info( "bridge: decl uix bound: declared." );
        ASSERT_AND( not _config.uix_bound ) {
            _config.uix_bound = true;
            uix_up( args_ );
        } else {
            logger->warn( "bridge: decl uix bound: extra call, ignoring." );
        }

        return OK;
    }

#pragma region DAEMON
public:
    virtual std::string_view daemon_name() const override { return "cliTor bridge"; }
    virtual std::string daemon_report( [[maybe_unused]]void* ) const override {
        return std::format( 
            "/// cliTor bridge - " CLITOR_VERSION_STR "\n"
        );
    }

protected:
    virtual status_t _daemon_start(
        IN   void*   ctx_
    ) override {
    //# Check for valid context and get the daemon start arguments.
        ASSERT_OR( ctx_ ) {
            logger->error( "bridge: start: null context." );
            return ERR_BADARG;
        }
        auto* args = ( start_args_t* )ctx_;

    //# Launch the worker threads. Might pass this to the daemon wake function so
    //    the number of threads can be chosen via the CLI.
        ASSERT_STATUS_AND( rgh::Thread_pool::launch( args->wcnt ) ) {
            logger->info( "bridge: start: launched {} workers.", args->wcnt );
        } else {
            logger->warn( "bridge: start: bad workers ({}) launch.", args->wcnt );
        }

        logger->info( "bridge: started." );
        return OK;
    }

    virtual void _daemon_wake(
        IN   void*   ctx_
    ) {
    //# Get the daemon start arguments and acquire a watch on the proxy table.
    //# No need to check on ctx_ since _daemon_start() is chained before this function. 
        auto* args     = ( start_args_t* )ctx_;
        auto proxy_tbl = _proxy_tbl.watch();

    //# Wake all the preinstalled proxys.
        //for( auto& proxy : *proxy_tbl ) proxy.second->proxy_wake();

    //# Find the command line interpreter proxy and save it in the special proxys table.
    //# Execute each argument from the shell command line as a separate command if availble.
        {
            auto itr = proxy_tbl->find( "#cli" );    
            ASSERT_OR( itr != proxy_tbl->end() ) {
                logger->warn( "bridge: start: no CLI proxy found." );
                goto l_cli_end;
            }
            auto& cli = itr->second;
            ASSERT_OR( cli.ref ) {
                logger->error( "bridge: start: null CLI proxy." );
                goto l_cli_end;
            }
            _specs_tbl.cli = cli.ref;
            logger->info( "bridge: start: found the CLI proxy." );

            if( args->argc > 1 ) {
                for( int n = 1; n < args->argc; ++n ) {
                    cli->proxy_pass( args->argv[ n ] );
                }
                logger->info( "bridge: start: passed {} arguments to the CLI.", args->argc - 1 );
            } else {
                logger->info( "bridge: start: no arguments to pass to the CLI." );
            }

        } l_cli_end:

        return;
    }

    virtual status_t _daemon_stop(
        IN   void*   ctx_
    ) override {
    {
        auto dock_tbl = _dock_tbl.control(); ASSERT_AND( dock_tbl ) dock_tbl->clear();
        auto proxy_tbl = _proxy_tbl.control(); ASSERT_AND( proxy_tbl ) proxy_tbl->clear();
    }
        _specs_tbl.cli.reset();

    //# Kill the graphical user interface.
        uix_down();

        logger->info( "bridge: shutdown." );
        return OK;
    }
#pragma endregion DAEMON





#pragma region UIX
public:
    status_t resink_logger( 
        IN   spdlog::sink_ptr   sink_ 
    ) {
        rgh::BridgE.resink_logger( sink_ );
        rgh::bridge_t::resink_logger( sink_ );
        
        auto dock_tbl = _dock_tbl.control();
        ASSERT_OR( dock_tbl ) {
            return ERR_BUSY;
        }

        for( auto& entry : *dock_tbl ) {
            auto& dock_logger = entry.second.ref->_dock_logger;

            spdlog::drop( dock_logger->name() );
            dock_logger = std::make_shared< spdlog::logger >( dock_logger->name(), sink_ );
            dock_logger->set_pattern( RGH_SPDLOG_PATTERN );
        }

        return OK;
    }

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


public:
    void uix_up( 
        IN   const uix_up_args_t&   args_
    ) {
    //# Load or reload the UIX.
        ASSERT_OR( not _uix ) uix_down();
        _uix = std::make_shared< _uix_t >();

    //# Launch the UIX thread.
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

    void uix_down( void ) {
        auto uix = std::move( _uix );
        ASSERT_OR( uix ) return;
    
    //# Notify active docks to unload their UIX stuff.
        logger->info( "bridge: uix down: notifying docks..." );
        for( auto& [ id, dock ] : *_dock_tbl.control() ) {
            dock->dock_uix_end();
        }
        logger->info( "bridge: uix down: docks notified." );
        
        uix->imm->sig_main_exit();
    }

//# Place the dock in focus.
    status_t uix_focus(
        IN   const _dock_entry_t&   dken_
    ) {
    //# Assert that UIX is up.
        auto uix = _uix; ASSERT_OR( uix ) return ERR_NO_RESOLVE;
    //# Store the dock entry into the focus reference.
        uix->focus.store( &dken_, std::memory_order_relaxed );
        return OK;
    }
//# Place the dock indexed by the given ID in focus.
    status_t uix_focus(
        IN   const std::string&   id_
    ) {
    //# Acquire watch over the dock table and set the UIX focus.
        auto dock_tbl = _dock_tbl.watch(); ASSERT_OR( dock_tbl ) return ERR_BUSY;
    //# Index the dock.
        auto itr = dock_tbl->find( id_ ); ASSERT_OR( itr != dock_tbl->end() ) return ERR_NOT_FOUND;
    //# Place in focus.
        uix_focus( itr->second );
        return OK;
    }

//# Remove the currently focused or specified dock, if any.
    status_t uix_unfocus( 
        IN   const Dock*   dock_ = nullptr
    ) {
    //# Assert that UIX is up.
        auto uix = _uix; ASSERT_OR( uix ) return ERR_NO_RESOLVE;
    //# Check if the current focused dock is the requested one.
        const auto* crt_focus = _uix->focus.load( std::memory_order_relaxed );
        ASSERT_OR( crt_focus ) return OK;
        if( dock_ && dock_ != crt_focus->ref.get() ) return OK;
    //# Drop focus.
        _uix->focus.store( nullptr, std::memory_order_relaxed );
        return OK;
    }

    bool uix_is_up() { return _uix.use_count() > 0; }

    std::shared_ptr< rgh::Immersive > uix_imm_strong() { 
        auto uix = _uix; 
        ASSERT_OR( uix ) return nullptr;
        return uix->imm; 
    }

    rgh::Immersive* uix_imm_weak() { return _uix->imm.get(); }
    operator rgh::Immersive*() { return uix_imm_weak(); }

protected:
    status_t _uix_frame( const rgh::Immersive::frame_cb_args_t& args_ ) {
        _uix->imm->clear();

        const auto* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );

        const auto* focus = _uix->focus.load( std::memory_order_relaxed );
        if( not focus ) [[likely]] {
            ImGui::Begin( CLITOR_VERSION_STR, nullptr, 
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |  
                ImGuiWindowFlags_NoSavedSettings
            );

            if( ImGui::BeginTable( "##tbl-proxy-dock-split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV ) ) {
                ImGui::TableSetupColumn( "##proxy-zone", ImGuiTableColumnFlags_WidthFixed, 196 );
                ImGui::TableSetupColumn( "##dock-zone", ImGuiTableColumnFlags_WidthStretch );

                ImGui::TableNextColumn(); {
                    ImGui::Separator();

                    static constexpr const char* const ANIM_STRS[ 5 ] = {
                        "   (O)   ",
                        "  (( ))  ",
                        " ((   )) ",
                        "((  .  ))",
                        "(   o   )"
                    };
                    const char* crt_anim_str = ANIM_STRS[ static_cast< int >( args_.t*5 ) % 5 ];
                    ImGui::TextUnformatted( crt_anim_str, crt_anim_str+9 );

                    ImGui::SeparatorText( "Proxy Zone" );
                    
                    for( auto& [ id, pxen ] : *_proxy_tbl.watch() ) {
                        ASSERT_OR( not id.starts_with( "#" ) ) continue;

                        ImGui::PushID( &*id.cbegin(), &*id.cend() );
                            ImGuiTreeNodeFlags col_hdr_flags = ImGuiTreeNodeFlags_DefaultOpen;

                            const bool uix_frm_ovr = pxen->_static_fields.uix.has_basic_uix_frame_overridden;
                            if( not uix_frm_ovr ) col_hdr_flags |= ImGuiTreeNodeFlags_Bullet;

                            const bool proxy_header_expanded = ImGui::CollapsingHeader( id.c_str(), col_hdr_flags );
                            bool proxy_will_auto_install = ImGui::IsItemHovered() and ( ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) or rgh::Immersive::ctrl( ImGuiKey_T ) );
            
                            if( proxy_header_expanded and uix_frm_ovr ) {
                                pxen->proxy_uix_frame( { args_ } );
                            }

                            if( proxy_will_auto_install ) {
                                push( [ this, pxid = id ] { proxy_pass( pxid, "install" ); } );
                            }
                        ImGui::PopID();
                    }
                }

                ImGui::TableNextColumn(); {
                    auto dock_tbl = _dock_tbl.watch(); ASSERT_OR( dock_tbl ) return ERR_BUSY;

                    const ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll |
                                                           ImGuiTabBarFlags_AutoSelectNewTabs   |
                                                           ImGuiTabBarFlags_DrawSelectedOverline;

                    if( ImGui::BeginTabBar( "##tabs-dock", tab_bar_flags ) ) {
                        for( auto& [ id, dock ] : *dock_tbl ) {
                            ASSERT_OR( not id.starts_with( '#' ) ) continue;

                            ImGui::PushID( &*id.cbegin(), &*id.cend() );
                                bool tab_open = true;

                                const ImGuiTabItemFlags tab_item_flags = ImGuiTabItemFlags_None;

                                if( ImGui::BeginTabItem( _dock_id_c_str( id ), dock->dock_uix_persistent() ? nullptr : &tab_open, tab_item_flags ) ) {
                                    if( rgh::Immersive::was_dbl_clk() ) {
                                        uix_focus( dock );
                                    }

                                    ImGui::BeginChild( "##dock_frame", ImVec2{ 0, -ImGui::GetFrameHeightWithSpacing() }, ImGuiChildFlags_Border );
                                        dock->dock_uix_frame( { args_, dock->_uix_pack.get() } );
                                    ImGui::EndChild(); ImGui::EndTabItem();

                                    if( rgh::Immersive::ctrl( ImGuiKey_W ) ) tab_open = false;
                                }
                                
                                if( not tab_open ) { push( [ this, id ] { uninstall_dock( id ); } ); }
                            ImGui::PopID();
                        }

                        ImGui::EndTabBar();
                    }
                }
                ImGui::EndTable();
            }

            ImGui::Separator();
        } else {
            bool focused = true;
            
            auto dock_tbl = _dock_tbl.watch(); ASSERT_OR( dock_tbl ) return ERR_BUSY;
                 focus    = _uix->focus.load( std::memory_order_relaxed );

            ImGui::Begin( _dock_id_c_str( focus->ref->dock_id() ), &focused, 
                ImGuiWindowFlags_NoMove          |
                ImGuiWindowFlags_NoResize        |  
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoCollapse
            );

            focus->ref->dock_uix_frame( { args_, focus->ref->_uix_pack.get() } );

            if( not focused ) uix_unfocus();
        }

        ImGui::End();
        return daemon_is_started() ? OK : ERR_TERMINATED;
    }

#pragma endregion UIX
};
extern Bridge BridgE;

