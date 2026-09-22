#include "bridge.hpp"
Bridge __attribute__((init_priority(101))) BridgE;

int main( int argc_, char* argv_[] ) {
    rgh::init( argc_, argv_, rgh::init_args_t{
        .flags = rgh::InitFlags_None
    } );

    std::signal( SIGINT, [] ( int sig_ ) static -> void {
        BridgE.daemon_stop();
    } );

    BridgE.daemon_start( rgh::rval_addr< Bridge::start_args_t >( {
        .argc = argc_,
        .argv = argv_
    } ) );
    BridgE.daemon_wait_until( rgh::Daemon::State_STOPPED );

    return 0x0;
}
