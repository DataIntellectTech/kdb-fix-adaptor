#################################################################################
# CMake build script for the FIX shared library for kdb+
#
# Builds the shared library and associated tests for the library. The tests can be
# executed by running the runtests executable from the bin/ directory.
#
cmake_minimum_required(VERSION 2.8)

option(BUILD_TESTS "Setup the c++ & kdb+ tests in the bin/ directory" ON)
option(BUILD_LIBRARY "Build the shared library & relevant scripts" ON)
option(REPLACE_SCRIPTS "Copy the scripts in the src/ directory to bin/ on each install" ON)

project(fixengine CXX C)
find_package(Threads)

include_directories("${CMAKE_SOURCE_DIR}/include")
link_directories("${CMAKE_SOURCE_DIR}/lib")
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/lib")

#################################################################################
# SHARED LIBRARY
#
# Build the shared library and copy the required scripts/third party binaries into
# the bin/ folder. The name of the resulting library is stripped of its lib prefix
# so that the run.q script will work without modification on all platforms.
#
if(BUILD_LIBRARY)
    file(COPY "src/q/fix.q" DESTINATION "${CMAKE_SOURCE_DIR}/bin")
    file(COPY "src/q/fixenums.q" DESTINATION "${CMAKE_SOURCE_DIR}/bin")
    file(COPY "src/run" DESTINATION "${CMAKE_SOURCE_DIR}/bin")
    file(COPY "src/config" DESTINATION "${CMAKE_SOURCE_DIR}/bin")
    file(COPY "lib/libquickfix.so.16" DESTINATION "${CMAKE_SOURCE_DIR}/bin/lib")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DKXVER=3")

    add_library(fixengine SHARED "src/cpp/FixEngine.cxx" "src/cpp/main.cxx")
    target_link_libraries(fixengine quickfix)
    set_target_properties(fixengine PROPERTIES PREFIX "")
endif()

#################################################################################
# TESTS
#
# Build the testing code that uses Google Test for the C++ side and k4unit for the
# kdb+ side of the project. No executables are built for the kdb+ side so a script
# is copied over which will launch both sets of tests and check the results.
#
if(BUILD_TESTS)
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/test")

    file(COPY "test/q/k4unit.q" DESTINATION "${CMAKE_SOURCE_DIR}/bin/test")
    file(COPY "test/q/runtests.q" DESTINATION "${CMAKE_SOURCE_DIR}/bin/test")
    file(COPY "test/q/tests.csv" DESTINATION "${CMAKE_SOURCE_DIR}/bin/test")

    add_executable(runtests "test/cpp/main.cxx")
    target_link_libraries(runtests gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
endif()

INSTALL(TARGETS fixengine DESTINATION "${CMAKE_SOURCE_DIR}/bin/lib")
INSTALL(TARGETS runtests DESTINATION "${CMAKE_SOURCE_DIR}/bin/test")

add_custom_command(TARGET fixengine POST_BUILD
                    COMMAND ln -s ${CMAKE_SOURCE_DIR}/bin/lib/libquickfix.so.16 ${CMAKE_SOURCE_DIR}/bin/lib/libquickfix.so)
