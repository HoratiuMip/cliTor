# DETAILS: This junctions.cmake file allows you to select which modules are built and linked to the bridge.
#          Use the add_junction( "junction_root_directory" ) function to add a built-in junction. 
#          Use the add_junction_ex( "junction_root_directory" ) function to add an external junction.
#
# MANUAL: Copy this file in "./junctions_sel.cmake" and comment out the junction you don't want.
#         Create the file "./junctions_spe.cmake". If you don't use special junctions, leave the file empty.

# VVV --- COPY FROM HERE --- VVV

# Quintessential junctions - yes, yes, like the anime.
    # The built-in command line interpreter.
    add_junction( cli )
    
# Baseic junctions:
    # Modbus RTU & TCP tool.
    add_junction( modbus )
    # SCPI protocol for lab instruments.
    add_junction( scpi )

# Target junctions - ready-to-use tools for target device or use case.
    # Topdon TC001 thermal camera tool. 
    add_junction( topdon-TC001 )

# Special junctions - closed source or client apps.
include( ${CMAKE_CURRENT_LIST_DIR}/junctions_spe.cmake OPTIONAL )