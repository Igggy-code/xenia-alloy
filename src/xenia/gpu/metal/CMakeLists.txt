add_library(xenia-gpu-metal STATIC
  metal_command_processor.cc metal_graphics_system.cc metal_heap_pool.cc
  metal_primitive_processor.cc metal_render_target_cache.cc metal_shared_memory.cc
  metal_shader_cache.cc metal_texture_cache.cc msl_shader.cc)
add_dependencies(xenia-gpu-metal xenia-metal-shader-sources)
target_include_directories(xenia-gpu-metal PUBLIC
  "${PROJECT_BINARY_DIR}/generated"
  "${PROJECT_SOURCE_DIR}/third_party/metal-cpp"
  "${PROJECT_SOURCE_DIR}/third_party/SPIRV-Cross"
  "${PROJECT_SOURCE_DIR}/third_party/glslang")
target_link_libraries(xenia-gpu-metal PUBLIC xenia-base xenia-gpu xenia-ui-metal
  fmt glslang-spirv spirv-cross "-framework Metal" "-framework MetalKit")
# Enable Apple's converter when its SDK and the DXBC conversion libraries have
# been configured by the parent project. The source-to-MSL path stays available.
if(TARGET metalirconverter AND TARGET dxilconv)
  target_sources(xenia-gpu-metal PRIVATE dxbc_to_dxil_converter.cc
    ir_runtime_impl.mm metal_geometry_shader.cc metal_shader.cc
    metal_shader_converter.cc)
  target_compile_definitions(xenia-gpu-metal PUBLIC
    METAL_SHADER_CONVERTER_AVAILABLE=1 IR_RUNTIME_METALCPP)
  target_include_directories(xenia-gpu-metal PRIVATE
    "${PROJECT_SOURCE_DIR}/third_party/metal-shader-converter/include"
    "${PROJECT_SOURCE_DIR}/third_party/DirectXShaderCompiler-mac/include"
    "${PROJECT_SOURCE_DIR}/third_party/DirectXShaderCompiler-mac/projects/dxilconv/include")
  target_link_libraries(xenia-gpu-metal PUBLIC metalirconverter dxilconv)
else()
  target_compile_definitions(xenia-gpu-metal PUBLIC METAL_SHADER_CONVERTER_AVAILABLE=0)
  message(STATUS "Metal: using experimental SPIRV-Cross shader translation")
endif()
xe_target_defaults(xenia-gpu-metal)
