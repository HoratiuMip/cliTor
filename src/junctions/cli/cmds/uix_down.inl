{   .text = "uix-down",
    .fnc = [ this ] ( auto& C ) -> status_t {
        BridgE.uix_down();
        return OK;
    }
}