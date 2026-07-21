cmake_minimum_required( VERSION 4.3 )

project(
    cliTor
)
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# ===== regolith-lain BAS begin =====
# Location of regolith-lain.
set( RGH_ROOT_DIR "$ENV{RGH_ROOT_DIR}" )
include( "${RGH_ROOT_DIR}/tools.cmake" )

# Build target plate.
# - "uC" - target is a microcontroller device.
# - "OS" - target is an operating system ready device. 
set( RGH_TARGET_PLATE "OS" )

# Target operating system.
# - OSp:
#   - "Windows"
#   - "Linux"
# - uCp:
#   - <empty>
#   - "FreeRTOS"
set( RGH_TARGET_OS "Linux" ) 

# Target platform.
# - OSp:
#   - <empty>
# - uCp:
#   - "esp32"
set( RGH_TARGET_PLATFORM "" )

# * Which external components to make, indicated by a string containing 'y' and 'n' ( for "yes" and "no" ), in the order in which they are built in the script.
# Example: "ynnny" - makes the first and fifth external component.
set( RGH_EXCOM_POPCOUNT
    "yyyyyyn"
)

# Add regolith-lain.
add_subdirectory( "${RGH_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/regolith-lain" )
# ===== regolith-lain BAS end =====

add_executable( ${PROJECT_NAME} main.cpp ${srcs} )
target_link_libraries( ${PROJECT_NAME} PRIVATE rgh::libosp )

target_include_directories( ${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/" )

macro( add_junction junc_ )
    set( JUNCTION_NAME ${junc_} )
    add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/src/junctions/${junc_}/" )
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${junc_} )
    target_link_libraries( ${junc_} PRIVATE rgh::libosp )
    target_include_directories( ${junc_} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/" )
endmacro()

include( "${CMAKE_CURRENT_LIST_DIR}/../junctions.cmake" )