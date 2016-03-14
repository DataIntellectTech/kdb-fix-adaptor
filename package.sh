#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TMP="/tmp/fix-engine-build.$$/"

echo "[building fix-engine in ${TMP}]"
mkdir -p ${TMP}
cd ${TMP}

cmake ${DIR}
make
make build_package

# clean up the temporary build directory when we have finished
echo "[cleaning up the build from ${TMP}]"
rm -rf ${TMP}
