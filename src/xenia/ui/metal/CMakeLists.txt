# Embed the current XeSL sources. The Metal driver compiles these at startup,
# so a separate Xcode Metal offline toolchain is not required.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(GLOB_RECURSE _metal_shader_inputs CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/xenia/gpu/shaders/*.xesl"
  "${PROJECT_SOURCE_DIR}/src/xenia/gpu/shaders/*.xesli"
  "${PROJECT_SOURCE_DIR}/src/xenia/gpu/shaders/*.metal"
  "${PROJECT_SOURCE_DIR}/src/xenia/ui/shaders/*.xesl"
  "${PROJECT_SOURCE_DIR}/src/xenia/ui/shaders/*.xesli")
set(_metal_generated "${PROJECT_BINARY_DIR}/generated")
set(_metal_stamp "${_metal_generated}/metal-shader-sources.stamp")
add_custom_command(OUTPUT "${_metal_stamp}"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/embed_shader_sources.py"
    --root "${PROJECT_SOURCE_DIR}" --output "${_metal_generated}"
  COMMAND ${CMAKE_COMMAND} -E touch "${_metal_stamp}"
  DEPENDS ${_metal_shader_inputs} "${CMAKE_CURRENT_SOURCE_DIR}/embed_shader_sources.py"
  VERBATIM)
add_custom_target(xenia-metal-shader-sources DEPENDS "${_metal_stamp}")

add_library(xenia-ui-metal STATIC
  metal_api.cc metal_gpu_completion_timeline.cc metal_provider.cc metal_util.cc
  metal_immediate_drawer.mm metal_presenter.mm)
add_dependencies(xenia-ui-metal xenia-metal-shader-sources)
target_include_directories(xenia-ui-metal PUBLIC "${_metal_generated}"
  "${PROJECT_SOURCE_DIR}/third_party/metal-cpp")
target_link_libraries(xenia-ui-metal PUBLIC xenia-base xenia-ui
  "-framework Metal" "-framework MetalFX" "-framework MetalKit"
  "-framework QuartzCore" "-framework Cocoa")
target_compile_options(xenia-ui-metal PUBLIC -fblocks)
xe_target_defaults(xenia-ui-metal)
