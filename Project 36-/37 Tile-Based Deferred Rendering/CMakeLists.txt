cmake_minimum_required (VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(UNICODE _UNICODE)
if (WIN7_SYSTEM_SUPPORT MATCHES ON)
	add_compile_definitions(_WIN32_WINNT=0x601)
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
file(COPY Shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

aux_source_directory(. DIR_SRCS)
file(GLOB SHADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/Shaders/*.hlsl)
file(GLOB HEADER_FILES ./*.h)
file(GLOB COMMON_FILES ../../Common/*)

foreach(SHADER_FILE ${SHADER_FILES})
	set_source_files_properties(${SHADER_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

add_executable(37_Tile_Based_Deferred_Rendering WIN32 ${DIR_SRCS} ${HEADER_FILES} ${SHADER_FILES} ${COMMON_FILES})
target_link_libraries(37_Tile_Based_Deferred_Rendering d3d11.lib dxgi.lib dxguid.lib D3DCompiler.lib winmm.lib)

# Assimp
target_link_libraries(37_Tile_Based_Deferred_Rendering "${Assimp_INSTALLED_DIR}/lib/assimp-vc${MSVC_TOOLSET_VERSION}-mt$<$<CONFIG:Debug>:d>.lib")
include_directories(${Assimp_INSTALLED_DIR}/include)
set_target_properties(37_Tile_Based_Deferred_Rendering PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${Assimp_INSTALLED_DIR}/bin")

# ImGui
target_link_libraries(37_Tile_Based_Deferred_Rendering ImGui)
file(COPY imgui.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Common
include_directories(../../Common)

source_group("Common" FILES ${COMMON_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
set_target_properties(37_Tile_Based_Deferred_Rendering PROPERTIES OUTPUT_NAME "37 Tile-Based Deferred Rendering")

set_target_properties(37_Tile_Based_Deferred_Rendering PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(37_Tile_Based_Deferred_Rendering PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(37_Tile_Based_Deferred_Rendering PROPERTIES FOLDER "Project 36-")
