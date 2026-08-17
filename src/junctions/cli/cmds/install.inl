{   
    .text = "install",
    .opts = {
        { .sh0rt = 'p', .l0ng = "proxy", .arg = rgh::Fast_cli::Arg_text, .fast_id = 0x0 },
        { .sh0rt = 'c', .l0ng = "cd", .arg = rgh::Fast_cli::Arg_flag }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string proxy_name = {};
        bool        cmd_dock   = false;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'p': proxy_name = C.text(); break;
            case 'c': cmd_dock = true; break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( !proxy_name.empty()  ) {
            JUNCTION_DOCK_LOGE( "install: no proxy specified." );
            return ERR_BADARG;
        }

        BridgE.proxy_pass( proxy_name, "install" );
        return OK;
    }
}