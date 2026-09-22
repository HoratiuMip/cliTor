cmake_minimum_required( VERSION 4.3 )
add_library( ${JUNCTION_NAME} OBJECT "cli.cpp" )

FetchContent_Declare(
  replxx
  GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx.git
  GIT_TAG        master
)
FetchContent_MakeAvailable( replxx )

target_link_libraries( ${JUNCTION_NAME} 
    PRIVATE replxx 
)