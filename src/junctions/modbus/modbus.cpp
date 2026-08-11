#include "modbus.h"

#include <rgh/brp/IO_port.hpp>

#include <bridge.hpp>
JUNCTION_HEADER( modbus, "modbus" )

#define LOCK_CONTEXT auto ctx = this->ctx; ASSERT_OR( ctx ) { return ERR_TERMINATED; }

class Modbus : public Dock {
public:
    Modbus( void )
    : cli{
        {},
        { 
{   .text = "connect-tcp",
    .opts = {
        { .sh0rt = 'i', .l0ng = "ip", .arg = rgh::Fast_cli::Arg_text, .fast_id = 0x0 },
        { .sh0rt = 's', .l0ng = "slave-id", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x1 },
        { .sh0rt = 'p', .l0ng = "port", .arg = rgh::Fast_cli::Arg_i32 },
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string          ip   = {};
        rgh::io::ipv4_port_t port = MODBUS_TCP_DEFAULT_PORT;
        int                  slv  = 0x1;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'i': ip = C.text(); break;
            case 's': slv = C.i32(); break;
            case 'p': port = C.i32(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        modbus_t* raw_ctx = modbus_new_tcp( ip.c_str(), port );
        ASSERT_OR( raw_ctx ) {
            JUNCTION_DOCK_LOGE( "connect tcp: bad raw ctx: {}.", modbus_strerror( errno ) );
            return ERR_BADALLOC;
        }

        ctx = std::shared_ptr< modbus_t >{ 
            std::move( raw_ctx ),
            [] ( modbus_t* ptr_ ) { modbus_free( ptr_ ); } 
        };
        ASSERT_OR( ctx ) {
            JUNCTION_DOCK_LOGE( "connect tcp: bad shared ctx alloc." );
            return ERR_BADALLOC;
        }

        ASSERT_OR( modbus_connect( ctx.get() ) == 0x0 ) {
            JUNCTION_DOCK_LOGE( "bad connect tcp to {}:{}: {}.", ip, port, modbus_strerror( errno ) );
            return ERR_EXCOMCALL;
        }
        ASSERT_OR( modbus_set_slave( ctx.get(), slv ) == 0x0 ) {
            JUNCTION_DOCK_LOGW( "bad set slave id {} to {}:{}: {}.", slv, ip, port, modbus_strerror( errno ) );
        } 

        JUNCTION_DOCK_LOGI( "connected tcp to {}:{}.", ip, port );
        return OK;
    }
},

{   .text = "wc",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x0 },
        { .sh0rt = 'v', .l0ng = "value", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x1 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t { LOCK_CONTEXT
        int addr   = -0x1;
        int status = -0x1;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'a': addr = C.i32(); break;
            case 'v': status = C.i32(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( addr >= 0x0 && status >= 0x0 ) {
            JUNCTION_DOCK_LOGE( "write coil: incomplete or bad arguments." );
            return ERR_PARTIAL;
        }

        ASSERT_OR( 0x1 == modbus_write_bit( ctx.get(), addr, status ) ) {
           JUNCTION_DOCK_LOGE( "write coil: {}.", modbus_strerror( errno ) );
            return ERR_EXCOMCALL;
        }
        
        JUNCTION_DOCK_LOGI( "write coil: ok." );
        return OK;
    }
},
        
{   .text = "wcns",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x0 },
        { .sh0rt = 'n', .l0ng = "count", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x1 },
        { .sh0rt = 'v', .l0ng = "value", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x2 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t { LOCK_CONTEXT
        int addr  = -0x1;
        int nb    =  0;
        int value = -0x1;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'a': addr = C.i32(); break;
            case 'n': nb = C.i32(); break;
            case 'v': value = C.i32(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( addr >= 0x0 && nb > 0 && value >= 0x0 ) {
            JUNCTION_DOCK_LOGE( "write coils same: incomplete or bad arguments." );
            return ERR_PARTIAL;
        }

        uint8_t src[ nb ]; std::fill_n( src, nb, static_cast< uint8_t >( value ) );

        ASSERT_OR( modbus_write_bits( ctx.get(), addr, nb, src ) != -1 ) {
           JUNCTION_DOCK_LOGE( "write coils same: {}.", modbus_strerror( errno ) );
            return ERR_EXCOMCALL;
        }
        
        JUNCTION_DOCK_LOGI( "write coils same: ok." );
        return OK;
    }
},

{   .text = "wr",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x0 },
        { .sh0rt = 'v', .l0ng = "value", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x1 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t { LOCK_CONTEXT
        int      addr   = -0x1;
        uint16_t value  = 0xFF'FF;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'a': addr = C.i32(); break;
            case 'v': value = static_cast< uint16_t >( C.i32() ); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( addr >= 0x0 && value != 0xFF'FF ) {
            JUNCTION_DOCK_LOGE( "write reg: incomplete or bad arguments." );
            return ERR_PARTIAL;
        }

        ASSERT_OR( modbus_write_register( ctx.get(), addr, value ) != -1 ) {
           JUNCTION_DOCK_LOGE( "write reg: {}.", modbus_strerror( errno ) );
            return ERR_EXCOMCALL;
        }
        
        JUNCTION_DOCK_LOGI( "write reg: ok." );
        return OK;
    }
},

{   .text = "wrns",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x0 },
        { .sh0rt = 'n', .l0ng = "count", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x1 },
        { .sh0rt = 'v', .l0ng = "value", .arg = rgh::Fast_cli::Arg_i32, .fast_id = 0x2 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t { LOCK_CONTEXT
        int addr   = -0x1;
        int nb     =  0;
        int value  = -0x1;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'a': addr = C.i32(); break;
            case 'n': nb = C.i32(); break;
            case 'v': value = C.i32(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( addr >= 0x0 && nb > 0 && value >= 0x0 ) {
            JUNCTION_DOCK_LOGE( "write regs same: incomplete or bad arguments." );
            return ERR_PARTIAL;
        }

        uint16_t src[ nb ]; std::fill_n( src, nb, static_cast< uint16_t >( value ) );

        ASSERT_OR( modbus_write_registers( ctx.get(), addr, nb, src ) != -1 ) {
           JUNCTION_DOCK_LOGE( "write regs same: {}.", modbus_strerror( errno ) );
            return ERR_EXCOMCALL;
        }
        
        JUNCTION_DOCK_LOGI( "write regs same: ok." );
        return OK;
    }
},

        }
    } {}


public:
    JUNCTION_DOCK_PASS_FNC_SIG {
        std::string out;
        cli.execute( line_, &out );
        return OK;
    }

public:
    std::shared_ptr< modbus_t >   ctx   = nullptr;
    rgh::Fast_cli                 cli   = {};

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

    JUNCTION_PROXY_WAKE_FNC_SIG {
    }

    JUNCTION_PROXY_PASS_FNC_SIG{
        switch( rgh::txt_hash( line_ ) ) {
            JUNCTION_PROXY_PASS_BASIC_DOCK_INSTALL( Modbus )
        }
        return OK;
    }
};
JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
