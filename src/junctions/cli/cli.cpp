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
        {},
        { 
{   .text = "exit",
    .opts = {},
    .fnc = [ this ] ( auto& C ) -> status_t {
        BridgE.daemon_stop();
        return OK;
    }
}, 

{   .text = "pxp",
    .opts = {
        { .sh0rt = 'n', .l0ng = "name", .arg = rgh::Fast_cli::Arg_text, .fast_id = 0x0 },
        { .sh0rt = 'l', .l0ng = "line", .arg = rgh::Fast_cli::Arg_text, .fast_id = 0x1 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string proxy_name = {};
        std::string line       = {};

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'n': proxy_name = C.text(); break;
            case 'l': line = C.text(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( !proxy_name.empty() && !line.empty() ) {
            JUNCTION_DOCK_LOGE( "proxy pass: incomplete arguments." );
            return ERR_PARTIAL;
        }

        BridgE.proxy_pass( proxy_name, std::move( line ) );
        return OK;
    }
},

{   .text = "cd",
    .opts = {
        { .sh0rt = 'i', .l0ng = "id", .arg = rgh::Fast_cli::Arg_text, .fast_id = 0x0 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        rgh::HVec< Dock > cd = nullptr;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'i': cd = BridgE.dock_by_id( C.text() ); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( cd ) {
            return ERR_NOT_FOUND;
        }

        _cd = std::move( cd );
        return OK;
    }
},

{   .text = "uix-up",
    .opts = {
        { .sh0rt = 'w', .l0ng = "width", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x0 },
        { .sh0rt = 'h', .l0ng = "height", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x1 },
        { .sh0rt = 'm', .l0ng = "minimize" },
        { .sh0rt = 'M', .l0ng = "maximize"}
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        int   width  = 680;
        int   height = 680;
        float font_scale = 1.22f;
        auto  bgnas  = rgh::Immersive::Default;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'w': width = C.i32(); break;
            case 'h': height = C.i32(); break;
            case 'f': font_scale = C.f32(); break;
            case 'm': bgnas = rgh::Immersive::Iconify; break;
            case 'M': bgnas = rgh::Immersive::Maximize; break;
        RGH_FASTCLI_OPT_SWITCH_END

        BridgE.uix_up( { width, height, font_scale, bgnas } );
        return OK;
    }
}
        }
    } {}

protected:
    rgh::Fast_cli       _cli      = {};
    std::jthread        _cin_th   = {};

    rgh::HVec< Dock >   _cd       = nullptr;

protected:
    void _cin_main( void ) {
        _rx.print( "\n" );

        std::string line = {}; 

        JUNCTION_DOCK_LOGI( "waiting for commands..." );

        while( BridgE.daemon_is_started() ) {
            auto cd = _cd;

            const char* line = _rx.input( std::format( 
                "\033[90m┌─────────────────────────────────────────────\n"
                "\033[90m[\033[33m{}\033[90m] \033[36m>>> \033[m"
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
    status_t execute(
        IN   std::string   line_
    ) {
        ASSERT_OR( not line_.empty() ) return ERR_NO_RESOLVE;

        auto cd       = _cd;
        bool under_cd = static_cast< bool >( cd );

        if( line_.starts_with( '\\' ) ) {
            if( line_.size() == 1uz ) {
                _cd.reset(); return OK;
            } else {
                line_.erase( 0uz, 1 );
                under_cd = false;
            }
        }

        switch( line_.at( 0x0uz ) ) {
            case '\\': 
                if( line_.size() == 1uz ) { _cd.reset(); return OK; }

                line_.erase( 0x0uz, 1 );
                under_cd = false;
            break;

            case '/':
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

    JUNCTION_PROXY_WAKE_FNC_SIG{
        BridgE.resink_logger( std::make_shared< replxx_sink_t >( _rx ) );

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
