function(RUN)
  cmake_parse_arguments(
    PARSED_ARGS # prefix of parameters
    "" # list of names of the boolean arguments (only defined ones will be true)
    "WD" # list of names of mono-valued arguments
    "CMD;NAME" # list of names of multi-valued arguments (output variables are lists)
    ${ARGN} # arguments of the function to parse, here we take the all original ones
  )
  message("${PARSED_ARGS_NAME}")

  execute_process(
    COMMAND ${PARSED_ARGS_CMD}
    WORKING_DIRECTORY ${PARSED_ARGS_WD}
    RESULT_VARIABLE RESULT
    COMMAND_ECHO STDOUT
  )

  if(RESULT)
    message(FATAL_ERROR "${PARSED_ARGS_NAME} failed (${RESULT}).")
  endif()
endfunction()
