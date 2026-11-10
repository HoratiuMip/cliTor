#include "modbus.h"

#include <rgh/brp/IO_port.hpp>

#include <print>

#include <clitor.hxx>
JUNCTION_HEADER( modbus, "modbus" )
using namespace clitor;

#define LOCK_CONTEXT auto ctx = this->ctx; ASSERT_OR( ctx ) { std::println( "No or bad modbus context. Have you successfully TCP'd/RTU'd to the slave?" ); return ERR_TERMINATED; }

class Modbus : public Dock {
public:
    Modbus( void )
    : cli{
        {},
        { 
        #include "cmds/uplink.inl"
        ,
        #include "cmds/read.inl"
        ,
        #include "cmds/write.inl"
        }
    } {}


public:
    JUNCTION_DOCK_PASS_FNC_SIG {
        std::string out; cli.execute( line_, &out ); std::println( "{}", out );
        return OK;
    }

public:
    std::shared_ptr< modbus_t >   ctx   = nullptr;
    rgh::Fast_cli                 cli   = {};
};

class ModbusProxy : public Proxy {
public:
    JUNCTION_PROXY_GET_NAME

    JUNCTION_PROXY_INIT_PROC_FNC_SIG { return OK; }

    JUNCTION_PROXY_PASS_FNC_SIG{
        switch( rgh::txt_hash( line_ ) ) {
            JUNCTION_PROXY_PASS_BASIC_DOCK_INSTALL( Modbus )
        }
        return OK;
    }
};
JUNCTION_PROXY_INSTALL( ModbusProxy )
JUNCTION_FOOTER
