cmake_minimum_required(VERSION 3.14)

add_subdirectory("26 Compute Shader Beginning")
add_subdirectory("27 Bitonic Sort")
add_subdirectory("28 Waves")
add_subdirectory("29 OIT")
add_subdirectory("30 Blur and Sobel")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
