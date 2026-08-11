cmake_minimum_required( VERSION 4.3 )
add_library( ${JUNCTION_NAME} OBJECT "modbus.cpp" )

find_package( PkgConfig REQUIRED )
pkg_check_modules( LIBMODBUS REQUIRED libmodbus )

target_link_libraries( ${JUNCTION_NAME} 
    PUBLIC ${LIBMODBUS_LIBRARIES}
)
target_include_directories( ${JUNCTION_NAME} 
    PUBLIC ${LIBMODBUS_INCLUDE_DIRS}
)