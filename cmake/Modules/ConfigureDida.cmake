# build bwa from github tag
set(DIDA_ROOT ${CMAKE_BINARY_DIR}/dida)
set(DIDA_INSTALL ${CMAKE_BINARY_DIR}/dida/install)

#if (UNIX)
#    set(DIDA_EXTRA_COMPILER_FLAGS "-fPIC")
#endif()

set(DIDA_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${DIDA_EXTRA_COMPILER_FLAGS})
set(DIDA_C_FLAGS ${CMAKE_C_FLAGS} ${DIDA_EXTRA_COMPILER_FLAGS})

SET(WITH_GFLAGS OFF)

configure_file("${CMAKE_SOURCE_DIR}/cmake/Templates/DIDA.CMakeLists.txt.cmake"
        "${DIDA_ROOT}/CMakeLists.txt")

execute_process(
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE DIDA_CONFIG
        WORKING_DIRECTORY ${DIDA_ROOT})

execute_process(
        COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE DIDA_BUILD
        WORKING_DIRECTORY ${DIDA_ROOT})

#execute_process(
#        COMMAND "git submodule init && git submodule update"
#        RESULT_VARIABLE DIDA_BUILD
#        WORKING_DIRECTORY ${DIDA_ROOT}/bwa)

message("Building DIDA")

# Find MPI
find_package(MPI REQUIRED)

execute_process(
        COMMAND "./autogen.sh"
        WORKING_DIRECTORY ${DIDA_ROOT}/dida)
execute_process(
        COMMAND "./configure" "--with-mpi=/opt/openmpi-4.0.5/build"
        WORKING_DIRECTORY ${DIDA_ROOT}/dida)
execute_process(
        COMMAND "make"
        RESULT_VARIABLE DIDA_BUILD
        WORKING_DIRECTORY ${DIDA_ROOT}/dida)

file(MAKE_DIRECTORY "${DIDA_ROOT}/include")

file(GLOB PUBLIC_HEADERS
        "${DIDA_ROOT}/dida/*.h"  "${DIDA_ROOT}/dida/Common/*.h"
        )
file(COPY ${PUBLIC_HEADERS} DESTINATION ${DIDA_ROOT}/include)

file(GLOB DIDA_OBJECTS
        "${DIDA_ROOT}/dida/dsp-dsp.o" "${DIDA_ROOT}/dida/Common/dsp-Uncompress.o"
        "${DIDA_ROOT}/dida/Common/dsp-SignalHandler.o"
        "${DIDA_ROOT}/dida/Common/dsp-Fcontrol.o"
        )

set(DIDA_LIBRARIES "${DIDA_OBJECTS}")
set(DIDA_INCLUDE_DIR "${DIDA_ROOT}/include")

set(DIDA_FOUND TRUE)
#
#file(GLOB all_bwa_libs
#        "${DIDA_ROOT}/bwa/src/*.o"
#        "${DIDA_ROOT}/bwa/ext/safestringlib/obj/*.o"
#        )
#
#set(DIDA_LIBRARIES "${all_bwa_libs}")