#!/bin/bash

mkdir -p "${TRAVIS_BUILD_DIR}"
cd "${TRAVIS_BUILD_DIR}"
mkdir build
cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_COVER=ON
make
#cp -R ../test test
#mkdir testdb
#./lemongrab --test
#lcov -c -d . -o coverage.info
#lcov --remove coverage.info "/usr*" -o coverage.clean.info
#coveralls-lcov --repo-token "${COVERALLS_REPO_TOKEN}" coverage.clean.info
