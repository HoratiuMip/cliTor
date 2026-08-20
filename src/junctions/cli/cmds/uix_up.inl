{   .text = "uix-up",
    .opts = {
        { .sh0rt = 'w', .l0ng = "width", .arg = rgh::Fast_cli::argi32, .fast_id = 0x0 },
        { .sh0rt = 'h', .l0ng = "height", .arg = rgh::Fast_cli::argi32, .fast_id = 0x1 },
        { .sh0rt = 'm', .l0ng = "minimize" },
        { .sh0rt = 'M', .l0ng = "maximize"}
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        int   width  = 680;
        int   height = 680;
        float font_scale = 1.22f;
        auto  bgnas  = rgh::Immersive::Default;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'w': width = C.i32(); break;
            case 'h': height = C.i32(); break;
            case 'f': font_scale = C.f32(); break;
            case 'm': bgnas = rgh::Immersive::Iconify; break;
            case 'M': bgnas = rgh::Immersive::Maximize; break;
        RGH_FASTCLI_OPT_SWITCH_END

        BridgE.uix_up( { width, height, font_scale, bgnas } );
        return OK;
    }
}