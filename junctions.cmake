# DETAILS: This junctions.cmake file allows you to select which modules are built and linked to the bridge.
#          Use the add_junction( "junction_root_directory" ) function to add a built-in junction. 
#          Use the add_junction_ex( "junction_root_directory" ) function to add an external junction.
#
# MANUAL: Copy this file in "./junctions_open.cmake" and comment out the junction you don't want.
#         Create the file "./junctions_closed.cmake". If you don't use closed junctions, you don't have to create the file.

# VVV --- COPY FROM HERE --- VVV

# Quintessential junctions - yes, yes, like the anime.
    # The built-in command line interpreter.
    add_junction( cli )
    
# Base:
    # Modbus RTU & TCP tool.
    add_junction( modbus )
    # SCPI protocol for lab instruments.
    add_junction( scpi )

# Specific junctions - ready-to-use tools for specific device or use case.
    # Topdon TC001 thermal camera tool. 
    add_junction( topdon-TC001 )

# Contract junctions - closed source or client apps.
include( ${CMAKE_CURRENT_LIST_DIR}/junctions_closed.cmake OPTIONAL )