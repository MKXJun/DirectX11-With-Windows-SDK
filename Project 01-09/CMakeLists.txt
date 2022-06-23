cmake_minimum_required(VERSION 3.14)

add_subdirectory("01 DirectX11 Initialization")
add_subdirectory("02 Rendering a Triangle")
add_subdirectory("03 Rendering a Cube")
add_subdirectory("06 Use ImGui")
add_subdirectory("07 Lighting")
add_subdirectory("08 Direct2D and Direct3D Interoperability")
add_subdirectory("09 Texture Mapping")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
