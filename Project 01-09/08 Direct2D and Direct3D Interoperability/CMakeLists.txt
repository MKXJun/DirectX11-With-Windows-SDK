cmake_minimum_required (VERSION 3.8)

add_compile_definitions(UNICODE _UNICODE)
if (WIN7_SYSTEM_SUPPORT MATCHES ON)
	add_compile_definitions(_WIN32_WINNT=0x601)
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

aux_source_directory(. DIR_SRCS)

add_executable(08_Direct2D_and_Direct3D_Interoperability WIN32 ${DIR_SRCS})
set_target_properties(08_Direct2D_and_Direct3D_Interoperability PROPERTIES OUTPUT_NAME "08 Direct2D and Direct3D Interoperability")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/HLSL)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/HLSL DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
