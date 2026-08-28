cmake_minimum_required( VERSION 4.3 )

find_package( OpenCV REQUIRED )

add_library( ${JUNCTION_NAME} OBJECT "topdon_TC001.cpp" )

target_include_directories( ${JUNCTION_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS} )
target_link_libraries( ${JUNCTION_NAME} PRIVATE ${OpenCV_LIBS} )
