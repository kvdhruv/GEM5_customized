cmake_minimum_required (VERSION 3.5)
project (DSENT)

include_directories(${CMAKE_SOURCE_DIR})

# Find Python 3 Development packages
find_package(Python3 COMPONENTS Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})

file(GLOB_RECURSE SRCS "*.cc")

# Build as a shared module for Python to import
add_library(dsent MODULE ${SRCS})
target_link_libraries(dsent PRIVATE ${Python3_LIBRARIES})

set_target_properties(dsent PROPERTIES 
    PREFIX "" 
    SUFFIX ".so"
    COMPILE_FLAGS "-std=c++11"
)
