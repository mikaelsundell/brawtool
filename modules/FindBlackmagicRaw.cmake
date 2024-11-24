# FindBlackmagicRaw.cmake
#
# This CMake find module attempts to find the Blackmagic Raw API and its dispatch file.
# It provides the following variables:
#   BlackmagicRaw_FOUND         - True if the API and libraries are found
#   BlackmagicRaw_INCLUDE_DIRS  - Where the BlackmagicRawAPI.h file is located
#   BlackmagicRaw_LIBRARIES     - The libraries to link against
#   BlackmagicRaw_DISPATCH      - The path to BlackmagicRawAPIDispatch.cpp

find_path (BlackmagicRaw_INCLUDE_DIRS 
           NAMES BlackmagicRawAPI.h
           PATH_SUFFIXES Include
)

find_file (BlackmagicRaw_SOURCES
          NAMES BlackmagicRawAPIDispatch.cpp
          PATH_SUFFIXES Include
)

find_library (BlackmagicRaw_LIBRARIES 
          NAMES BlackmagicRawAPI
          PATH_SUFFIXES Libraries
)

if (BlackmagicRaw_LIBRARIES)
    get_filename_component( BlackmagicRaw_LIBRARY_PATH "${BlackmagicRaw_LIBRARIES}" DIRECTORY CACHE)
endif()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args ( 
        BlackmagicRaw DEFAULT_MSG
        BlackmagicRaw_INCLUDE_DIRS BlackmagicRaw_LIBRARY_PATH BlackmagicRaw_SOURCES BlackmagicRaw_LIBRARIES 
 )

if (BlackmagicRaw_FOUND)
  message (STATUS "Found BlackmagicRaw: include at ${BlackmagicRaw_INCLUDE_DIRS}, library at ${BlackmagicRaw_LIBRARY_PATH}, sources ${BlackmagicRaw_SOURCES}, library at ${BlackmagicRaw_LIBRARIES}")
else ()
  message (FATAL_ERROR "Could not find BlackmagicRaw")
endif ()

mark_as_advanced (BlackmagicRaw_INCLUDE_DIRS BlackmagicRaw_SOURCES BlackmagicRaw_LIBRARIES)
