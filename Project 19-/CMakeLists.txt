cmake_minimum_required(VERSION 3.14)

add_subdirectory("19 Meshes")
add_subdirectory("20 Instancing and Frustum Culling")
add_subdirectory("21 Picking")
add_subdirectory("22 Static Cube Mapping")
add_subdirectory("23 Dynamic Cube Mapping")
add_subdirectory("24 Render To Texture")
add_subdirectory("25 Normal Mapping")
add_subdirectory("26 Compute Shader Beginning")
add_subdirectory("27 Bitonic Sort")
add_subdirectory("28 Waves")
add_subdirectory("29 OIT")
add_subdirectory("30 Blur and Sobel")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_subdirectory("Common")
