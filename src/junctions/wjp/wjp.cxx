#define WJP_EUCLID_IMPLEMENTATION
#include <rgh/brp/WJPv5.h>

#include <clitor.hxx>
JUNCTION_HEADER( wjp, "wjp" )

class Proxy : public clitor::Proxy {
public:
    JUNCTION_PROXY_GET_NAME

    JUNCTION_PROXY_INIT_PROC_FNC_SIG {
        return OK;
    }
};

JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
