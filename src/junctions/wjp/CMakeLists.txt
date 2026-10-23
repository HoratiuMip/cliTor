cmake_minimum_required( VERSION 4.3 )
add_library( ${JUNCTION_NAME} OBJECT "wjp.cxx" )

add_executable( wjp_self_test wjp_self_test.cxx )
target_link_libraries( wjp_self_test PRIVATE rgh::libbrp )

add_custom_command(
    TARGET wjp_self_test POST_BUILD
    COMMAND wjp_self_test
    COMMENT "[WJP] Running self tests..."
    VERBATIM
)