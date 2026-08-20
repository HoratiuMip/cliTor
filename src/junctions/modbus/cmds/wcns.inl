{   
    .text = "wcns",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::argi32, .fast_id = 0x0 },
        { .sh0rt = 'n', .l0ng = "count", .arg = rgh::Fast_cli::argi32, .fast_id = 0x1 },
        { .sh0rt = 'v', .l0ng = "value", .arg = rgh::Fast_cli::argi32, .fast_id = 0x2 }
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
}