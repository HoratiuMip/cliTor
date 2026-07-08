#include <rgh/osp/IO_serial.hpp>
using namespace rgh;

int main( int argc_, char* argv_[] ) {
    auto ser = io::Serial{ "/dev/ttyUSB0", {
        .baud_rate = 115200
    } };

    while( true ) {
        if( int bca = ser.rx_available(); bca > 0 ) {
            char msg[ bca + 1 ];
            ser.read( {
                .dst_ptr = msg,
                .dst_n   = bca
            } );
            msg[ bca ] = '\0';

            spdlog::info( "rx: {}", (const char* )msg );
        }
    }

    return 0;
}