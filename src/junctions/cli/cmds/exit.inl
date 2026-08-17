{   .text = "exit",
    .opts = {},
    .fnc = [ this ] ( auto& C ) -> status_t {
        BridgE.daemon_stop();
        return OK;
    }
}