cmake_minimum_required(VERSION 3.14)

aux_source_directory(. IMGUI_DIR_SRCS)
add_library(ImGui STATIC ${IMGUI_DIR_SRCS})

target_include_directories(ImGui PUBLIC .)
