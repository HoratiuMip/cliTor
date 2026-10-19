{   .text = "clear",
    .opts = {},
    .fnc = [ this ] ( auto& C ) -> rgh::ret_t {
        this->clear();
        return OK;
    }
}