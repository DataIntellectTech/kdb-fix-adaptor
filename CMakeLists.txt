cmake_minimum_required(VERSION 3.2)

###################################################################################
# CONFIGURATION
###################################################################################
set(PROGRAM_NAME          "kdbfix")
set(PROGRAM_VER           "0.1.3")
set(PROGRAM_MAIN          "src/main.cxx")
set(QUICKFIX_VER          "1.14.3")
set(KX_VER                "3")
set(BINARY_NAME           "${PROGRAM_NAME}.${PROGRAM_VER}")

option(BUILD_BOOST        "build with the boost libraries available on the path"      ON)
option(BUILD_x86          "build a 32-bit binary instead of the default 64 bit one"   OFF)
option(BUILD_DEBUG        "build debug versions of the binaries with symbols"         OFF)

project(${BINARY_NAME} CXX C)

add_library(${BINARY_NAME} SHARED 
	"${CMAKE_SOURCE_DIR}/${PROGRAM_MAIN}" 
	"${CMAKE_SOURCE_DIR}/third_party/pugixml-1.7/src/pugixml.cpp")
target_link_libraries(${BINARY_NAME} "quickfix")

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_BINARY_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/third_party/kx/include")
include_directories("${CMAKE_SOURCE_DIR}/third_party/pugixml-1.7/src")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK -Wall -Werror -fno-strict-aliasing -std=c++11 -Os -march=native -DKXVER=${KX_VER} -fpic")

# Make sure that the build system doesn't add a 'lib' prefix to the shared library
set_target_properties(${BINARY_NAME} PROPERTIES PREFIX "")

execute_process(COMMAND
    "git" describe --match=NeVeRmAtCh --always --abbrev=40 --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHA1
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND
    "git" log -1 --format=%ad --date=local
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_DATE
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND
    "date"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE BUILD_DATE
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

# Generate a configuration file that will contain all of the build variables used by CMake
configure_file("${CMAKE_SOURCE_DIR}/config.h" "${CMAKE_BINARY_DIR}/include/config.h")
configure_file("${CMAKE_SOURCE_DIR}/README.md" "${CMAKE_BINARY_DIR}/README.md")
configure_file("${CMAKE_SOURCE_DIR}/LICENSE.md" "${CMAKE_BINARY_DIR}/LICENSE.md")
configure_file("${CMAKE_SOURCE_DIR}/src/config/fix.q" "${CMAKE_BINARY_DIR}/fix.q")

file(COPY "${CMAKE_SOURCE_DIR}/src/config/spec" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/src/config/log" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/src/config/sessions" DESTINATION "${CMAKE_BINARY_DIR}")

add_custom_target(build_package COMMAND
    ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_SOURCE_DIR}/${PROGRAM_NAME}-${PROGRAM_VER}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz"
    "${CMAKE_BINARY_DIR}/${BINARY_NAME}.so"
    "${CMAKE_BINARY_DIR}/README.md"
    "${CMAKE_BINARY_DIR}/LICENSE.md"
    "${CMAKE_BINARY_DIR}/spec"
    "${CMAKE_BINARY_DIR}/log"
    "${CMAKE_BINARY_DIR}/sessions"
    "${CMAKE_BINARY_DIR}/fix.q")
