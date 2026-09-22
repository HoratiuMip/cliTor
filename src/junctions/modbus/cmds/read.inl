{   
    .text = "rh",
    .opts = {
        { .sh0rt = 'a', .l0ng = "address", .arg = rgh::Fast_cli::argi32, .fast_id = 0x0 },
        { .sh0rt = 'n', .l0ng = "number", .arg = rgh::Fast_cli::argi32, .fast_id = 0x1 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t { LOCK_CONTEXT
        int  addr  = -0x1;
        int  nb    = 1;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'a': addr = C.i32(); break;
            case 'n': nb   = C.i32(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( addr >= 0x0 ) {
            JUNCTION_DOCK_LOGE( "rh: bad or no address." );
            return ERR_BADARG;
        }

        uint16_t holds[ nb ];
        ASSERT_OR( modbus_read_registers( ctx.get(), addr, nb, holds ) != -1 ) {
            JUNCTION_DOCK_LOGE( "rh: {}.", modbus_strerror( errno ) );
            return ERR_EXCOMCALL;
        }
        
        std::string output;
        for( int idx = 0x0; idx < nb; ++idx ) {
            output += std::format( 
                "{:#X}: {:#X} | {:#b} | {}\n",
                addr + idx, holds[ idx ], holds[ idx ], holds[ idx ]
            );
        }
        JUNCTION_DOCK_LOGI( "\n{}", output );
        
        return OK;
    }
}