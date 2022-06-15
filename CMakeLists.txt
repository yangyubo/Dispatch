cmake_minimum_required(VERSION 3.0)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

if(POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

################################################################################
## Dispatch++
################################################################################

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/scripts/version.txt ver)
project(Dispatch++ VERSION ${ver} LANGUAGES CXX)

# Determine if Dispatch++ is built as a subproject (using add_subdirectory) or if it is the main project.
set(MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

option(DISPATCH_WITH_TESTS  "Build tests/examples" ${MAIN_PROJECT})

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Dispatch++ requires Clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(STATUS "Apple Clang compiler")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(${PROJECT_NAME} INTERFACE -ldispatch -lBlocksRuntime)
    include(local.cmake OPTIONAL)
else()
    message(FATAL_ERROR "Dispatch++ reqires Apple Clang or LLVM Clang compiler")
endif()

if(NOT CMAKE_VERSION VERSION_LESS 3.8)
    target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
endif()

target_compile_options(${PROJECT_NAME} INTERFACE -Wno-nullability-completeness)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################
## TESTS/EXAMPLES/HELPERS
################################################################################

if(MAIN_PROJECT AND DISPATCH_WITH_TESTS)
    add_subdirectory(tests)
endif()
