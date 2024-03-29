# Copyright 2024-present Contributors to the brawtool project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/brawtool

cmake_minimum_required( VERSION 3.23 )
set( project_name "brawtool" )

project( "${project_name} project" )

# cmake
list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules" )
list( APPEND CMAKE_PREFIX_PATH "/Applications/Blackmagic RAW/Blackmagic RAW SDK/Mac" )

# package
find_package( OpenImageIO CONFIG REQUIRED )
find_package( OpenColorIO CONFIG REQUIRED )
find_package( Boost CONFIG REQUIRED COMPONENTS filesystem )
find_package( BlackmagicRaw REQUIRED )

# project
add_executable( ${project_name} "brawtool.cpp" ${BlackmagicRaw_SOURCES} )

# definitions
add_definitions(-DBlackmagicRaw_LIBRARY_PATH="${BlackmagicRaw_LIBRARY_PATH}")

target_include_directories(${project_name}
    PRIVATE 
        ${BlackmagicRaw_INCLUDE_DIRS}
)

target_link_libraries(${project_name}
    PRIVATE
        OpenImageIO::OpenImageIO
        OpenColorIO::OpenColorIO
        Boost::filesystem
        ${BlackmagicRaw_LIBRARIES}
        "-framework CoreFoundation"
)

set_property(TARGET ${project_name} PROPERTY CXX_STANDARD 14)

add_custom_command(
    TARGET ${project_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/fonts
        $<TARGET_FILE_DIR:${project_name}>/fonts
)

add_custom_command(
    TARGET ${project_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${project_name}>/resources
)
