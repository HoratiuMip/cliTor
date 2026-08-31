{   .text = "qinet",
    .opts = {
        { .sh0rt = 'h', .l0ng = "hosts-of", .arg = rgh::Fast_cli::argtext },
        { .sh0rt = 't', .l0ng = "ntp", .arg = rgh::Fast_cli::argtext },
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'h': {
                const auto domain = C.str();
                const auto hosts  = rgh::io::ipv4_hosts_of( domain );
                ASSERT_OR( hosts ) break;
                
                std::string out; out.reserve( ( rgh::io::IPv4_ADDR_STR_MAX_SZ + 1 ) * hosts->size() );
                for( const auto& host : *hosts ) out += std::format( "\n{}", rgh::io::ipv4_addr_str_t{ host }.c_str() );

                JUNCTION_DOCK_LOGI( "qinet: {} hosts for {}:{}", hosts->size(), domain, out );
            break; }

            case 't': {
                const auto ntp_server = C.str();
                auto ntp_hosts = rgh::io::ipv4_hosts_of( ntp_server );

                ASSERT_OR( ntp_hosts && not ntp_hosts->empty() ) {
                    JUNCTION_DOCK_LOGE( "qinet ntp: no hosts found for {}.", ntp_server );
                    break;
                }
    
                const auto& ntp_host = ntp_hosts->front();

                rgh::io::IPv4_UDP_rogue_client client;
                auto port = client.port_of( { .addr = ntp_host, .port = rgh::io::NTP_PORT } );

                auto ntp_packet = rgh::io::ntp_get( port );
                ASSERT_OR( ntp_packet ) {
                    JUNCTION_DOCK_LOGE( "qinet ntp: bad request from {}:{} ({})...", ntp_server, rgh::io::NTP_PORT, rgh::io::ipv4_addr_str_t{ ntp_host }.c_str() );
                    break;
                }   

                JUNCTION_DOCK_LOGI( 
                    "qinet ntp: current NTP unix time:\n{} - {}", 
                    ntp_packet->ts.tx_s, 
                    std::format( 
                        "{:%Y-%m-%d %H:%M:%S}", 
                        std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::sys_seconds{ std::chrono::seconds{ ntp_packet->ts.tx_s } } } 
                    )
                );

            break; }
        RGH_FASTCLI_OPT_SWITCH_END
        return OK;
    }
}