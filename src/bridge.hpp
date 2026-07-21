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
    virtual std::string_view proxy_get_name( void ) const override { return JUNCTION_NAME; }

#define JUNCTION_DOCK_GET_ID_FNC_SIG \
    virtual std::string_view dock_get_id( void ) const noexcept

#define JUNCTION_DOCK_STOP_OR_BRIDGE_STOP \
    (this->dock_stop_signaled() or BridgE.status() != OK)

#define JUNCTION_DOCK_IS_UIX_PERSISTENT \
    virtual bool dock_uix_persistent( void ) const override { return true; }


typedef   rgh::status_t   status_t;

/**
 * @brief: Dock structure. This object handles everything needed for
 *           a specific communication test, such as a serial monitor.
 */
class Dock {
public: friend class Bridge;

public:
    /**
     * @brief: Immersion loop. Invoked repeatedly when the graphical user
     *           interface of the bridge is active.
     */
    virtual status_t dock_uix_frame( const rgh::Immersive::frame_cb_args_t& args_ ) { return OK; }

    /**
     * @brief: A UIX persistent dock will not have a close button.
     */
    virtual bool dock_uix_persistent( void ) const { return false; }
};

/**
 * @brief: Proxy structure. This object helps the Bridge to instantiate and
 *           talk to Docks.
 */
class Proxy {
public: friend class Bridge;

public: 
    /**
     * @brief: Get the name of the proxy. This is not unique, it is used to find
     *           and reference the Proxy in the Bridge proxy-registry.
     */
    virtual std::string_view proxy_get_name( void ) const = 0;
    /**
     * @brief: Called by the bridge after it starts.
     */
    virtual void proxy_wake( void ) { return; }
    /**
     * @brief: Pass a command line to the proxy. Minimally this must implement the
     *           install command for the docks.
     */
    virtual status_t proxy_pass( std::string line_ ) { return ERR_NOT_IMPL; };
};

class Bridge : public rgh::bridge_t, public rgh::Daemon {
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
    rgh::Thread_pool    _workers   = {};

protected:
    rgh::Dispenser< std::map< std::string, rgh::HVec< Proxy > > >   _proxys   = { rgh::DispenserMode_Lock };
    rgh::Dispenser< std::map< std::string, rgh::HVec< Dock > > >    _docks    = { rgh::DispenserMode_Lock };

protected:
    rgh::Immersive   _imm      = {};
    std::jthread     _uix_th   = {};

/**
 * @brief: Daemon overrides.
 */
public:
    virtual std::string_view daemon_name( void ) const override { return "cliTor bridge"; }
    virtual std::string daemon_report( [[maybe_unused]]void* ) const override {
        return std::format( 
            "/// cliTor bridge - " CLITOR_VERSION_STR "\n"
        );
    }

protected:
    virtual status_t _daemon_start(
        IN   void*   ctx_
    ) override {
        ASSERT_OR( ctx_ ) {
            logger->error( "bridge: start: null context." );
            return ERR_BADARG;
        }
        auto* args = ( start_args_t* )ctx_;

        ASSERT_STATUS_AND( _workers.launch( args->wcnt ) ) {
            logger->info( "bridge: start: launched {} workers.", args->wcnt );
        } else {
            logger->warn( "bridge: start: bad workers ({}) launch.", args->wcnt );
        }

        logger->info( "bridge: start ok." );
        return OK;
    }

    virtual void _daemon_wake(
        IN   void*   ctx_
    ) {
        auto* args = ( start_args_t* )ctx_;
        
        auto proxys = _proxys.watch();

        ASSERT_AND( args->argc > 1 ) {
            auto pitr  = proxys->find( "##cli" );
            ASSERT_OR( pitr != proxys->end() ) {
                logger->warn( "bridge: start: no CLI proxy to execute arguments." );
                goto l_cli_end;
            }

            auto cli_proxy = pitr->second;
            ASSERT_OR( cli_proxy ) {
                logger->error( "bridge: start: null CLI proxy." );
                goto l_cli_end;
            }

            for( int n = 1; n < args->argc; ++n ) {
                cli_proxy->proxy_pass( args->argv[ n ] );
            }
        } else {
            logger->info( "bridge: start: no arguments to execute." );
        }
    l_cli_end:

        for( auto& proxy : *proxys ) proxy.second->proxy_wake();
        return;
    }

    virtual status_t _daemon_stop(
        IN   void*   ctx_
    ) override {
        return OK;
    }

/**
 * @brief: Proxys.
 */
public:
    status_t install_proxy(
        IN   rgh::HVec< Proxy >&&   proxy_
    ) {
        ASSERT_OR( proxy_ ) {
            logger->error( "bridge: install proxy: null." );
            return ERR_BADARG;
        }
        
        auto pn = proxy_->proxy_get_name();
        ASSERT_OR( not pn.empty() ) {
            logger->error( "bridge: install proxy: empty name." );
            return ERR_BADARG;
        }

        auto proxys = _proxys.control();
        ASSERT_OR( proxys ) {
            logger->error( "bridge: install proxy (\"{}\"): bad registry control.", pn );
            return ERR_BUSY;
        }

        auto& proxy = ( *proxys )[ std::string{ pn } ];
        ASSERT_OR( not proxy ) {
            proxys.release();
            logger->error( "bridge: install proxy (\"{}\"): already exists.", pn );
            return ERR_WOULD_OVRWR; 
        }

        proxy = std::move( proxy_ );
        proxys.release();

        logger->info( "bridge: installed proxy: \"{}\".", pn );
        return OK;
    }

    status_t uninstall_proxy(
        IN   const std::string&   pn_
    ) {
        auto proxys = _proxys.control();
        ASSERT_OR( proxys ) {
            logger->error( "bridge: uninstall proxy (\"{}\"): bad registry control.", pn_ );
            return ERR_BUSY;
        }

        proxys->erase( pn_ );
        proxys.release();

        logger->info( "bridge: uninstalled proxy: \"{}\".", pn_ );
        return OK;
    }

/**
 * @brief: Docks.
 */
public:
    status_t install_dock(
        IN   std::string           did_,
        IN   rgh::HVec< Dock >&&   dock_
    ) {
        ASSERT_OR( dock_ ) {
            logger->error( "bridge: install dock: null." );
            return ERR_BADARG;
        }

        ASSERT_OR( not did_.empty() ) {
            logger->error( "bridge: install dock: empty name." );
            return ERR_BADARG;
        }

        auto docks = _docks.control();
        ASSERT_OR( docks ) {
            logger->error( "bridge: install dock (\"{}\"): bad registry control.", did_ );
            return ERR_BUSY;
        }

        auto& dock = ( *docks )[ did_ ];
        ASSERT_OR( not dock ) {
            docks.release();
            logger->error( "bridge: install dock (\"{}\"): already exists.", did_ );
            return ERR_WOULD_OVRWR; 
        }

        dock = std::move( dock_ );
        docks.release();

        logger->info( "bridge: installed dock: \"{}\".", did_ );
        return OK;
    }

    status_t uninstall_dock(
        IN   const std::string&   did_
    ) {
        auto docks = _docks.control();
        ASSERT_OR( docks ) {
            logger->error( "bridge: uninstall dock (\"{}\"): bad registry control.", did_ );
            return ERR_BUSY;
        }

        docks->erase( did_ );
        docks.release();

        logger->info( "bridge: uninstalled dock: \"{}\".", did_ );
        return OK;
    }

/**
 * @brief: Utility.
 */
public:
    inline auto push_task( auto tsk_ ) { return _workers.push( std::move( tsk_ ) ); }

/**
 * @brief: UIX.
 */
public:
    void start_uix( 
        IN   int                           width_,
        IN   int                           height_,
        IN   rgh::Immersive::SrfBeginAs_   bgnas_
    ) {
        this->stop_uix();

        _uix_th = std::jthread( &rgh::Immersive::main, &_imm, 0, nullptr, rgh::Immersive::config_t{
            .ctx        = nullptr,
            .title      = CLITOR_VERSION_STR,
            .width      = width_,
            .height     = height_,
            .srf_bgn_as = bgnas_,
            .init_cb    = [ this ] ( const auto& args_ ) -> auto {
            /* Cyberpunk theme from: https://github.com/ocornut/imgui/issues/707 */
#pragma region UIX_Theme
                ImGuiStyle& style = *_imm.imgui.stl;
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
                            
                _imm.imgui.io->FontGlobalScale = 1.22f;
                _imm->disengage_face_culling();   
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

    void stop_uix( void ) {
        ASSERT_AND( _uix_th.joinable() ) { _imm.sig_main_exit(); _uix_th.join(); }
    }

    inline bool uix_is_up( void ) { return _uix_th.joinable(); }

protected:
    RGH_inline status_t _uix_frame( const rgh::Immersive::frame_cb_args_t& args_ ) {
        _imm->clear();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );

        bool uix_open = true;
        ImGui::Begin( CLITOR_VERSION_STR, &uix_open, 
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
            
            int proxy_id = 0x0; for( auto& proxy : *_proxys.watch() ) {
                ASSERT_OR( not proxy.first.starts_with( "##" ) ) continue;

                ImGui::PushID( proxy_id );

                if( ImGui::Selectable( proxy.first.c_str() ) ) {

                }

                ImGui::PopID();
            }
    
            ImGui::TableNextColumn();

            if( auto docks = _docks.watch(); ImGui::BeginTabBar( "##docks", ImGuiTabBarFlags_None ) ) {
                int dock_id = 0x0; for( auto& dock : *docks ) {
                    ImGui::PushID( dock_id );

                    bool tab_open = true;
                    if( ImGui::BeginTabItem( dock.first.c_str(), dock.second->dock_uix_persistent() ? nullptr : &tab_open ) ) {
                        ImGui::BeginChild( "##dock_frame", ImVec2{ 0, -ImGui::GetFrameHeightWithSpacing() }, ImGuiChildFlags_Border );
                        dock.second->dock_uix_frame( args_ );
                        ImGui::EndChild(); ImGui::EndTabItem();
                    }
                    if( not tab_open ) this->push_task( [ this, dock_id = dock.first ] ( void ) -> void { this->uninstall_dock( dock_id ); } );

                    ImGui::PopID();
                }

                ImGui::EndTabBar();
            }

            ImGui::EndTable();
        }

        // --- 2. Bottom Command Bar ---
        ImGui::Separator(); // Horizontal line separating workspace from command line
        ImGui::Text("COMMAND"); 

        ImGui::End();
        return uix_open and this->daemon_is_started() ? OK : ERR_TERMINATED;
    }

};
extern Bridge BridgE;

