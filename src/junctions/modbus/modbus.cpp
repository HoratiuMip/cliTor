#include "modbus.h"

#include <rgh/brp/IO_port.hpp>

#include <bridge.hpp>
JUNCTION_HEADER( modbus, "modbus" )

#define LOCK_CONTEXT auto ctx = this->ctx; ASSERT_OR( ctx ) { return ERR_TERMINATED; }

class Modbus : public Dock {
public:
    Modbus( void )
    : cli{
        {},
        { 
#include "cmds/tcp.inl"
,

#include "cmds/wc.inl"
,
#include "cmds/wcns.inl"        
,
#include "cmds/wr.inl"
,
#include "cmds/wrns.inl"
        }
    } {}


public:
    JUNCTION_DOCK_PASS_FNC_SIG {
        std::string out;
        cli.execute( line_, &out );
        return OK;
    }

public:
    std::shared_ptr< modbus_t >   ctx   = nullptr;
    rgh::Fast_cli                 cli   = {};

#pragma region UIX
public:
    struct UIX_pack : ::Dock::UIX_pack {
        virtual ~UIX_pack() override = default;
    };


//# TUFILIN - optional, the function called when the UIX begins, or the dock is installed.
//#         - return a shared reference to your UIX pack structure. It will be passed to your UIX frame callback.
    JUNCTION_DOCK_UIX_BEGIN_FNC_SIG {
        return std::make_shared< UIX_pack >();
    }
//# TUFILIN - optional, the function called each frame of the UIX.
//#         - all UIX related memory is guaranteed to be valid inside this function.
//#         - a reference to your UIX pack is passed through this function's argument.
    JUNCTION_DOCK_UIX_FRAME_FNC_SIG {
        return OK;
    }
//# TUFILIN - optional, the function called when the UIX ends, or the dock is uninstalled.
    JUNCTION_DOCK_UIX_END_FNC_SIG {

    }
#pragma endregion UIX

};

class Proxy : public ::Proxy {
public:
    JUNCTION_PROXY_GET_NAME

    JUNCTION_PROXY_WAKE_FNC_SIG {
    }

    JUNCTION_PROXY_PASS_FNC_SIG{
        switch( rgh::txt_hash( line_ ) ) {
            JUNCTION_PROXY_PASS_BASIC_DOCK_INSTALL( Modbus )
        }
        return OK;
    }
};
JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
