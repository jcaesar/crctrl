
# PROJECT ( crctrl )

FIND_PACKAGE ( Boost REQUIRED )
FIND_PACKAGE ( MySQL )
FIND_PACKAGE ( mysqlpp )
FIND_PACKAGE ( pthread) 

INCLUDE_DIRECTORIES ( ../inc ${MySQL_INCLUDE_DIR} ${MySQLpp_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${PTHREADS_INCLUDE_DIR} )


INCLUDE_DIRECTORIES ( .. )

SET ( crctrl_source 
	Config.cpp
	GameControl.cpp
	Control.cpp
	AutoHost.cpp
	Main.cpp
)

IF( "${CMAKE_C_COMPILER}" MATCHES "/usr/bin/gcc" )
	ADD_DEFINITIONS(-Wall -g)
ENDIF( "${CMAKE_C_COMPILER}" MATCHES "/usr/bin/gcc" )
ADD_EXECUTABLE( crctrl ${crctrl_source} )
TARGET_LINK_LIBRARIES( crctrl pthread mysqlpp boost_regex-mt ../helpers/libhelpers )
