#pragma once
/**
 * @file: src/bridge.hpp
 * @brief: Bridge structure connecting the root machine with the proxys and docks.
 * @authors: Vatca "Mip" Tudor-Horatiu
 */

#include <rgh/osp/core.hpp>

typedef   rgh::status_t   status_t;

/**
 * @brief: Dock structure. This object handles everything needed for
 *           a specific communication test, such as a serial monitor.
 */
class Dock {
public:
    /**
     * @brief: Get the unqiue ID of the dock. Used to find and reference
     *           the Dock in the Bridge dock-registry.
     */
    virtual std::string_view dock_get_id( void ) const noexcept = 0x0;

};

/**
 * @brief: Proxy structure. This object helps the Bridge to instantiate and
 *           talk to Docks.
 */
class Proxy {
public: 
    /**
     * @brief: Get the name of the proxy. This is not unique, it is used to find
     *           and reference the Proxy in the Bridge proxy-registry.
     */
    virtual std::string_view proxy_get_name( void ) const noexcept = 0x0;
    /**
     * @brief: Create a dock using the given arguments. Minimally, this function should
     *           allocate a Dock instance and execute the argument commands on it. 
     */
    virtual rgh::HVec< Dock > proxy_spawn_dock( std::string_view args_ ) const noexcept = 0x0;

};

class Bridge {


};
extern Bridge BridgE;