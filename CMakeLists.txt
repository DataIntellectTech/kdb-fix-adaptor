cmake_minimum_required(VERSION 3.14)
include(ExternalProject)

###################################################################################
# CONFIGURATION
###################################################################################
set(PROGRAM_NAME          "kdbfix")
set(PROGRAM_MAJOR         "0")
set(PROGRAM_MINOR         "6")
set(PROGRAM_PATCH         "0")
set(PROGRAM_VER           "${PROGRAM_MAJOR}.${PROGRAM_MINOR}.${PROGRAM_PATCH}")
set(PROGRAM_MAIN          "src/cpp/main.cxx")
set(BINARY_NAME           "${PROGRAM_NAME}")

project(${BINARY_NAME} CXX)

add_definitions(-DKXVER=3 -Wno-deprecated -Wno-unused)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

option(USE_SYSTEM_QUICKFIX "Use a system installed version of QuickFix instead of the bundled version" OFF)
option(USE_32BIT "Create 32-bit binaries instead of the default 64-bit" OFF)
option(USE_GENERIC_FIX_GROUPS "Enable generic FIX repeated groups processing" ON)

# This option assumes that the host has a working multilib setup installed
if(USE_32BIT)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
	set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -m32")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
	set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} -m32")
endif(USE_32BIT)

###################################################################################
# THIRD PARTY DEPENDENCIES
###################################################################################

set(QUICKFIX_GITHUB_TAG   "5ee692dd5739f77430cd91e3b4814c33b638fdb4") # git ls-remote https://github.com/quickfix/quickfix v1.15.1
set(PUGIXML_GITHUB_TAG    "dd50fa5b45ab8d58d6c27566c2eaf04a8b7e5841") # git ls-remote https://github.com/zeux/pugixml v1.12.1
set(SPDLOG_GITHUB_TAG     "eb3220622e73a4889eee355ffa37972b3cac3df5") # git ls-remote https://github.com/gabime/spdlog v1.9.2

ExternalProject_Add(QuickFix
		GIT_REPOSITORY
		    "https://github.com/quickfix/quickfix.git"
		GIT_TAG
		    "${QUICKFIX_GITHUB_TAG}"
		CMAKE_ARGS
		    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
		    "-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}"
		    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/l64"
		    "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
		    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		    "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}"
		    "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -Wno-deprecated")

ExternalProject_Add(PugiXML
		GIT_REPOSITORY
		    "https://github.com/zeux/pugixml.git"
		GIT_TAG
		    "${PUGIXML_GITHUB_TAG}"
		CMAKE_ARGS
		    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
		    "-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}"
		    "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
		    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		    "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}"
		    "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")

ExternalProject_Add(SpdLog
		GIT_REPOSITORY
		    "https://github.com/gabime/spdlog.git"
		GIT_TAG
		    "${SPDLOG_GITHUB_TAG}"
		CMAKE_ARGS
		    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
		    "-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}"
		    "-DSPDLOG_BUILD_TESTS=OFF"
		    "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
		    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		    "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}"
		    "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")

find_package(Threads REQUIRED)

###################################################################################
# THIRD PARTY DEPENDENCIES
###################################################################################
include_directories(
		"${CMAKE_SOURCE_DIR}/include"
		"${CMAKE_BINARY_DIR}/include"
		"${CMAKE_SOURCE_DIR}/third_party/kx/c/c")

link_directories(
		"${CMAKE_BINARY_DIR}/l64"
        "${CMAKE_BINARY_DIR}")

add_library(${BINARY_NAME} SHARED
		"${CMAKE_SOURCE_DIR}/${PROGRAM_MAIN}")

target_link_libraries(${BINARY_NAME} PRIVATE
		"quickfix"
		"Threads::Threads")

# We want to look for the QuickFix.so file next to the kdbfix shared library before we search system paths/LD_LIBRARY_PATH/QHOME...
# This will help ensure we are linking against the expected QuickFix version instead of a system version where possible.
SET(CMAKE_EXE_LINKER_FLAGS
		"${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,$ORIGIN,$ORIGIN/l64,$ORIGIN/lib,$ORIGIN/lib64")

# We need to build SpdLog, PugiXML and QuickFix before we can build the rest of the project...
add_dependencies("${PROJECT_NAME}"
		SpdLog
		PugiXML
		QuickFix)

set_target_properties(${BINARY_NAME} PROPERTIES
		VERSION "${PROGRAM_VER}"
		SOVERSION "${PROGRAM_MAJOR}"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/l64")

execute_process(COMMAND
		"date"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE BUILD_DATE
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

# Generate a configuration file that will contain all of the build variables used by CMake
configure_file("${CMAKE_SOURCE_DIR}/src/cpp/config.h.in" "${CMAKE_BINARY_DIR}/include/config.h")
configure_file("${CMAKE_SOURCE_DIR}/README.md" "${CMAKE_BINARY_DIR}/README.md")
configure_file("${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE")
configure_file("${CMAKE_SOURCE_DIR}/CHANGELOG.md" "${CMAKE_BINARY_DIR}/CHANGELOG.md")
configure_file("${CMAKE_SOURCE_DIR}/src/q/fix.q" "${CMAKE_BINARY_DIR}/fix.q")
configure_file("${CMAKE_SOURCE_DIR}/src/q/example.q" "${CMAKE_BINARY_DIR}/example.q")

file(COPY "${CMAKE_SOURCE_DIR}/config/sessions" DESTINATION "${CMAKE_BINARY_DIR}")

# Copy the latest spec files that are shipped with the QuickFix into the build directory. This will
# prevent issues when the FIX spec files are changed in quickfix and don't align with the old versions
# that we have checked into our own repository.
add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_BINARY_DIR}/QuickFix-prefix/src/QuickFix/spec
		${CMAKE_CURRENT_BINARY_DIR}/spec)

add_custom_target(build_package COMMAND
		${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_SOURCE_DIR}/${PROGRAM_NAME}-${PROGRAM_VER}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}.tar.gz"
		"${CMAKE_BINARY_DIR}/README.md"
		"${CMAKE_BINARY_DIR}/LICENSE"
		"${CMAKE_BINARY_DIR}/CHANGELOG.md"
		"${CMAKE_BINARY_DIR}/spec"
		"${CMAKE_BINARY_DIR}/sessions"
		"${CMAKE_BINARY_DIR}/l64"
		"${CMAKE_BINARY_DIR}/fix.q")

add_dependencies(build_package
		${BINARY_NAME})