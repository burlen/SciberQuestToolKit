cmake_minimum_required(VERSION 2.4)


INCLUDE( ../GimpConfig.cmake )



if (NOT WIN32)
    foreach (GMPFILE ${CMD_FILES} )
    	install ( FILES ${GMPFILE} 
    		  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    	)	
    endforeach (GMPFILE)
    foreach (GMPFILE ${GIMP_FILES} )
    	install ( FILES ${GMPFILE} 
    		  DESTINATION  ${GIMP_INSTALL_DIRECTORY}
    	)	
    endforeach (GMPFILE)
endif (NOT WIN32)

