#include <print>
#include <rgh/brp/descriptor.hpp>

#define WJP_EUCLID_IMPLEMENTATION
#define WJP_EUCLID_TEST
#include <rgh/gep/WJPv5.hxx>

int main( int, char*[] ) {
    const char* ret = nullptr;

    ret = wjpxx::Euclid::self_test();
    ASSERT_OR( ret == nullptr ) {
        std::println( stderr, "\033[1;31mWJP euclid self test failed: {}.\033[0m", ret );
        return ERR_GENERAL;
    }

    std::println( "\033[1;32m[WJP] Self tests OK.\033[0m" );
    return OK;
}