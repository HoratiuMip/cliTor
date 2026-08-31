# DETAILS: This junctions.cmake file allows you to select which modules are built and linked to the bridge.
#          Use the add_junction( "junction_root_directory" ) function. The junction root directory must be under
#            the src/junction directory, as of now...

# Quintessential junctions - yes, yes, like the anime.
    # The built-in command line interpreter.
    add_junction( cli )
    # Modbus RTU & TCP tool.
    add_junction( modbus )
# Base:

# Specific junctions - ready-to-use tools for specific device or use case.
    # Topdon TC001 thermal camera tool. 
    add_junction( topdon-TC001 )

# Contract junctions - closed source or client apps.
include( ${CMAKE_CURRENT_LIST_DIR}/closed_junctions.cmake )