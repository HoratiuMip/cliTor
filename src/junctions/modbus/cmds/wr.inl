{   
    .text = "wr",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::argi32, .fast_id = 0x0 },
        { .sh0rt = 'v', .l0ng = "value", .arg = rgh::Fast_cli::argi32, .fast_id = 0x1 }
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
}