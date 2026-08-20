{   
    .text = "uninstall",
    .opts = {
        { .sh0rt = 'd', .l0ng = "dock", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string dock_id = {};

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'd': dock_id = C.text(); break;
        RGH_FASTCLI_OPT_SWITCH_END

        if( dock_id.empty() ) {
            auto cd = _cd;
            ASSERT_OR( cd ) {
                JUNCTION_DOCK_LOGE( "uninstall: dock not specified nor commanded." );
                return ERR_NO_RESOLVE;
            }

            dock_id = cd->dock_id();
            _cd.reset();
        } 

        return BridgE.uninstall_dock( dock_id );
    }
}