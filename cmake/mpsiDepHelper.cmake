set(CMAKE_PREFIX_PATH "${MPSI_THIRDPARTY_HINT};${CMAKE_PREFIX_PATH}")

# ====== volepsi ======
macro(FIND_VOLEPSI)
    set(ARGS ${ARGN})
    if(FETCH_VOLEPSI)
        list(APPEND ARGS NO_DEFAULT_PATH PATHS ${MPSI_THIRDPARTY_HINT})
    elseif(${NO_SYSTEM_PATH})
        list(APPEND ARGS NO_DEFAULT_PATH PATHS ${CMAKE_PREFIX_PATH})
    endif()
    set(VOLEPSI_INCLUDE_DIR ${MPSI_THIRDPARTY_HINT}/include/)
    find_library(VOLEPSI_LIB NAMES volePSI PATH_SUFFIXES "/lib/" ${ARGS})
    if(EXISTS ${VOLEPSI_INCLUDE_DIR} AND EXISTS ${VOLEPSI_LIB})
        set(VOLEPSI_FOUND ON)
    else() 
        set(VOLEPSI_FOUND OFF) 
    endif()
endmacro(FIND_VOLEPSI)

if (FETCH_VOLEPSI_IMPL)
    FIND_VOLEPSI(QUIET)
    include(${CMAKE_CURRENT_LIST_DIR}/../thirdparty/getVolePSI.cmake)
endif()

if (ENABLE_VOLEPSI)
    FIND_VOLEPSI(REQUIRED)
    find_package(volePSI REQUIRED HINTS "${MPSI_THIRDPARTY_CLONE_DIR}/volepsi/build/linux")

    if(NOT TARGET cryptoTools)
        set(CRYPTOTOOLS_INCLUDE_DIR ${MPSI_THIRDPARTY_HINT}/include/)
        find_library(CRYPTOTOOLS_LIB NAMES cryptoTools PATH_SUFFIXES "/lib/" ${ARGS})
        add_library(cryptoTools STATIC IMPORTED)
        set_property(TARGET cryptoTools PROPERTY IMPORTED_LOCATION ${CRYPTOTOOLS_LIB})
        target_include_directories(cryptoTools INTERFACE
                        $<BUILD_INTERFACE:${CRYPTOTOOLS_INCLUDE_DIR}>
                        $<INSTALL_INTERFACE:>)
    endif()
endif()
