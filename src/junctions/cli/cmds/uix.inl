{   .text = "uix-up",
    .opts = {
        { .sh0rt = 'w', .l0ng = "width", .arg = rgh::Fast_cli::argi32, .fast_id = 0x0 },
        { .sh0rt = 'h', .l0ng = "height", .arg = rgh::Fast_cli::argi32, .fast_id = 0x1 },
        { .sh0rt = 'm', .l0ng = "minimize" },
        { .sh0rt = 'M', .l0ng = "maximize"}
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        Bridge::uix_up_args_t args;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'w': args.width = C.i32(); break;
            case 'h': args.height = C.i32(); break;
            case 'f': args.font_scale = C.f32(); break;
            case 'm': args.bgnas = rgh::Immersive::Iconify; break;
            case 'M': args.bgnas = rgh::Immersive::Maximize; break;
        RGH_FASTCLI_OPT_SWITCH_END

        BridgE.uix_up( args );
        return OK;
    }
},
{   .text = "uix-down",
    .fnc = [ this ] ( auto& C ) -> status_t {
        BridgE.uix_down();
        return OK;
    }
}