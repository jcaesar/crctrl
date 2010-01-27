cmake_minimum_required(VERSION 2.6)

PROJECT( crctrl )

# Ask for DB login data.
IF( NOT DEFINED SQL_RUNTIME_LOGIN )
	SET( SQL_RUNTIME_LOGIN OFF CACHE BOOL "Whether Login info will be retrieved at startup-time or at compile-time." )
ENDIF()
IF(NOT DEFINED SQL_NAME)

	SET( SQL_NAME "crctrl" CACHE STRING "MySQL login name." )
ENDIF()
IF(NOT DEFINED SQL_DB)
	SET( SQL_DB "crctrl" CACHE STRING "MySQL db." )
ENDIF()
IF(NOT DEFINED SQL_PW)
	SET( SQL_PW "" CACHE STRING "MySQL login password." )
ENDIF()
IF(NOT DEFINED SQL_ADDR)
	SET( SQL_ADDR "" CACHE STRING "MySQL server adress." )
ENDIF()

IF( NOT SQL_RUNTIME_LOGIN  )
	IF( SQL_DB STREQUAL "" )
		MESSAGE(SEND_ERROR "You did not specify a database to use.")
	ENDIF()
	IF( SQL_NAME STREQUAL "" )
		MESSAGE(SEND_ERROR "You did not specify a MySQL-login name.")
	ENDIF()
ENDIF()


SET ( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/etc )
SET ( BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

ADD_SUBDIRECTORY( helpers )
ADD_SUBDIRECTORY( src )