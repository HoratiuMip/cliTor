cmake_minimum_required( VERSION 4.3 )

project(
    cliTor
)
set( CMAKE_CXX_STANDARD 26 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# ===== regolith-lain BAS begin =====
if( NOT DEFINED RGH_ROOT_DIR )
    set( RGH_ROOT_DIR "$ENV{RGH_ROOT_DIR}" )
endif()
include( "${RGH_ROOT_DIR}/tools.cmake" )

set( RGH_TARGET_PLATE "OS" )
set( RGH_EXCOM_POPCOUNT "yyyyyyn" )

add_subdirectory( "${RGH_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/regolith-lain" )
# ===== regolith-lain BAS end =====

add_executable( ${PROJECT_NAME} main.cpp ${srcs} )
target_link_libraries( ${PROJECT_NAME} PRIVATE rgh::libosp )

target_include_directories( ${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/" )

macro( add_junction junc_ )
    set( JUNCTION_NAME junction-${junc_} )
    add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/src/junctions/${junc_}/" )
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${JUNCTION_NAME} )
    target_link_libraries( ${JUNCTION_NAME} PRIVATE rgh::libosp )
    target_include_directories( ${JUNCTION_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/" )
endmacro()

macro( add_junction_ex junc_ dir_ )
    set( JUNCTION_NAME junction-${junc_} )
    add_subdirectory( "${dir_}" "${CMAKE_CURRENT_BINARY_DIR}/junction-${junc_}-build" )
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${JUNCTION_NAME} )
    target_link_libraries( ${JUNCTION_NAME} PRIVATE rgh::libosp )
    target_include_directories( ${JUNCTION_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/" )
endmacro()

include( "${CMAKE_CURRENT_LIST_DIR}/../selected_junctions.cmake" )