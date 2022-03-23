cmake_minimum_required(VERSION 3.14)
include(ExternalProject)

###################################################################################
# CONFIGURATION
###################################################################################
set(PROGRAM_NAME          "kdbfix")
set(PROGRAM_VER           "0.5.2")
set(PROGRAM_MAIN          "src/main.cxx")
set(BINARY_NAME           "${PROGRAM_NAME}.${PROGRAM_VER}")

set(QUICKFIX_GITHUB_TAG   "v1.15.1")
set(PUGIXML_GITHUB_TAB    "v1.12.1")

option(BUILD_DEBUG        "build debug versions of the binaries with symbols"         OFF)

project(${BINARY_NAME} CXX)

add_definitions(-DKXVER=3 -Wno-deprecated -Wno-unused -Denable-static)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

ExternalProject_Add(QuickFix
		GIT_REPOSITORY
		"https://github.com/quickfix/quickfix.git"
		GIT_TAG
		"${QUICKFIX_GITHUB_TAG}"
		CMAKE_ARGS
		"-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		"-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}")

ExternalProject_Add(PugiXML
		GIT_REPOSITORY
		"https://github.com/zeux/pugixml.git"
		GIT_TAG
		"${PUGIXML_GITHUB_TAG}"
		CMAKE_ARGS
		"-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		"-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}")

include_directories(
		"${CMAKE_SOURCE_DIR}/include"
		"${CMAKE_BINARY_DIR}/include"
		"${CMAKE_SOURCE_DIR}/third_party/kx/include")

link_directories(
		"${CMAKE_BINARY_DIR}/lib")

add_library(${BINARY_NAME} SHARED
		"${CMAKE_SOURCE_DIR}/${PROGRAM_MAIN}")

target_link_libraries(${BINARY_NAME} PRIVATE
		"quickfix"
		"Threads::Threads")

# We need to build PugiXML and QuickFix before we can build the rest of the project...
add_dependencies("${PROJECT_NAME}"
		PugiXML
		QuickFix)

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
