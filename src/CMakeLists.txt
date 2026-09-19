cmake_minimum_required( VERSION 4.3 )

project(
    cliTor
)
set( CMAKE_CXX_STANDARD 26 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

include( FetchContent )
set( BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE )

# Check for the junctions.json configuration file.
set( cliTor_File_JunctionsJson ${CMAKE_CURRENT_LIST_DIR}/../junctions.json )
if( NOT EXISTS ${cliTor_File_JunctionsJson} )
    message( FATAL_ERROR "[cliTor] The file \"junctions.json\" was not found. See docs on how to add it manually or use make.py." )
endif()

# ===== regolith-lain BAS begin =====
if( NOT DEFINED RGH_ROOT_DIR )
    set( RGH_ROOT_DIR $ENV{RGH_ROOT_DIR} )
endif()
include( ${RGH_ROOT_DIR}/tools.cmake )

set( RGH_EXCOM_POPCOUNT "yyyyyyn" )

add_subdirectory( ${RGH_ROOT_DIR} ${CMAKE_CURRENT_BINARY_DIR}/regolith-lain/ )
# ===== regolith-lain BAS end =====

add_executable( ${PROJECT_NAME} main.cpp )
target_link_libraries( ${PROJECT_NAME} PRIVATE rgh::libosp )

target_include_directories( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ )

# Macros for adding internal and external junctions.
macro( add_junction junc_ )
    set( JUNCTION_NAME junction-${junc_} )
    add_subdirectory( ${CMAKE_CURRENT_LIST_DIR}/junctions/${junc_}/ )
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${JUNCTION_NAME} )
    target_link_libraries( ${JUNCTION_NAME} PRIVATE rgh::libosp )
    target_include_directories( ${JUNCTION_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ )

    message( STATUS "[cliTor] Linked internal junction: ${junc_}." )
endmacro()
macro( add_junction_ex junc_ dir_ )
    set( JUNCTION_NAME junction-${junc_} )
    add_subdirectory( ${dir_} ${CMAKE_CURRENT_BINARY_DIR}/junction-${junc_}-build )
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${JUNCTION_NAME} )
    target_link_libraries( ${JUNCTION_NAME} PRIVATE rgh::libosp )
    target_include_directories( ${JUNCTION_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ )

    message( STATUS "[cliTor] Linked external junction: ${junc_} (${dir_})." )
endmacro()

# Configure the selected junctions.
file( READ ${cliTor_File_JunctionsJson} _jsonj )

string( JSON _selcntj LENGTH ${_jsonj} "selected" )
math( EXPR _selcnt "${_selcntj} - 1" )
foreach( idx RANGE ${_selcnt} )
    string( JSON sel GET ${_jsonj} "selected" ${idx} )

    string( JSON path GET ${sel} "path" )
# Internal junction.
    if( NOT path MATCHES "/" )
        add_junction( ${path} )
# External junction.
    else()
        string( JSON name GET ${sel} "name" )
        add_junction_ex( ${name} ${path} )
    endif()
endforeach()

# Installation.
install( TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION bin 
    COMPONENT ${PROJECT_NAME}
)