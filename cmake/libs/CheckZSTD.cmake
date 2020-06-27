# Check for zstd.
# If zstd isn't found, extlib/zstd/ will be used instead.

IF(NOT USE_INTERNAL_ZSTD)
	IF(ZSTD_LIBRARY MATCHES "^zstd$" OR ZSTD_LIBRARY MATCHES "^zstd")
		# Internal zstd was previously in use.
		UNSET(ZSTD_FOUND)
		UNSET(HAVE_ZSTD)
		UNSET(ZSTD_LIBRARY CACHE)
		UNSET(ZSTD_LIBRARIES CACHE)
	ENDIF()

	# Check for ZSTD.
	FIND_PACKAGE(ZSTD)
	IF(ZSTD_FOUND)
		# Found system ZSTD.
		SET(HAVE_ZSTD 1)
	ELSE()
		# System ZSTD was not found.
		MESSAGE(STATUS "Using the internal copy of zstd since a system version was not found.")
		SET(USE_INTERNAL_ZSTD ON CACHE BOOL "Use the internal copy of zstd" FORCE)
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of zstd.")
ENDIF(NOT USE_INTERNAL_ZSTD)

IF(USE_INTERNAL_ZSTD)
	# Using the internal zstd library.
	SET(ZSTD_FOUND 1)
	SET(HAVE_ZSTD 1)
	# FIXME: When was it changed from LIBRARY to LIBRARIES?
	IF(WIN32 OR APPLE)
		# Using DLLs on Windows and Mac OS X.
		SET(USE_INTERNAL_ZSTD_DLL ON)
		SET(ZSTD_LIBRARY zstd CACHE INTERNAL "ZSTD library" FORCE)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_ZSTD_DLL OFF)
		SET(ZSTD_LIBRARY zstdstatic CACHE INTERNAL "ZSTD library" FORCE)
	ENDIF()
	SET(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
	# FIXME: When was it changed from DIR to DIRS?
	SET(ZSTD_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extlib/zstd)
	SET(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
ELSE(USE_INTERNAL_ZSTD)
	SET(USE_INTERNAL_ZSTD_DLL OFF)
ENDIF(USE_INTERNAL_ZSTD)
