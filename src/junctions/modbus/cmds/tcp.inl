{   
    .text = "tcp",
    .opts = {
        { .sh0rt = 'i', .l0ng = "ip", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 },
        { .sh0rt = 's', .l0ng = "slave-id", .arg = rgh::Fast_cli::argi32, .fast_id = 0x1 },
        { .sh0rt = 'p', .l0ng = "port", .arg = rgh::Fast_cli::argi32 },
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
}