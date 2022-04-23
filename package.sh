#!/usr/bin/env bash

# This is an example script for building the library on a Unix based system.
# It creates a temporary directory in /tmp and performs an out-of-source build.

# Find the location of this script and create a temporary directory path for this build.
DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
TMP="/tmp/fix-engine-build.$$/"

# Create and move into our out-of-source build directory.
echo "[building libkdbfix in ${TMP}]"
mkdir -p "${TMP}" || (echo "error: failed to create directory ${TMP} for the build" && exit 1)
pushd  "${TMP}" || (echo "error: failed to move into ${TMP}" && exit 1)

# Read the CMakeLists.txt from the source directory $DIR and generate
# make files into our $TMP directory
cmake "${DIR}" -DCMAKE_BUILD_TYPE=Release

# Perform the build and create a tar.gz file with the build contents.
make build_package

# Clean up the temporary build directory when we have finished.
echo "[cleaning up the build from ${TMP}]"
rm -rf "${TMP}"