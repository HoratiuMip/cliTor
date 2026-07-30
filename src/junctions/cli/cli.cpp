#include <iostream>
#include <print>

#include <bridge.hpp>
JUNCTION_HEADER( cli, "#cli" )

class Cli : public Dock, public Proxy {
public:
    Cli( void )
    : _cli{
        {},
        { 
{   .text = "progctl",
    .opts = {
        { .sh0rt = 'h', .l0ng = "help" },
        { .sh0rt = 'v', .l0ng = "version" },
        { .sh0rt = 'e', .l0ng = "exit" }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'h': {
                
                break; }
            case 'v': {
                C( "cliTor version: {}", CLITOR_VERSION_STR );
                break; }
            case 'e': {
                C( "Stopping bridge..." );
                BridgE.daemon_stop();
                return OK;
            }
        RGH_FASTCLI_OPT_SWITCH_END
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
        int  width  = 680;
        int  height = 680;
        auto bgnas  = rgh::Immersive::SrfBeginAs_Default;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'w': width = C.i32(); break;
            case 'h': height = C.i32(); break;
            case 'm': bgnas = rgh::Immersive::SrfBeginAs_Iconify; break;
            case 'M': bgnas = rgh::Immersive::SrfBeginAs_Maximize; break;
        RGH_FASTCLI_OPT_SWITCH_END

        BridgE.uix_up( width, height, bgnas );
        return OK;
    }
}
        }
    } {}

protected:
    rgh::Fast_cli   _cli      = {};
    std::jthread    _cin_th   = {};

protected:
    void _cin_main( void ) {
        std::string line = {}; 

        BridgE->info( "cli: waiting for commands..." );
        while( BridgE.daemon_is_started() ) {
            std::print( ">>> " );
            ASSERT_OR( std::getline( std::cin, line ) ) {
                BridgE->error( "cli: bad line read." );
                std::cin.clear();
                continue; 
            }
            this->execute_and_print( std::move( line ) );
        }
    }

public:
    inline status_t execute( 
        IN    std::string    line_, 
        OUT   std::string*   out_
    ) {
        return _cli.execute( line_, out_ ); 
    }

    inline status_t execute_and_print(
        IN   std::string   line_
    ) {
        std::string out = {};
        ASSERT_STATUS_AND( this->execute( line_, &out ) ) {
            BridgE->info( "cli: \"{}\":\n{}", line_, out ); return status_;
        } else {
            BridgE->error( "cli: \"{}\":\n{}", line_, out ); return status_;
        }
        std::unreachable();
    }

public:
    JUNCTION_PROXY_GET_NAME
    JUNCTION_PROXY_IS_DOCK

    virtual void proxy_wake( void ) override {
        _cin_th = std::jthread( &Cli::_cin_main, this );
    }

    virtual status_t proxy_pass( 
        IN   std::string    line_ 
    ) override {
        return this->execute_and_print( line_ );
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
