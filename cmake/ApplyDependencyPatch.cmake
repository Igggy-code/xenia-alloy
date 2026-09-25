execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${PATCH_FILE}"
  WORKING_DIRECTORY "${SOURCE_DIR}" RESULT_VARIABLE already_applied
  OUTPUT_QUIET ERROR_QUIET)
if(already_applied EQUAL 0)
  return()
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" apply --check "${PATCH_FILE}"
  WORKING_DIRECTORY "${SOURCE_DIR}" RESULT_VARIABLE can_apply)
if(NOT can_apply EQUAL 0)
  message(FATAL_ERROR "Dependency patch does not match ${SOURCE_DIR}: ${PATCH_FILE}")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" apply "${PATCH_FILE}"
  WORKING_DIRECTORY "${SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
