cmake_minimum_required(VERSION 3.14)

add_compile_definitions(UNICODE _UNICODE)
if (WIN7_SYSTEM_SUPPORT MATCHES ON)
    add_compile_definitions(_WIN32_WINNT=0x601)
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

aux_source_directory(. DIR_SRCS)
file(GLOB HEADER_FILES ./*.h)

add_executable(01_DirectX11_Initialization WIN32 ${DIR_SRCS} ${HEADER_FILES})
target_link_libraries(01_DirectX11_Initialization d3d11.lib dxgi.lib dxguid.lib D3DCompiler.lib d2d1.lib dwrite.lib winmm.lib)

set_target_properties(01_DirectX11_Initialization PROPERTIES OUTPUT_NAME "01 DirectX11 Initialization")

set_target_properties(01_DirectX11_Initialization PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(01_DirectX11_Initialization PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(01_DirectX11_Initialization PROPERTIES FOLDER "Project 01-09")


