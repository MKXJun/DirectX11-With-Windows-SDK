cmake_minimum_required(VERSION 3.14)

include(FetchContent)

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

option(WIN7_SYSTEM_SUPPORT "Windows7 users need to select this option!" OFF)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

#
# Assimp
#
set(ASSIMP_BUILD_ZLIB ON)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assimp/CMakeLists.txt")
    add_subdirectory("assimp")
else()
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.2.4
    )
    FetchContent_MakeAvailable(assimp)
endif()
set_target_properties(assimp PROPERTIES FOLDER "Assimp")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Assimp")
set_target_properties(zlibstatic PROPERTIES FOLDER "Assimp")

#
# ImGui(modified)
#
add_subdirectory("ImGui")
set_target_properties(ImGui PROPERTIES FOLDER "ImGui")

add_subdirectory("Project 01-09")
add_subdirectory("Project 10-17")
add_subdirectory("Project 19-")
add_subdirectory("Project Archive")



