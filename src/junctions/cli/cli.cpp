#include <iostream>
#include <print>

#include <replxx.hxx>

#include <bridge.hpp>
JUNCTION_HEADER( cli, "#cli" )

static const std::map< spdlog::level::level_enum, std::string > SPDLOG_LEVEL_TO_ANSI_COLOR_MAP = {
    { spdlog::level::trace,    "\033[37m" },
    { spdlog::level::debug,    "\033[36m" },
    { spdlog::level::info,     "\033[32m" },
    { spdlog::level::warn,     "\033[33m\033[1m" },
    { spdlog::level::err,      "\033[31m\033[1m" },
    { spdlog::level::critical, "\033[1m\033[41m" },
};

struct replxx_sink_t : public spdlog::sinks::base_sink< std::mutex > {
    explicit replxx_sink_t( replxx::Replxx& rx_ ) : _rx{ rx_ } {}

    void sink_it_(
        IN   const spdlog::details::log_msg&   msg_
    ) override {
        spdlog::memory_buf_t formatted;
        formatter_->format( msg_, formatted );

        std::string text = fmt::to_string( formatted );

        if( msg_.color_range_end > msg_.color_range_start ) {
            text.insert( msg_.color_range_end, "\033[m" );
            text.insert( msg_.color_range_start, SPDLOG_LEVEL_TO_ANSI_COLOR_MAP.at( msg_.level ) );
        }

        _rx.print( "%s", text.c_str() );
    }

    void flush_( void ) override {}

protected:
    replxx::Replxx&   _rx;
};
static replxx::Replxx _rx = {};

class Cli : public Dock, public Proxy {
public:
    Cli( void )
    : _cli{
        {
            .when_man = [ this ] ( std::string_view cmd_, std::string_view man_ ) { 
                JUNCTION_DOCK_LOGI( "manual for: {}:\n{}", cmd_, man_ ); 
            }
        },
        { 
            #include "cmds/clear.inl"
            , 
            #include "cmds/exit.inl"
            , 

            #include "cmds/pxp.inl"
            ,
            #include "cmds/install.inl"
            ,
            #include "cmds/uninstall.inl"
            ,
            #include "cmds/cd.inl"
            ,

            #include "cmds/uix_up.inl"
            ,
            #include "cmds/uix_down.inl"
        }
    } {}

    ~Cli( void ) {
        _rx.emulate_key_press( replxx::Replxx::KEY::control( 'C' ) );
    }

protected:
    rgh::Fast_cli       _cli      = {};
    std::jthread        _cin_th   = {};

    rgh::HVec< Dock >   _cd       = nullptr;

protected:
    void _cin_main( void ) {
        std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
        BridgE.resink_logger( std::make_shared< replxx_sink_t >( _rx ) );

        _rx.bind_key( replxx::Replxx::KEY::control( 'C' ), [ this ] ( char32_t code_ ) {
            return replxx::Replxx::ACTION_RESULT::BAIL;
        } );
        _rx.bind_key( replxx::Replxx::KEY::control( 'W' ), [ this ] ( char32_t code_ ) {
            _rx.print( "\n" );
            auto cd = std::move( _cd );
            ASSERT_AND( cd ) BridgE.uninstall_dock( cd->dock_id() );
            return replxx::Replxx::ACTION_RESULT::RETURN;
        } );

        //this->clear();

        std::string line = {}; 
        while( BridgE.daemon_is_started() ) {
            auto cd = _cd;

            const char* line = _rx.input( std::format( 
                "\033[90m┌───────────────────────────────────────────────────┘\n"
                "\033[90m└─ \033[33m{}\033[90m ── \033[36m>>> \033[m"
            ,
                cd ? cd->dock_id() : "BridgE"
            ) );

            ASSERT_OR( line ) break;
            ASSERT_OR( *line != '\0' ) continue;

            _rx.history_add( line );
            this->execute( line );
        }

        JUNCTION_DOCK_LOGW( "command loop terminated." );
    }

public:
    void clear( void ) {
        #include "logo.inl"
    #ifdef RGH_TARGET_OS_LINUX
        std::system( "clear" );
    #elifdef RGH_TARGET_OS_WINDOWS
        std::system( "cls" );
    #endif
        _rx.print( LOGO );
    }

    status_t execute(
        IN   std::string   line_
    ) {
        ASSERT_OR( not line_.empty() ) return ERR_NO_RESOLVE;

        auto cd       = _cd;
        bool under_cd = static_cast< bool >( cd );

        if( line_.starts_with( '/' ) ) {
            if( line_.size() == 1uz ) {
                _cd.reset(); return OK;
            } else {
                line_.erase( 0uz, 1 );
                under_cd = false;
            }
        }

        switch( line_.at( 0x0uz ) ) {
            case '/': 
                if( line_.size() == 1uz ) { _cd.reset(); return OK; }

                line_.erase( 0x0uz, 1 );
                under_cd = false;
            break;

            case '\\':
                std::system( line_.c_str() + 1 );
                return OK;
        }

        std::string out    = {};
        status_t    status = under_cd ? _cd->dock_pass( line_ ) : _cli.execute( line_, &out );

        return status;
    }

public:
    JUNCTION_PROXY_GET_NAME
    JUNCTION_PROXY_IS_DOCK

    JUNCTION_PROXY_WAKE_FNC_SIG {
        _cin_th = std::jthread( &Cli::_cin_main, this );
    }

    JUNCTION_PROXY_PASS_FNC_SIG {
        return this->execute( line_ );
    }

protected:
    struct _uix_t {
        std::string                     cin   = {};
        rgh::Dispenser< std::string >   cout  = { rgh::DispenserMode_Trylock };
    } _uix;

public:
    virtual status_t dock_uix_frame( 
        IN   const dock_uix_frame_args_t&   args_ 
    ) override {
        return OK;
    }

};

JUNCTION_PROXY_INSTALL( Cli )
JUNCTION_FOOTER
