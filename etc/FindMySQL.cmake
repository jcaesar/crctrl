
# PROJECT ( mysql )
# Powered by Me

if (MySQL_INCLUDE_DIR)
  # Already in cache, be silent
  set (MySQL_FIND_QUIETLY TRUE)
endif (MySQL_INCLUDE_DIR)

find_path(MySQL_INCLUDE_DIR mysql.h
  /opt/local/include/mysql
  /usr/local/include/mysql
  /usr/include/mysql
)

set(MySQL_NAMES mysql)
find_library(MySQL_LIBRARY
  NAMES ${MySQL_NAMES}
  PATHS /usr/lib /usr/local/lib
  PATH_SUFFIXES mysql
)

if (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)
  set(MySQL_FOUND TRUE)
  set( MySQL_LIBRARIES ${MySQL_LIBRARY} )
else (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)
  set(MySQL_FOUND FALSE)
  set( MySQL_LIBRARIES )
endif (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)

if (MySQL_FOUND)
  if (NOT MySQL_FIND_QUIETLY)
    message(STATUS "Found MySQL: ${MySQL_LIBRARY}")
  endif (NOT MySQL_FIND_QUIETLY)
else (MySQL_FOUND)
  if (MySQL_FIND_REQUIRED)
    message(STATUS "Looked for MySQL libraries named ${MySQL_NAMES}.")
    message(FATAL_ERROR "Could NOT find MySQL library")
  endif (MySQL_FIND_REQUIRED)
endif (MySQL_FOUND)

mark_as_advanced(
  MySQL_LIBRARY
  MySQL_INCLUDE_DIR
  )
