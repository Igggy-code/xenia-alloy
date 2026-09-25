# Native Apple Silicon target, including when CMake runs under Rosetta.
# Canary currently selects the CPU backend using CMAKE_SYSTEM_PROCESSOR,
# independently of CMAKE_OSX_ARCHITECTURES. Keep the two in agreement.
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "macOS target architecture" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS version")

# Run xcrun natively: an Intel CMake may otherwise launch the Intel slice of
# /usr/bin/cc, which cannot load an arm64-only Command Line Tools libxcrun.
foreach(_xe_language C CXX OBJC OBJCXX)
  if(NOT CMAKE_${_xe_language}_COMPILER)
    if(_xe_language MATCHES "CXX$")
      set(_xe_compiler_name clang++)
    else()
      set(_xe_compiler_name clang)
    endif()
    execute_process(
      COMMAND /usr/bin/arch -arm64 /usr/bin/xcrun --find ${_xe_compiler_name}
      RESULT_VARIABLE _xe_find_result
      OUTPUT_VARIABLE _xe_compiler_path
      ERROR_VARIABLE _xe_find_error
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _xe_find_result EQUAL 0)
      message(FATAL_ERROR "Cannot find native Apple compiler: ${_xe_find_error}")
    endif()
    set(CMAKE_${_xe_language}_COMPILER "${_xe_compiler_path}" CACHE FILEPATH
        "Native Apple compiler for ${_xe_language}")
  endif()
endforeach()

# /usr/bin/git is another xcrun shim affected by an Intel CMake process.
execute_process(COMMAND /usr/bin/arch -arm64 /usr/bin/xcrun --find git
  OUTPUT_VARIABLE _xe_git_path OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
set(GIT_EXECUTABLE "${_xe_git_path}" CACHE FILEPATH "Native Apple Git" FORCE)
unset(_xe_git_path)

unset(_xe_language)
unset(_xe_compiler_name)
unset(_xe_compiler_path)
unset(_xe_find_result)
unset(_xe_find_error)
