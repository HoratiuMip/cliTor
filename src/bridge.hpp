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
#define JUNCTION_PROXY_PASS_FNC_SIG \
    virtual status_t proxy_pass( std::string line_ ) override
#define JUNCTION_PROXY_WAKE_FNC_SIG \
    virtual void proxy_wake() override
#define JUNCTION_PROXY_PASS_BASIC_DOCK_INSTALL( dock_t_ ) \
    case rgh::txt_hash( "install" ): { \
        static int _next_dock_id = 0; \
        BridgE.install_dock( std::format( "{}-{}", JUNCTION_NAME, _next_dock_id++ ), rgh::HVec< dock_t_ >::make() ); \
        break; }

#define JUNCTION_DOCK_GET_ID_FNC_SIG \
    virtual std::string_view dock_get_id() const noexcept
#define JUNCTION_DOCK_PASS_FNC_SIG \
    virtual status_t dock_pass( std::string line_ ) override 
#define JUNCTION_DOCK_UIX_BEGIN_FNC_SIG \
    virtual std::shared_ptr< ::Dock::UIX_pack > dock_uix_begin() override
#define JUNCTION_DOCK_UIX_FRAME_FNC_SIG \
    virtual status_t dock_uix_frame( \
        IN   const dock_uix_frame_args_t&   args_ \
    ) override
#define JUNCTION_DOCK_UIX_END_FNC_SIG \
    virtual void dock_uix_end() override

#define JUNCTION_DOCK_LOGI( ... ) _dock_logger->info( __VA_ARGS__ )
#define JUNCTION_DOCK_LOGW( ... ) _dock_logger->warn( __VA_ARGS__ )
#define JUNCTION_DOCK_LOGE( ... ) _dock_logger->error( __VA_ARGS__ )

#define JUNCTION_DOCK_UIX_REINTR_PACK( pack_t_ ) auto* pack = reinterpret_cast< pack_t_* >( args_.pack );

#define JUNCTION_DOCK_STOP_OR_BRIDGE_STOP \
    (this->dock_stop_signaled() or BridgE.status() != OK)

#define JUNCTION_DOCK_IS_UIX_PERSISTENT \
    virtual const bool dock_uix_persistent() const override { return true; }

#define JUNCTION_DOCK_WITH_BRIDGE_IMM_AND_UIX_PACK( pack_t_ ) \
    auto imm = BridgE.uix_imm_strong(); auto _raw_uix_pack = this->_uix_pack; auto uix_pack = reinterpret_cast< pack_t_* >( _raw_uix_pack.get() ); \
    ASSERT_AND( imm and uix_pack )


typedef   rgh::status_t   status_t;


class Dock {
public: friend class Bridge;

protected:
    std::shared_ptr< spdlog::logger >   _dock_logger   = nullptr;
    std::string                         _dock_id       = {};

public:
    std::string dock_id( void ) const { return _dock_id; }
    virtual status_t dock_pass( std::string line_ ) { return ERR_NOT_IMPL; }

public:
    class UIX_pack {
        public: virtual ~UIX_pack() = default;
    };

    struct dock_uix_frame_args_t : rgh::Immersive::frame_cb_args_t {
        UIX_pack*   pack   = nullptr;
    };

public:
    virtual std::shared_ptr< UIX_pack > dock_uix_begin() { return nullptr; }
    virtual status_t dock_uix_frame( const dock_uix_frame_args_t& args_ ) { return ERR_NOT_IMPL; }
    virtual void  dock_uix_end() { return; }
    virtual const bool dock_uix_persistent() const { return false; }

protected:
    std::shared_ptr< UIX_pack >   _uix_pack   = nullptr;

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

    struct uix_up_args_t {
        int                             width        = 512;
        int                             height       = 256;
        float                           font_scale   = 1.22f;
        rgh::Immersive::Word_           bgnas        = rgh::Immersive::Default;
        std::function< void( void ) >   styler       = &Bridge::uix_styler_dark_cyberpunk;
    };

protected:
    struct _config_t {
        bool   uix_bound   = false;
    };

    struct _proxy_entry_t {
        rgh::HVec< Proxy >   ref   = nullptr;

        auto operator->() const { return ref.operator->(); }
    };

    struct _dock_entry_t {
        rgh::HVec< Dock >   ref   = nullptr;

        auto operator->() const { return ref.operator->(); }
    };
  
public:
    Bridge( void ) : rgh::bridge_t{ "cliTor" } {
        logger->info( "bridge: init ok." );
    }

protected:
    _config_t   _config   = {};                                  

    rgh::Dispenser< std::map< std::string, _proxy_entry_t > >   _proxy_tbl   = { rgh::DispenserMode_Lock };
    rgh::Dispenser< std::map< std::string, _dock_entry_t > >    _dock_tbl    = { rgh::DispenserMode_Lock };

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
        if( auto dock = proxy.ref->proxy_as_dock(); dock ) {
            this->install_dock( proxy.ref->proxy_get_name().cbegin(), rgh::HVec< Dock >{ rgh::hvec_weak_ptr_t{ dock } } );
        }

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
            logger->error( "bridge: uninstall proxy ({}): bad table control.", pn_ );
            return ERR_BUSY;
        }

        proxy_tbl->erase( pn_ );
        proxy_tbl.release();

        logger->info( "bridge: uninstalled proxy: \"{}\".", pn_ );
        return OK;
    }

    status_t proxy_pass(
        IN   const std::string&   pn_,
        IN   std::string          line_
    ) {
        auto proxy_tbl = _proxy_tbl.control();
        ASSERT_OR( proxy_tbl ) {
            logger->error( "bridge: proxy pass ({}): bad table lock.", pn_ );
            return ERR_BUSY;
        }

        auto proxy = proxy_tbl->find( pn_ ); 
        ASSERT_OR( proxy != proxy_tbl->end() ) {
            logger->error( "bridge: proxy pass: no such proxy: {}.", pn_ );
            return ERR_NOT_FOUND;
        }

        return proxy->second.ref->proxy_pass( std::move( line_ ) );
    }
#pragma endregion PROXY

#pragma region DOCK
protected:
    void _dock_entry_set_id(
        IN   _dock_entry_t&   dken_,
        IN   std::string            id_
    ) {
        Dock& dock = *dken_.ref;

        dock._dock_id = std::move( id_ ); 
    }

    void _dock_entry_make_logger(
        IN   _dock_entry_t&   dken_
    ) {
        Dock& dock = *dken_.ref;
 
        dock._dock_logger = std::make_shared< spdlog::logger >( _dock_id_c_str( dock._dock_id ), this->get_logger_sink() );
        dock._dock_logger->set_pattern( RGH_SPDLOG_PATTERN );
    }

    void _dock_entry_load_uix(
        IN   _dock_entry_t&   dken_
    ) {
        auto imm = uix_imm_strong();
        ASSERT_OR( imm ) return;
        
        Dock& dock = *dken_.ref;

        dock._uix_pack = dock.dock_uix_begin();
    }

    const char* _dock_id_c_str(
        IN   const std::string&   id_
    ) {
        const int forced_ord_id_offset = id_.starts_with( '/' ) ? 2 : 0;
        return &id_[ forced_ord_id_offset ];
    }

public:
//# Install a dock in the bridge with the given ID. 
//# Note that IDs must be unique.
    status_t install_dock(
        IN   std::string           id_,
        IN   rgh::HVec< Dock >&&   dock_
    ) {
    //# Check for a valid dock, i.e. valid pointer and non-empty ID.
        ASSERT_OR( dock_ ) {
            logger->error( "bridge: install dock: null dock." );
            return ERR_BADARG;
        }
        ASSERT_OR( not id_.empty() ) {
            logger->error( "bridge: install dock: empty name." );
            return ERR_BADARG;
        }

        auto dock_tbl = _dock_tbl.control();
        ASSERT_OR( dock_tbl ) {
            logger->error( "bridge: install dock (\"{}\"): bad table control.", id_ );
            return ERR_BUSY;
        }

        auto& dock = ( *dock_tbl )[ id_ ];
        ASSERT_OR( not dock.ref ) {
            dock_tbl.release();
            logger->error( "bridge: install dock (\"{}\"): already installed.", id_ );
            return ERR_WOULD_OVRWR; 
        }
        
        dock.ref = std::move( dock_ );
        _dock_entry_set_id( dock, std::move( id_ ) );
        _dock_entry_make_logger( dock );
        _dock_entry_load_uix( dock );

        dock_tbl.release();

        logger->info( "bridge: installed dock: \"{}\".", dock.ref->_dock_id );
        return OK;
    }

//# Uninstall a dock from the bridge.
    status_t uninstall_dock(
        IN   const std::string&   id_
    ) {
    //# Acquire the dock table and obtain the entry.
        auto dock_tbl = _dock_tbl.control();
        ASSERT_OR( dock_tbl ) {
            logger->error( "bridge: uninstall dock (\"{}\"): bad table lock.", id_ );
            return ERR_BUSY;
        }

        auto itr = dock_tbl->find( id_ );
        ASSERT_OR( itr != dock_tbl->end() ) return OK;
    
    //# Release any references pointing to this dock entry.
        uix_unfocus( itr->first );

        if( auto imm = uix_imm_strong(); imm ) {
            itr->second.ref->dock_uix_end();
            itr->second.ref->_uix_pack.reset();
        }
        
    //# Erase the entry.
        dock_tbl->erase( itr );
        dock_tbl.release();

        spdlog::drop( id_ );

        logger->info( "bridge: uninstalled dock: \"{}\".", id_ );
        return OK;
    }

//# Retrieve a strong reference to the dock having the given ID.
    rgh::HVec< Dock > dock_by_id( 
        IN   const std::string&   id_
    ) {
    //# Acquire the lock over the dock table, search for the entry and return it.
        auto dock_tbl = _dock_tbl.control();
        ASSERT_OR( dock_tbl ) {
            logger->error( "bridge: dock by id: bad table lock." );
            return nullptr;
        }

        auto dock_itr = dock_tbl->find( id_ ); 
        ASSERT_OR( dock_itr != dock_tbl->end() ) {
            logger->error( "bridge: dock by id: no such dock: {}.", id_ );
            return nullptr;
        }

        return dock_itr->second.ref;
    }

#pragma endregion DOCK

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
    struct _uix_t {
        std::shared_ptr< rgh::Immersive >               imm      = std::make_shared< rgh::Immersive >();
        std::pair< std::string_view, _dock_entry_t* >   focus    = {};
        std::jthread                                    imm_th   = {};
    };
    std::shared_ptr< _uix_t >   _uix   = nullptr;

public:
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
            .title      = CLITOR_VERSION_STR,
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
                return this->_uix_frame( args_ );
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

    status_t uix_focus(
        IN   const std::string&   id_
    ) {
    //# Assert that UIX is up.
        auto uix = _uix; ASSERT_OR( uix ) return ERR_NO_RESOLVE;

    //# Acquire control over the dock table and set the UIX focus.
        auto dock_tbl = _dock_tbl.control();
        
        auto itr = dock_tbl->find( id_ );
        ASSERT_OR( itr != dock_tbl->end() ) return ERR_NOT_FOUND;
        
        uix->focus = { itr->first, &itr->second };
        return OK;
    }

    status_t uix_unfocus(
        IN   std::string_view   id_   = ""
    ) {
    //# Assert that UIX is up.
        auto uix = _uix; ASSERT_OR( uix ) return ERR_NO_RESOLVE;

        if( id_.empty() || id_ == _uix->focus.first ) _uix->focus = { {}, nullptr };
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

        if( not _uix->focus.second ) {
            ImGui::Begin( CLITOR_VERSION_STR, nullptr, 
                ImGuiWindowFlags_NoDecoration          |
                ImGuiWindowFlags_NoMove                |
                ImGuiWindowFlags_NoResize              |  
                ImGuiWindowFlags_NoSavedSettings
            );

            if( ImGui::BeginTable( "##tbl-proxy-dock-split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV ) ) {
                ImGui::TableSetupColumn( "##proxy-zone", ImGuiTableColumnFlags_WidthFixed, 150.0f );
                ImGui::TableSetupColumn( "##dock-zone", ImGuiTableColumnFlags_WidthStretch );

                ImGui::TableNextColumn();
                
                ImGui::Text( "cliTor" );
                ImGui::Separator();
                
                int proxy_id = 0x0; for( auto& proxy : *_proxy_tbl.watch() ) {
                    ASSERT_OR( not proxy.first.starts_with( "#" ) ) continue;

                    ImGui::PushID( proxy_id );

                    if( ImGui::Selectable( proxy.first.c_str() ) ) {
                        push( [ this, pn = proxy.first ] {
                            proxy_pass( pn, "install" );
                        } );
                    }

                    ImGui::PopID();
                }
        
                ImGui::TableNextColumn();

                if( auto dock_tbl = _dock_tbl.watch(); ImGui::BeginTabBar( "##tabs-dock", ImGuiTabBarFlags_FittingPolicyScroll ) ) {
                    int  crtno = 0x0; 

                    for( auto& [ id, dock ] : *dock_tbl ) {
                        ASSERT_OR( not id.starts_with( '#' ) ) continue;
                        ImGui::PushID( crtno );

                        bool tab_open = true;
                        if( ImGui::BeginTabItem( _dock_id_c_str( id ), dock->dock_uix_persistent() ? nullptr : &tab_open ) ) {
                            if( rgh::Immersive::was_dbl_clk() ) {
                                _uix->focus = { id, const_cast< _dock_entry_t* >( &dock ) };
                            }

                            ImGui::BeginChild( "##dock_frame", ImVec2{ 0, -ImGui::GetFrameHeightWithSpacing() }, ImGuiChildFlags_Border );
                                dock->dock_uix_frame( { args_, dock->_uix_pack.get() } );
                            ImGui::EndChild(); ImGui::EndTabItem();

                            tab_open = !rgh::Immersive::ctrl( ImGuiKey_W );
                        }
                        
                        if( not tab_open ) { push( [ this, id ] { uninstall_dock( id ); } ); }
                        ImGui::PopID();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndTable();
            }

            ImGui::Separator();
        } else {
            bool focused = true;
            auto focus   = _uix->focus;

            ImGui::Begin( focus.first.cbegin(), &focused, 
                ImGuiWindowFlags_NoMove          |
                ImGuiWindowFlags_NoResize        |  
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoCollapse
            );

            auto dock_tbl = _dock_tbl.watch();
            focus.second->ref->dock_uix_frame( { args_, focus.second->ref->_uix_pack.get() } );

            if( not focused ) _uix->focus = { {}, nullptr };
        }

        ImGui::End();
        return this->daemon_is_started() ? OK : ERR_TERMINATED;
    }

#pragma endregion UIX
};
extern Bridge BridgE;

