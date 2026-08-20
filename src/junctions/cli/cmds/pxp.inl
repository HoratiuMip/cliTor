{   .text = "pxp",
    .opts = {
        { .sh0rt = 'n', .l0ng = "name", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 },
        { .sh0rt = 'l', .l0ng = "line", .arg = rgh::Fast_cli::argtext, .fast_id = 0x1 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string proxy_name = {};
        std::string line       = {};

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'n': proxy_name = C.text(); break;
            case 'l': line = C.text(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( !proxy_name.empty() && !line.empty() ) {
            JUNCTION_DOCK_LOGE( "proxy pass: incomplete arguments." );
            return ERR_PARTIAL;
        }

        BridgE.proxy_pass( proxy_name, std::move( line ) );
        return OK;
    }
}