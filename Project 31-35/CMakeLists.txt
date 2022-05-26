cmake_minimum_required(VERSION 3.14)

add_subdirectory("31 Shadow Mapping")
add_subdirectory("32 SSAO")
add_subdirectory("33 Tessellation")
add_subdirectory("34 Displacement Mapping")
add_subdirectory("35 Particle System")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
