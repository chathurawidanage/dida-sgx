cmake_minimum_required(VERSION 2.8)

include(ExternalProject)
project(DIDA)
ExternalProject_Add(dida
        GIT_REPOSITORY "https://github.com/chathurawidanage/dida"
        GIT_TAG origin/master
        SOURCE_DIR "${DIDA_ROOT}/dida"
        BINARY_DIR "${DIDA_ROOT}/build"
        INSTALL_DIR "${DIDA_ROOT}/install"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DIDA_ROOT}/install -DWITH_GFLAGS=OFF)