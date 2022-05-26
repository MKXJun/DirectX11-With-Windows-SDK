cmake_minimum_required(VERSION 3.14)

add_subdirectory("36 Deferred Rendering")
add_subdirectory("37 Tile-Based Deferred Rendering")
add_subdirectory("38 Cascaded Shadow Mapping")
add_subdirectory("39 VSM and ESM")
add_subdirectory("40 FXAA")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
