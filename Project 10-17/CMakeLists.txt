cmake_minimum_required(VERSION 3.14)

add_subdirectory("10 Camera")
add_subdirectory("11 Blending")
add_subdirectory("12 Depth and Stenciling")
add_subdirectory("13 Living Without FX11")
add_subdirectory("14 Depth Test")
add_subdirectory("15 Geometry Shader Beginning")
add_subdirectory("16 Stream Output")
add_subdirectory("17 Tree Billboard")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
