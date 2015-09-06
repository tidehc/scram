pkg_check_modules(GLIB_PKG glib-2.0)

if(GLIB_PKG_FOUND)
  find_path(GLIB_INCLUDE_DIR  NAMES glib.h PATH_SUFFIXES glib-2.0
    PATHS
    ${GLIB_PKG_INCLUDE_DIRS}
    /usr/include/glib-2.0
    /usr/include
    /usr/local/include
    )
  find_path(GLIB_CONFIG_INCLUDE_DIR NAMES glibconfig.h PATHS ${GLIB_PKG_LIBDIR} PATH_SUFFIXES glib-2.0/include)

  find_library(GLIB_LIBRARIES NAMES glib-2.0
    PATHS
    ${GLIB_PKG_LIBRARY_DIRS}
    /usr/lib
    /usr/local/lib
    )

else()
  # Find Glib even if pkg-config is not working (eg. cross compiling to Windows)
  find_library(GLIB_LIBRARIES NAMES glib-2.0)
  string(REGEX REPLACE "/[^/]*$" "" GLIB_LIBRARIES_DIR ${GLIB_LIBRARIES})

  find_path(GLIB_INCLUDE_DIR NAMES glib.h PATH_SUFFIXES glib-2.0)
  find_path(GLIB_CONFIG_INCLUDE_DIR NAMES glibconfig.h PATHS ${GLIB_LIBRARIES_DIR} PATH_SUFFIXES glib-2.0/include)

endif()

if(GLIB_INCLUDE_DIR AND GLIB_CONFIG_INCLUDE_DIR AND GLIB_LIBRARIES)
  set(GLIB_INCLUDE_DIRS ${GLIB_INCLUDE_DIR} ${GLIB_CONFIG_INCLUDE_DIR})
endif()

if(GLIB_INCLUDE_DIRS AND GLIB_LIBRARIES)
  set(GLIB_FOUND TRUE CACHE INTERNAL "glib-2.0 found")
  message(STATUS "Found glib-2.0: ${GLIB_INCLUDE_DIR}, ${GLIB_LIBRARIES}")
else()
  set(GLIB_FOUND FALSE CACHE INTERNAL "glib-2.0 found")
  message(STATUS "glib-2.0 not found.")
endif()

mark_as_advanced(GLIB_INCLUDE_DIR GLIB_CONFIG_INCLUDE_DIR GLIB_INCLUDE_DIRS GLIB_LIBRARIES)