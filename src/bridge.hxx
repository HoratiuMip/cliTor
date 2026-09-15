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


#pragma endregion UIX
};
extern Bridge BridgE;

