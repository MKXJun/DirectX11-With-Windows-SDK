cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT SET_UP_CONFIGURATIONS_DONE)
    set(SET_UP_CONFIGURATIONS_DONE TRUE)

    # No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
    # Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(isMultiConfig)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE) 
    else()
        if(NOT CMAKE_BUILD_TYPE)
            message("Defaulting to release build.")
            set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
        endif()
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
        # set the valid options for cmake-gui drop-down list
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;Profile")
    endif()
    # now set up the Profile configuration
    set(CMAKE_C_FLAGS_PROFILE "...")
    set(CMAKE_CXX_FLAGS_PROFILE "...")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "...")
endif()

project("DirectX11 With Windows SDK")

option(USE_IMGUI "Replace Direct2D UI with ImGui" ON)
option(WIN7_SYSTEM_SUPPORT "Windows7 users need to select this option!" OFF)

set(Assimp_INSTALLED_DIR "" CACHE STRING "The Assimp Library where you installed for Project 36-.")
if (NOT "${Assimp_INSTALLED_DIR}" STREQUAL "")
    find_package(Assimp REQUIRED)
endif()

add_subdirectory("ImGui")
add_subdirectory("Project 01-09")
add_subdirectory("Project 10-16")
add_subdirectory("Project 17-25")
add_subdirectory("Project 26-30")
add_subdirectory("Project 31-35")
add_subdirectory("Project 36-")

set_target_properties(ImGui PROPERTIES FOLDER "ImGui")

