#include <rgh/osp/IO_sockets.hpp>
#include <rgh/osp/IO_utils.hpp>

#include <bridge.hpp>
JUNCTION_HEADER( scpi, "scpi" )

#define SCPI_DEFAULT_TCP_PORT 5025

static rgh::io::IPv4_Kraken   s_Kraken   = {};

class SCPI : public ::Dock {
public:
#pragma region Structures
    struct context_t {
        rgh::io::ipv4_endpoint_t   endpt   = {};
    };
#pragma endregion Structures

#pragma region Constructors
    SCPI( void )
    : cli{
        {},
        { 
        #include "cmds/connect.inl"
        ,
        #include "cmds/protocol.inl"

        }
    } {
        th_rx = std::jthread{ &SCPI::_rx_main, this };
    }
#pragma endregion Constructors

#pragma region Fields
    rgh::Fast_cli   cli     = {};
    context_t       ctx     = {};
    std::jthread    th_rx   = {};
#pragma endregion Fields

#pragma region Dock_overrides
    JUNCTION_DOCK_PASS_FNC_SIG {
        std::string out; cli.execute( line_, &out );
        return OK;
    }
#pragma endregion Dock_overrides

#pragma region Utility
    ret_t tx_raw_str( std::string str_ ) {
        str_ += "\r\n";

        return s_Kraken.write_to( ctx.endpt, rgh::io::port_W_desc_t{ 
            .src_ptr = reinterpret_cast< rgh::byte_t* >( str_.data() ),
            .src_n   = static_cast< int >( str_.length() )
        } );
    } 

    void _rx_main( std::stop_token thctl_ ) {
        while( not thctl_.stop_requested() ) {
            static constexpr int BUF_SZ = 1024;
            rgh::byte_t buf[ BUF_SZ ];

            int rx_byte_count = 0;
            ASSERT_OR( ctx.endpt.is_valid() and OK == s_Kraken.read_from( ctx.endpt, rgh::io::port_R_desc_t{ 
                .dst_ptr    = buf,
                .dst_n      = BUF_SZ,
                .byte_count = &rx_byte_count
            } ) ) {
                std::this_thread::sleep_for( std::chrono::milliseconds{ 300 } );
                continue;
            }

            std::string resp{ reinterpret_cast< char* >( buf ), static_cast< std::size_t >( rx_byte_count ) };
            JUNCTION_DOCK_LOGI( "{}", std::stod( resp ) );
        }
    }
#pragma endregion Utility

#pragma region UIX
public:
    struct UIX_pack : ::Dock::UIX_pack {
        virtual ~UIX_pack() override = default;
    };


//# TUFILIN - optional, the function called when the UIX begins, or the dock is installed.
//#         - return a shared reference to your UIX pack structure. It will be passed to your UIX frame callback.
    JUNCTION_DOCK_UIX_BEGIN_FNC_SIG {
        return std::make_shared< UIX_pack >();
    }
//# TUFILIN - optional, the function called each frame of the UIX.
//#         - all UIX related memory is guaranteed to be valid inside this function.
//#         - a reference to your UIX pack is passed through this function's argument.
    JUNCTION_DOCK_UIX_FRAME_FNC_SIG {
        return OK;
    }
//# TUFILIN - optional, the function called when the UIX ends, or the dock is uninstalled.
    JUNCTION_DOCK_UIX_END_FNC_SIG {

    }
#pragma endregion UIX

};

class Proxy : public ::Proxy {
public:
    JUNCTION_PROXY_GET_NAME

    JUNCTION_PROXY_PASS_FNC_SIG{
        switch( rgh::txt_hash( line_ ) ) {
            JUNCTION_PROXY_PASS_BASIC_DOCK_INSTALL( SCPI )
        }
        return OK;
    }
};
JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
