cmake_minimum_required (VERSION 3.12)

aux_source_directory(. DIR_SRCS)
add_library(ImGui STATIC ${DIR_SRCS})

target_include_directories(ImGui PUBLIC .)
