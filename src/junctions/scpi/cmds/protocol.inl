{   
    .text = "raw",
    .opts = {
        { .sh0rt = 'm', .l0ng = "message", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string msg = {};

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'm': msg = C.text(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( not msg.empty() ) {
            JUNCTION_DOCK_LOGE( "raw: no message to tx." ); return ERR_BADARG;
        }

        ASSERT_RET_OR( tx_raw_str( std::move( msg ) ) ) {
            JUNCTION_DOCK_LOGE( "raw: bad tx: {}.", RET_MSG( ret_ ) ); return ret_;
        }
        return OK;
    }
},
{   
    .text = "read",
    .opts = {
        { .sh0rt = 'c', .l0ng = "continous" },
        { .sh0rt = 'i', .l0ng = "interval", .arg = rgh::Fast_cli::argi32 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        int read_count  = 1;
        int interval_ms = 1'000;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'c': read_count = std::numeric_limits< decltype(read_count) >::max(); break;
            case 'i': interval_ms = C.i32(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        static volatile bool running = true;
        running = true;
        auto prev_handler = std::signal( SIGINT, [] ( int sig_ ) {
            running = false;
        } );

        for( int n = 1; n <= read_count && running; ++n ) {
            ASSERT_RET_OR( tx_raw_str( "READ?" ) ) {
                JUNCTION_DOCK_LOGE( "raw: bad tx: {}.", RET_MSG( ret_ ) ); return ret_;
            }
            std::this_thread::sleep_for( std::chrono::milliseconds{ interval_ms } );
        }
        std::signal( SIGINT, prev_handler );
        
        return OK;
    }
}
