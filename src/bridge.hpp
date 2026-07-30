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


#define CLITOR_VERSION_MAJOR 1
#define CLITOR_VERSION_MINOR 0
#define CLITOR_VERSION_PATCH 0
#define CLITOR_VERSION_STR "cliTor-v1.0.0"


#define JUNCTION_HEADER(namespace_name,junction_name) \
    static const char* const JUNCTION_NAME = junction_name; \
    namespace namespace_name {
#define JUNCTION_FOOTER \
    };

#define JUNCTION_PROXY_INSTALL(t, ...) \
    static struct _junction_proxy_installer_##t##_t_ { \
        _junction_proxy_installer_##t##_t_( void ) { \
            BridgE.install_proxy( rgh::HVec< t >::make( __VA_ARGS__ ) ); \
        } \
    } _junction_proxy_installer_##t##_; 

#define JUNCTION_PROXY_GET_NAME \
    virtual std::string_view proxy_get_name() const override { return JUNCTION_NAME; }
#define JUNCTION_PROXY_IS_DOCK \
    virtual Dock* proxy_as_dock() override { return static_cast< Dock* >( this ); }

#define JUNCTION_DOCK_GET_ID_FNC_SIG \
    virtual std::string_view dock_get_id() const noexcept

#define JUNCTION_DOCK_STOP_OR_BRIDGE_STOP \
    (this->dock_stop_signaled() or BridgE.status() != OK)

#define JUNCTION_DOCK_IS_UIX_PERSISTENT \
    virtual const bool dock_uix_persistent() const override { return true; }


typedef   rgh::status_t   status_t;

/*
# DETAILS: The dock is the tool itself.
*/
class Dock {
public: friend class Bridge;

public:
    class UIX_pack {
        public: virtual ~UIX_pack() = 0;
    };

public:
    struct dock_uix_frame_args_t : rgh::Immersive::frame_cb_args_t {
        UIX_pack*   pack   = nullptr;
    };

public:
    virtual std::unique_ptr< UIX_pack > dock_uix_begin     ()                                           { return nullptr; }
    virtual status_t                    dock_uix_frame     ( const dock_uix_frame_args_t& args_ )       { return OK; }
    virtual void                        dock_uix_end       ()                                           { return; }
    virtual const bool                  dock_uix_persistent()                                     const { return false; }
};

class Proxy {
public: friend class Bridge;

public: 
    virtual std::string_view proxy_get_name() const = 0;
    
    virtual void     proxy_wake()                    { return; }
    virtual status_t proxy_pass( std::string line_ ) { return ERR_NOT_IMPL; };

    virtual Dock* proxy_as_dock() { return nullptr; }
};

class Bridge : public rgh::bridge_t, public rgh::Daemon, public rgh::Thread_pool  {
public:
    struct start_args_t {
        int      argc   = 0;
        char**   argv   = nullptr;
        int      wcnt   = 4;
    };

public:
    Bridge( void ) : rgh::bridge_t{ CLITOR_VERSION_STR } {
        logger->info( "bridge: init ok." );
    }

protected:
    struct _proxy_entry_t {
        rgh::HVec< Proxy >   ref   = nullptr;

        auto operator->() const { return ref.operator->(); }
    };

    struct _dock_entry_t {
        struct uix_t {
            std::unique_ptr< Dock::UIX_pack >   pack   = nullptr;
        };

        rgh::HVec< Dock >   ref   = nullptr;
        uix_t               uix   = {};

        auto operator->() const { return ref.operator->(); }
    };

    rgh::Dispenser< std::map< std::string, _proxy_entry_t > >   _proxy_tbl   = { rgh::DispenserMode_Lock };
    rgh::Dispenser< std::map< std::string, _dock_entry_t > >    _dock_tbl    = { rgh::DispenserMode_Lock };

    struct _specprox_tbl_t {
        rgh::HVec< Proxy >   cli   = nullptr;
    } _specprox_tbl;

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
        ASSERT_STATUS_AND( this->rgh::Thread_pool::launch( args->wcnt ) ) {
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
        for( auto& proxy : *proxy_tbl ) proxy.second->proxy_wake();

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
            _specprox_tbl.cli = cli.ref;
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
    //# Kill the graphical user interface.
        uix_down();

        logger->info( "bridge: shutdown." );
        return OK;
    }
#pragma endregion DAEMON

#pragma region PROXY
public:
    status_t install_proxy(
        IN   rgh::HVec< Proxy >&&   proxy_
    ) {
    //# Checl for a valid proxy, i.e. valid pointer and non-empty name.
        ASSERT_OR( proxy_ ) {
            logger->error( "bridge: install proxy: null proxy." );
            return ERR_BADARG;
        }
        auto pn = proxy_->proxy_get_name();
        ASSERT_OR( not pn.empty() ) {
            logger->error( "bridge: install proxy: empty name." );
            return ERR_BADARG;
        }

    //# Acquire control over the proxy table and install it if it does not already exist.
        auto proxy_tbl = _proxy_tbl.control();
        ASSERT_OR( proxy_tbl ) {
            logger->error( "bridge: install proxy (\"{}\"): bad table control.", pn );
            return ERR_BUSY;
        }

        auto& proxy = ( *proxy_tbl )[ std::string{ pn } ];
        ASSERT_OR( not proxy.ref ) {
            proxy_tbl.release();
            logger->error( "bridge: install proxy (\"{}\"): already installed.", pn );
            return ERR_WOULD_OVRWR; 
        }

        proxy.ref = std::move( proxy_ );
        proxy_tbl.release();

        logger->info( "bridge: installed proxy: \"{}\".", pn );
        return OK;
    }

    status_t uninstall_proxy(
        IN   const std::string&   pn_
    ) {
    //# Acquire control over the proxy table and erase the matching entry.
        auto proxy_tbl = _proxy_tbl.control();
        
        ASSERT_OR( proxy_tbl ) {
            logger->error( "bridge: uninstall proxy (\"{}\"): bad table control.", pn_ );
            return ERR_BUSY;
        }

        proxy_tbl->erase( pn_ );
        proxy_tbl.release();

        logger->info( "bridge: uninstalled proxy: \"{}\".", pn_ );
        return OK;
    }
#pragma endregion PROXY

#pragma region DOCK
public:
    status_t install_dock(
        IN   std::string           did_,
        IN   rgh::HVec< Dock >&&   dock_
    ) {
    //# Check for a valid dock, i.e. valid pointer and non-empty ID.
        ASSERT_OR( dock_ ) {
            logger->error( "bridge: install dock: null dock." );
            return ERR_BADARG;
        }
        ASSERT_OR( not did_.empty() ) {
            logger->error( "bridge: install dock: empty name." );
            return ERR_BADARG;
        }

        auto dock_tbl = _dock_tbl.control();
        ASSERT_OR( dock_tbl ) {
            logger->error( "bridge: install dock (\"{}\"): bad table control.", did_ );
            return ERR_BUSY;
        }

        auto& dock = ( *dock_tbl )[ did_ ];
        ASSERT_OR( not dock.ref ) {
            dock_tbl.release();
            logger->error( "bridge: install dock (\"{}\"): already installed.", did_ );
            return ERR_WOULD_OVRWR; 
        }

        dock.ref = std::move( dock_ );
        dock_tbl.release();

        logger->info( "bridge: installed dock: \"{}\".", did_ );
        return OK;
    }

    status_t uninstall_dock(
        IN   const std::string&   id_
    ) {
    //# Acquire control over the dock table and obtain the entry.
        auto dock_tbl = _dock_tbl.control();
        ASSERT_OR( dock_tbl ) {
            logger->error( "bridge: uninstall dock (\"{}\"): bad table control.", id_ );
            return ERR_BUSY;
        }

        auto itr = dock_tbl->find( id_ );
        ASSERT_OR( itr != dock_tbl->end() ) return OK;
    
    //# Release any references pointing to this dock entry.
        if( _uix ) {
            if( _uix->focus == &itr->second ) _uix->focus = nullptr;
        }
        
    //# Erase the entry.
        dock_tbl->erase( itr );
        dock_tbl.release();

        logger->info( "bridge: uninstalled dock: \"{}\".", id_ );
        return OK;
    }
#pragma endregion DOCK

#pragma region UIX
protected:
    struct _uix_t {
        rgh::HVec< rgh::Immersive >   imm      = rgh::HVec< rgh::Immersive >::make();
        _dock_entry_t*                focus    = nullptr;
        std::jthread                  imm_th   = {};
    };
    std::unique_ptr< _uix_t >   _uix   = nullptr;

public:
    void uix_up( 
        IN   int                           width_,
        IN   int                           height_,
        IN   rgh::Immersive::SrfBeginAs_   bgnas_
    ) {
    //# Load or reload the UIX.
        ASSERT_OR( not uix_is_up() ) uix_down();
        _uix = std::make_unique< _uix_t >();

    //# Notify active docks to load their UIX stuff.
        for( auto& [ id, dock ] : *_dock_tbl.control() ) {
            dock.uix.pack = dock->dock_uix_begin();
        }

    //# Launch the UIX thread.
        _uix->imm_th = std::jthread( &rgh::Immersive::main, _uix->imm.get(), 0, nullptr, rgh::Immersive::config_t{
            .ctx        = nullptr,
            .title      = CLITOR_VERSION_STR,
            .width      = width_,
            .height     = height_,
            .srf_bgn_as = bgnas_,
            .init_cb    = [ this ] ( const auto& args_ ) -> auto {
/* Cyberpunk theme from: https://github.com/ocornut/imgui/issues/707 */
#pragma region UIX_Theme
                ImGuiStyle& style = *_uix->imm->imgui.stl;
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
#pragma endregion UIX_Theme        
                _uix->imm->imgui.io->FontGlobalScale = 1.22f;
                _uix->imm->disengage_face_culling();   
                return OK;
            },
            .loop_cb    = [ this ] ( const auto& args_ ) -> auto { 
                return this->_uix_frame( args_ );
            },
            .exit_cb    = [ this ] ( const auto& args_ ) -> auto { 
                return OK;
            }
        } );
    }

    void uix_down( void ) {
        ASSERT_OR( _uix ) return;
        
        _uix->imm->sig_main_exit();
        _uix.reset();
    }

    status_t uix_focus(
        IN   const std::string&   id_
    ) {
    //# Assert that UIX is up.
        ASSERT_OR( _uix ) return ERR_NO_RESOLVE;

    //# Acquire control over the dock table and set the UIX focus.
        auto dock_tbl = _dock_tbl.control();
        
        auto itr = dock_tbl->find( id_ );
        ASSERT_OR( itr != dock_tbl->end() ) return ERR_NOT_FOUND;
        
        _uix->focus = &itr->second;
        return OK;
    }

    bool uix_is_up( void ) { return (bool)_uix; }
    rgh::Immersive* uix_imm_weak( void ) { return _uix ? _uix->imm.get() : nullptr; }
    operator rgh::Immersive* ( void ) { return _uix ? _uix->imm.get() : nullptr; }

protected:
    RGH_inline status_t _uix_frame( const rgh::Immersive::frame_cb_args_t& args_ ) {
        _uix->imm->clear();

        const auto* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );

        if( not _uix->focus ) {
            ImGui::Begin( CLITOR_VERSION_STR, nullptr, 
                ImGuiWindowFlags_NoDecoration          |
                ImGuiWindowFlags_NoMove                |
                ImGuiWindowFlags_NoResize              |  
                ImGuiWindowFlags_NoSavedSettings       |
                ImGuiWindowFlags_NoBringToFrontOnFocus
            );

            if( ImGui::BeginTable( "##proxy-dock-split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV ) ) {
                ImGui::TableSetupColumn( "##proxys", ImGuiTableColumnFlags_WidthFixed, 150.0f );
                ImGui::TableSetupColumn( "##docks", ImGuiTableColumnFlags_WidthStretch );

                ImGui::TableNextColumn();
                
                ImGui::Text( "cliTor" );
                ImGui::Separator();
                
                int proxy_id = 0x0; for( auto& proxy : *_proxy_tbl.watch() ) {
                    ASSERT_OR( not proxy.first.starts_with( "#" ) ) continue;

                    ImGui::PushID( proxy_id );

                    if( ImGui::Selectable( proxy.first.c_str() ) ) {

                    }

                    ImGui::PopID();
                }
        
                ImGui::TableNextColumn();

                if( auto dock_tbl = _dock_tbl.watch(); ImGui::BeginTabBar( "##docks", ImGuiTabBarFlags_None ) ) {
                    int crtno = 0x0; for( auto& [ id, dock ] : *dock_tbl ) {
                        ImGui::PushID( crtno );

                        bool tab_open = true;
                        if( ImGui::BeginTabItem( id.c_str(), dock->dock_uix_persistent() ? nullptr : &tab_open ) ) {
                            ImGui::BeginChild( "##dock_frame", ImVec2{ 0, -ImGui::GetFrameHeightWithSpacing() }, ImGuiChildFlags_Border );
                                dock->dock_uix_frame( { args_, dock.uix.pack.get() } );
                            ImGui::EndChild(); ImGui::EndTabItem();
                        }
                        if( not tab_open ) this->push( [ this, id ] { this->uninstall_dock( id ); } );

                        ImGui::PopID();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndTable();
            }

            ImGui::Separator();
            ImGui::Text( "COMMAND" ); 
        } else {
            bool focused = true;

            ImGui::Begin( CLITOR_VERSION_STR, &focused, 
                ImGuiWindowFlags_NoMove                |
                ImGuiWindowFlags_NoResize              |  
                ImGuiWindowFlags_NoSavedSettings       |
                ImGuiWindowFlags_NoBringToFrontOnFocus
            );

            auto dock_tbl = _dock_tbl.watch();
            _uix->focus->ref->dock_uix_frame( { args_, _uix->focus->uix.pack.get() } );
        }

        ImGui::End();
        return this->daemon_is_started() ? OK : ERR_TERMINATED;
    }

#pragma endregion UIX
};
extern Bridge BridgE;

