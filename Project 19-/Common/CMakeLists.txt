cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_definitions(UNICODE _UNICODE)

aux_source_directory(. COMMON_SRCS)
file(GLOB COMMON_HEADERS ./*.h)
add_library(Common STATIC ${COMMON_SRCS} ${COMMON_HEADERS})

# ImGui
target_link_libraries(Common ImGui)

# Assimp
target_link_libraries(Common assimp)

target_include_directories(Common PUBLIC .)

set_target_properties(Common PROPERTIES FOLDER "Project 19-")
