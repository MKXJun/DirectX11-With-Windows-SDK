cmake_minimum_required (VERSION 3.12)

add_subdirectory("17 Tree Billboard")
add_subdirectory("19 Meshes")
add_subdirectory("20 Instancing and Frustum Culling")
add_subdirectory("21 Picking")
add_subdirectory("22 Static Cube Mapping")
add_subdirectory("23 Dynamic Cube Mapping")
add_subdirectory("24 Render To Texture")
add_subdirectory("25 Normal Mapping")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
