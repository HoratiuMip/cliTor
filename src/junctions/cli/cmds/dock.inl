{   
    .text = "install",
    .opts = {
        { .sh0rt = 'p', .l0ng = "proxy", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 },
        { .sh0rt = 'c', .l0ng = "cd" }
    },
    .man = "Install a dock into the bridge.",
    .fnc = [ this ] ( auto& C ) -> status_t {
        std::string proxy_name = {};
        bool        cmd_dock   = false;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'p': proxy_name = C.str(); break;
            case 'c': cmd_dock = true; break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( !proxy_name.empty()  ) {
            JUNCTION_DOCK_LOGE( "install: no proxy specified." );
            return ERR_BADARG;
        }

        BridgE.proxy_pass( proxy_name, "install" );
        return OK;
    }
},
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
},
{   .text = "cd",
    .opts = {
        { .sh0rt = 'i', .l0ng = "id", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        rgh::HVec< Dock > cd = nullptr;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'i': {
                auto id = C.text(); cd = BridgE.dock_by_id( id ); 
            break; }
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( cd ) {
            return ERR_NOT_FOUND;
        }

        _cd = std::move( cd );
        return OK;
    }
}