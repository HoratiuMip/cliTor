#define WJP_EUCLID_IMPLEMENTATION
#define WJP_EUCLID_TEST
#include <cstdint>
#include <rgh/brp/wjp_euclid.h>

#include <clitor.hxx>
JUNCTION_HEADER( wjp, "wjp" )

class Proxy : public clitor::Proxy {
public:
    JUNCTION_PROXY_GET_NAME

    JUNCTION_PROXY_INIT_PROC_FNC_SIG {
        if( auto res = WJP_euclid_test(); res ) {
            BRIDGE_LOGE( "self test failed: {}.", res );
        } else {
            BRIDGE_LOGI( "self test passed." );
        }
        
        return OK;
    }
};

JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
