#pragma once /*
# FILE: directives/bridge_directive.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: General.
*/
#include "descriptor_directive.hxx"
#include <rgh/brp/descriptor.hpp>

CLITOR_NAMESPACE {

#define BRIDGE_LOGI( fmt, ... ) clitor::BridgE.logger->info( "bridge: " fmt __VA_OPT__(,) __VA_ARGS__ )
#define BRIDGE_LOGW( fmt, ... ) clitor::BridgE.logger->warn( "bridge: " fmt __VA_OPT__(,) __VA_ARGS__ )
#define BRIDGE_LOGE( fmt, ... ) clitor::BridgE.logger->error( "bridge: " fmt __VA_OPT__(,) __VA_ARGS__ )

};

/// Use this macro after your includes in your main junction file.
/// Do not forget to #include <clitor.hxx>
///
/// @example #include <clitor.hxx>
///          JUNCTION_HEADER( my_app, "MyApp" )
/// ---
#define JUNCTION_HEADER(namespace_name,junction_name) \
    static constexpr std::string_view JUNCTION_NAME = junction_name; \
    namespace namespace_name {

/// Use this macro at the end of your main junction file.
/// ---
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

#include <rgh/gep/dispenser.hpp>
#include <rgh/gep/fastcli.hpp>
#include <rgh/gep/daemon.hpp>

#include <rgh/osp/core.hpp>
#include <rgh/osp/thread_pool.hpp>
#include <rgh/osp/immersive.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

#include "directives/dock_directive.hxx"
#include "directives/proxy_directive.hxx"
#include "directives/uix_directive.hxx"

CLITOR_NAMESPACE {

class Bridge : public rgh::Bridge, public rgh::Daemon, public rgh::Thread_pool,
               public Dock_Directive, public Proxy_Directive, public UIX_Directive
{
public: _DIRECTIVE_FRIENDS

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
    Bridge( void ) : rgh::Bridge{ "cliTor" } {
        logger->info( "bridge: init ok." );
    }

protected:
    _config_t   _config   = {};                                  

    struct _specs_tbl_t {
        rgh::HVec< Proxy >   cli   = nullptr;
    } _specs_tbl;

#pragma region DAEMON
public:
    virtual std::string_view daemon_name() const override { return "cliTor bridge"; }
    virtual std::string daemon_report( [[maybe_unused]]void* ) const override {
        return std::format( 
            "/// cliTor bridge - " CLITOR_VERSION_STR "\n"
        );
    }

protected:
    virtual rgh::ret_t _daemon_start(
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

    virtual rgh::ret_t _daemon_start_proc(
        IN   void*   ctx_
    ) override {
    //# Get the daemon start arguments and acquire a watch on the proxy table.
    //# No need to check on ctx_ since _daemon_start() is chained before this function. 
        auto* args     = ( start_args_t* )ctx_;
        auto proxy_tbl = _proxy_tbl.watch();

    //# Wake all the preinstalled proxys.
        for( auto& proxy : *proxy_tbl ) (void)proxy.second->proxy_init_proc( ctx_ );

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

        return OK;
    }

    virtual rgh::ret_t _daemon_stop(
        IN   void*   ctx_
    ) override {
        logger->info( "bridge: stop: triggered." );
    {
        auto dock_tbl = _dock_tbl.control(); ASSERT_AND( dock_tbl ) dock_tbl->clear();
        logger->info( "bridge: stop: released docks." );
        auto proxy_tbl = _proxy_tbl.control(); ASSERT_AND( proxy_tbl ) proxy_tbl->clear();
        logger->info( "bridge: stop: released proxys." );
    }

    //# Kill the graphical user interface.
        uix_down(); logger->info( "bridge: stop: UIX killed." );

        logger->info( "bridge: stop: shutdown." );
        return OK;
    }
#pragma endregion DAEMON





#pragma region UIX
public:
    rgh::ret_t resink_logger( 
        IN   spdlog::sink_ptr   sink_ 
    ) {
        rgh::BridgE.resink_logger( sink_ );
        rgh::Bridge::resink_logger( sink_ );
        
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

public:


public:


#pragma endregion UIX
};
extern Bridge BridgE;

}//#CLITOR_NAMESPACE

CLITOR_NAMESPACE {

#define DIRECTIVE_BRIDGE_SELF_SHIFT auto& DBSS = static_cast< Bridge& >( *this );  

}//# CLITOR_NAMESPACE
    