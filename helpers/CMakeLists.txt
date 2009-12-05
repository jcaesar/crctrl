
# PROJECT ( helpers )

FIND_PACKAGE ( pthread )
INCLUDE_DIRECTORIES ( ${PTHREADS_INCLUDE_DIR} )

FILE(GLOB_RECURSE helpers_source "*.cpp")
FILE(GLOB_RECURSE helpers_headers "*.h")
FILE(GLOB_RECURSE helpers_ppheaders "*.hpp")

IF( "${CMAKE_C_COMPILER}" MATCHES "/usr/bin/gcc" )
	ADD_DEFINITIONS(-Wall -g)
ELSEIF( MSVC )
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
ENDIF( "${CMAKE_C_COMPILER}" MATCHES "/usr/bin/gcc" )
ADD_LIBRARY( helpers STATIC ${helpers_source} ${helpers_headers} ${helpers_ppheaders} )
