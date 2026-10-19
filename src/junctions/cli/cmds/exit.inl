{   .text = "exit",
    .opts = {},
    .fnc = [ this ] ( auto& C ) -> rgh::ret_t {
        clitor::BridgE.daemon_stop();
        return OK;
    }
}