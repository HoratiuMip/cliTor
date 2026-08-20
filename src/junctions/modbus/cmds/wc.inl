{   
    .text = "wc",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::argi32, .fast_id = 0x0 },
        { .sh0rt = 'v', .l0ng = "value", .arg = rgh::Fast_cli::argi32, .fast_id = 0x1 }
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
}