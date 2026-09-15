#define CLITOR_VERSION_MAJOR 1
#define CLITOR_VERSION_MINOR 0
#define CLITOR_VERSION_PATCH 0
#define CLITOR_VERSION_STR "cliTor-v1.0.0"

#define CLITOR_NAMESPACE namespace clitor

#define _DIRECTIVE_FRIENDS \
    _Pragma( "GCC diagnostic push" ) \
    _Pragma( "GCC diagnostic ignored \"-Wredundant-tags\"" ) \
    friend class Bridge; \
    friend class Dock_Directive; \
    friend class Proxy_Directive; \
    friend class UIX_Directive; \
    _Pragma( "GCC diagnostic pop" )