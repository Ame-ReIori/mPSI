# set platform
if(MSVC)
  set(MPSI_CONFIG "x64-${CMAKE_BUILD_TYPE}")
elseif(APPLE)
  set(MPSI_CONFIG "osx")
else()
  set(MPSI_CONFIG "linux")
endif()

# set project compile type
if(NOT DEFINED MPSI_BUILE_TYPE)
  if(DEFINED CMAKE_BUILE_TYPE)
    set(MPSI_BUILD_TYPE ${CMAKE_BUILD_TYPE})
  else()
    set(MPSI_BUILD_TYPE "Release")
  endif()
endif()

# set config name, may be used in msvc
if(MSVC)
  set(MPSI_CONFIG_NAME "${MPSI_BUILD_TYPE}")
  if("${MPSI_CONFIG_NAME}" STREQUAL "RelWithDebInfo")
    set(MPSI_CONFIG_NAME "Release")
  endif()
  set(MPSI_CONFIG "x64-${MPSI_CONFIG_NAME}")
elseif(APPLE)
  set(MPSI_CONFIG "osx")
else()
  set(MPSI_CONFIG "linux")
endif()

# set project build dir
if(NOT MPSI_BUILD_DIR)
  set(MPSI_BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../out/build/${MPSI_CONFIG}")
else()
  message(STATUS "MPSI_BUILD_DIR preset to ${MPSI_BUILD_DIR}")
endif()

# set thirdparty dir
set(MPSI_INSOURCE_FIND_DEPS (EXISTS ${CMAKE_CURRENT_LIST_DIR}/FindBuildDir.cmake))
if(NOT DEFINED MPSI_THIRDPARYT_HINT)
  if(MPSI_INSOURCE_FIND_DEPS)
    set(MPSI_THIRDPARTY_HINT "${CMAKE_CURRENT_LIST_DIR}/../out/install/${MPSI_CONFIG}")
    if(NOT DEFINED MPSI_THIRDPARTY_INSTALL_PREFIX)
      set(MPSI_THIRDPARTY_INSTALL_PREFIX ${MPSI_THIRDPARTY_HINT})
    endif()
  else()
    set(MPSI_THIRDPARTY_HINT "${CMAKE_CURRENT_LIST_DIR}/../../..")
  endif()
endif()

# set thirdparty clone dir
if(NOT MPSI_THIRDPARTY_CLONE_DIR)
  set(MPSI_THIRDPARTY_CLONE_DIR "${CMAKE_CURRENT_LIST_DIR}/../out")
endif()

# create project build dir
if(NOT EXISTS "${MPSI_BUILD_DIR}")
  execute_process(COMMAND mkdir -p "${MPSI_BUILD_DIR}")
endif()

# put your other build dir here
set(CMAKE_PREFIX_PATH "${MPSI_BUILD_DIR};${CMAKE_PREFIX_PATH}")
