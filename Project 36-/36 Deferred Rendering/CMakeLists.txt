cmake_minimum_required (VERSION 3.12)

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

add_executable(36_Deferred_Rendering WIN32 ${DIR_SRCS} ${HEADER_FILES} ${SHADER_FILES} ${COMMON_FILES})
target_link_libraries(36_Deferred_Rendering d3d11.lib dxgi.lib dxguid.lib D3DCompiler.lib winmm.lib)

include_directories(../../ImGui)
include_directories(../../Common)

aux_source_directory(../../ImGui IMGUI_SRCS)
file(GLOB IMGUI_HEADER_FILES ../../ImGui/*.h)
file(COPY imgui.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_sources(36_Deferred_Rendering PRIVATE ${IMGUI_SRCS} ${IMGUI_HEADER_FILES})
source_group("ImGui" FILES ${IMGUI_SRCS} ${IMGUI_HEADER_FILES})
source_group("Common" FILES ${COMMON_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
set_target_properties(36_Deferred_Rendering PROPERTIES OUTPUT_NAME "36 Deferred Rendering")

set_target_properties(36_Deferred_Rendering PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(36_Deferred_Rendering PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(36_Deferred_Rendering PROPERTIES FOLDER "Project 36-")
