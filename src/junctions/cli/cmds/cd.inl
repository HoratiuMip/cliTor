{   .text = "cd",
    .opts = {
        { .sh0rt = 'i', .l0ng = "id", .arg = rgh::Fast_cli::argtext, .fast_id = 0x0 }
    },
    .fnc = [ this ] ( auto& C ) -> status_t {
        rgh::HVec< Dock > cd = nullptr;

        RGH_FASTCLI_OPT_SWITCH_BEGIN(C)
            case 'i': cd = BridgE.dock_by_id( C.text() ); break;
        RGH_FASTCLI_OPT_SWITCH_END

        ASSERT_OR( cd ) {
            return ERR_NOT_FOUND;
        }

        _cd = std::move( cd );
        return OK;
    }
}