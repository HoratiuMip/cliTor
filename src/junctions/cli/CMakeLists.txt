cmake_minimum_required( VERSION 4.3 )
add_library( ${JUNCTION_NAME} OBJECT "cli.cpp" )

RGH_Fnc_CloneRepo( https://github.com/AmokHuginnsson/replxx.git master replxx ${CMAKE_CURRENT_BINARY_DIR} )

target_link_libraries( ${JUNCTION_NAME} 
    PRIVATE replxx 
)