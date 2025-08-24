set(GIT_REPOSITORY https://github.com/Visa-Research/volepsi.git)
set(GIT_TAG        ed943f5f814591cdf864777c73b7bc9e7526c1a8)
set(CLONE_DIR      "${MPSI_THIRDPARTY_CLONE_DIR}/volepsi")
set(BUILD_DIR      "${CLONE_DIR}/build/${MPSI_CONFIG}")
set(CONFIG         --config ${CMAKE_BUILD_TYPE})
set(LOG_FILE       "${CMAKE_CURRENT_LIST_DIR}/log-volepsi.txt")

include("${CMAKE_CURRENT_LIST_DIR}/fetch.cmake")

if(NOT EXISTS ${BUILD_DIR} OR NOT VOLEPSI_FOUND)
  find_program(GIT git REQUIRED)
  set(DOWNLOAD_CMD  ${GIT} clone ${GIT_REPOSITORY})
  set(CHECKOUT_CMD  ${GIT} checkout ${GIT_TAG})
  set(SUBMODULE_CMD ${GIT} submodule update --init --recursive)
  set(CONFIGURE_CMD ${CMAKE_COMMAND} -S ${CLONE_DIR} -B ${BUILD_DIR}
                                     -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                                     -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                                     -DVOLE_PSI_ENABLE_BOOST=ON
                                     -DVOLE_PSI_ENABLE_RELIC=true
                                     -DVOLE_PSI_ENABLE_SODIUM=false
                                     -DSUDO_FETCH=OFF
                                     -DFETCH_AUTO=ON
                                     -DVOLE_PSI_NO_SYSTEM_PATH=true
                                     -DPARALLEL_FETCH=56)
  set(BUILD_CMD     ${CMAKE_COMMAND} --build ${BUILD_DIR} --parallel 56 ${CONFIG})
  set(INSTALL_CMD   ${CMAKE_COMMAND} --install ${BUILD_DIR} ${CONFIG} --prefix ${MPSI_THIRDPARTY_INSTALL_PREFIX})
  message("============= Building Volepsi =============")
  if(NOT EXISTS ${CLONE_DIR})
    run(NAME "Cloning ${GIT_REPOSITORY}" CMD ${DOWNLOAD_CMD} WD ${MPSI_THIRDPARTY_CLONE_DIR})
  endif()
  run(NAME "Checkout ${GIT_TAG} "     CMD ${CHECKOUT_CMD}   WD ${CLONE_DIR})
  if(EXISTS ${CLONE_DIR}/.gitmodules)
    run(NAME "Submodule updating" CMD ${SUBMODULE_CMD} WD ${CLONE_DIR})
  endif()
  run(NAME "Configure"              CMD ${CONFIGURE_CMD}  WD ${CLONE_DIR})
  run(NAME "Build"                  CMD ${BUILD_CMD}      WD ${CLONE_DIR})
  run(NAME "Install"                CMD ${INSTALL_CMD}    WD ${CLONE_DIR})
  message("log ${LOG_FILE}\n==========================================")
else()
  message("volepsi already fetched.")
endif()

install(CODE "
  if(NOT CMAKE_INSTALL_PREFIX STREQUAL \"${MPSI_THIRDPARTY_INSTALL_PREFIX}\")
    execute_process(
      COMMAND ${SUDO} \${CMAKE_COMMAND} --install \"${BUILD_DIR}\" ${CONFIG}  --prefix \${CMAKE_INSTALL_PREFIX}
      WORKING_DIRECTORY ${CLONE_DIR}
      RESULT_VARIABLE RESULT
      COMMAND_ECHO STDOUT
    )
  endif()
")
