{   .text = "clear",
    .opts = {},
    .fnc = [ this ] ( auto& C ) -> status_t {
        this->clear();
        return OK;
    }
}