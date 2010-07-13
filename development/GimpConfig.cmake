cmake_minimum_required(VERSION 2.4)


set (WHOAMI_COMMAND whoami )
set (GIMP_COMMAND gimp -v )

set (PROJECT_SOURCE_DIR  SRC )
set (ECHO echo )

set (GIMP_FILES  annotate.scm  crop.scm axes.scm )
set (CMD_FILES   annotate.sh  crop.sh axes.sh )



set (DIR . )



execute_process(COMMAND ${GIMP_COMMAND}
      WORKING_DIRECTORY ${DIR}
      OUTPUT_VARIABLE GIMP_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(COMMAND ${WHOAMI_COMMAND}
     WORKING_DIRECTORY ${DIR}
     OUTPUT_VARIABLE  USERID
     OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX REPLACE "^.*([gG][iI][mM][pP]|Manipulation Program) +[Vv][Ee][rR][sS][Ii][oO][Nn] +([0-9]+\\.[0-9]+).*"
        "\\2" GIMP_VERSION_NUMBER "${GIMP_VERSION}"
)

set (GIMP_INSTALL_DIRECTORY "/home/${USERID}/.gimp-${GIMP_VERSION_NUMBER}/scripts")


#message ("Your gimp-version is '${GIMP_VERSION}'")
message ("Your Gimp install directory is ${GIMP_INSTALL_DIRECTORY}")
#message ("You are ${USERID}" )

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