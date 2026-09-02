{   
    .text = "tcp",
    .opts = {
        { .sh0rt = 'h', .l0ng = "host", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 },
        { .sh0rt = 'p', .l0ng = "port", .arg = rgh::Fast_cli::argi32 },
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string          host = {};
        rgh::io::ipv4_port_t port = SCPI_DEFAULT_TCP_PORT;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'h': host = C.text(); break;
            case 'p': port = C.i32(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        auto host_addrs = rgh::io::ipv4_hosts_of( host );
        ASSERT_OR( host_addrs ) {
            JUNCTION_DOCK_LOGE( "tcp: no hosts for {}: {}.", host, RET_MSG( host_addrs.error() ) );
            return host_addrs.error();
        }

        rgh::io::ipv4_endpoint_t endpt{ .addr = host_addrs->front(), .port = port, .proto = rgh::io::IP_PROTO_TCP };
        ASSERT_RET_OR_RET( s_Kraken.push( endpt ) );
        ctx.endpt = std::move( endpt );

        JUNCTION_DOCK_LOGI( "tcp'd {}:{}.", host, port );
        return OK;
    }
}