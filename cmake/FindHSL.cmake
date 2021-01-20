# - try to find the HeartSensorLibrary SDK - currently designed for the version on GitHub.
#
# Cache Variables: (probably not for direct use in your scripts)
#  HSL_INCLUDE_DIR
#
# Non-cache variables you might use in your CMakeLists.txt:
#  HSL_FOUND
#  HSL_INCLUDE_DIR
#  HSL_BINARIES_DIR
#  HSL_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

message(STATUS "Finding HeartSensorLibrary, CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}/../../../deps/HeartSensorLibrary/src/HeartSensorLibrary")

IF (HSL_INCLUDE_DIR AND HSL_LIBRARIES AND HSL_BINARIES_DIR)
    # in cache already
    set(HSL_FOUND TRUE)

ELSE (HSL_INCLUDE_DIR AND HSL_LIBRARIES AND HSL_BINARIES_DIR)
    set(HSL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../deps/HeartSensorLibrary/dist/)

    # Find the include path
    find_path(HSL_INCLUDE_DIR
        NAMES
            ClientConstants.h ClientMath_CAPI.h HSLClient_CAPI.h HSLClient_export.h
        PATHS 
            ${HSL_ROOT_DIR}/include
            /usr/local/include)

    # Find the libraries 
    set(HSL_LIB_SEARCH_PATH ${HSL_ROOT_DIR}/lib)
    IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        IF (${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
            list(APPEND HSL_LIB_SEARCH_PATH
                ${HSL_ROOT_DIR}/lib/win64)
        ELSE()
            list(APPEND HSL_LIB_SEARCH_PATH
                ${HSL_ROOT_DIR}/lib/win32)
        ENDIF()
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        IF (${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
            list(APPEND HSL_LIB_SEARCH_PATH
                ${HSL_ROOT_DIR}/lib/linux64
                /usr/local/lib)
        ELSE()
            list(APPEND HSL_LIB_SEARCH_PATH
                ${HSL_ROOT_DIR}/lib/linux32
                /usr/local/lib)  
        ENDIF()      
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        list(APPEND HSL_LIB_SEARCH_PATH
            ${HSL_ROOT_DIR}/lib/osx32
            /usr/local/lib)
    ENDIF()
    
    FIND_LIBRARY(HSL_LIBRARY
            NAMES HSLService.dylib HSLService.so HSLService.lib HSLService
            PATHS ${HSL_LIB_SEARCH_PATH})
    SET(HSL_LIBRARIES ${HSL_LIBRARY})
    
    # Find the path to copy DLLs from
    set(HSL_BIN_SEARCH_PATH ${HSL_ROOT_DIR}/bin)
    IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        IF (${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
            list(APPEND HSL_BIN_SEARCH_PATH
                ${HSL_ROOT_DIR}/bin/win64)
        ELSE()
            list(APPEND HSL_BIN_SEARCH_PATH
                ${HSL_ROOT_DIR}/bin/win32)
        ENDIF()
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        IF (${CMAKE_C_SIZEOF_DATA_PTR} EQUAL 8)
            list(APPEND HSL_BIN_SEARCH_PATH
                ${HSL_ROOT_DIR}/bin/linux64
                /usr/local/bin)
        ELSE()
            list(APPEND HSL_BIN_SEARCH_PATH
                ${HSL_ROOT_DIR}/bin/linux32
                /usr/local/bin)  
        ENDIF()      
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        list(APPEND HSL_BIN_SEARCH_PATH
            ${HSL_ROOT_DIR}/bin/osx32
            /usr/local/bin)
    ENDIF()

    find_path(HSL_BINARIES_DIR
        NAMES
            HSLService.dylib HSLService.so HSLService.dll
        PATHS 
            ${HSL_BIN_SEARCH_PATH})

    # Register HSL_LIBRARIES, HSL_INCLUDE_DIR, HSL_BINARIES_DIR
    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(HSL DEFAULT_MSG HSL_LIBRARIES HSL_INCLUDE_DIR HSL_BINARIES_DIR)

    MARK_AS_ADVANCED(HSL_INCLUDE_DIR HSL_LIBRARIES)           
     
ENDIF (HSL_INCLUDE_DIR AND HSL_LIBRARIES AND HSL_BINARIES_DIR)