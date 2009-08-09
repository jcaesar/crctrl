
# PROJECT ( crctrl )

FIND_PACKAGE ( Boost REQUIRED )
FIND_PACKAGE ( mysqlpp )
FIND_PACKAGE ( MySQL )

INCLUDE_DIRECTORIES ( ../inc )
INCLUDE_DIRECTORIES ( ${MySQLpp_INCLUDE_DIR} )
INCLUDE_DIRECTORIES ( ${MySQL_INCLUDE_DIR} )

INCLUDE_DIRECTORIES ( .. )

SET ( crctrl_source 
	Config.cpp
	GameControl.cpp
	Control.cpp
	AutoHost.cpp
	Main.cpp
)

ADD_EXECUTABLE( crctrl ${crctrl_source} )
TARGET_LINK_LIBRARIES( crctrl pthread mysqlpp boost_regex-mt ${BASE_DIR}/helpers/libhelpers.a )

