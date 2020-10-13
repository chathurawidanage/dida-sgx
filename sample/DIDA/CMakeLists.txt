set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include(ConfigureDida)
include_directories(SYSTEM "${DIDA_INCLUDE_DIR}")

set(EDL_SEARCH_PATHS Enclave Enclave/TrustedLibrary)

message("SGX HW : ${SGX_HW}")


set(E_SRCS Enclave/Enclave.cpp)
set(T_SRCS Enclave/TrustedLibrary/LibcxxDsp.cpp Enclave/TrustedLibrary/LibcxxMrg.cpp)
if (SGX_HW AND SGX_MODE STREQUAL "Release")
    set(LDS Enclave/Enclave.lds)
else ()
    set(LDS Enclave/Enclave_debug.lds)
endif ()
add_trusted_library(trusted_lib SRCS ${T_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
add_enclave_library(enclave SRCS ${E_SRCS} TRUSTED_LIBS trusted_lib
        EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT ${LDS} ${DIDA_INCLUDE_DIR})
enclave_sign(enclave KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp
        App/TrustedLibrary/LibcxxDsp.cpp
        App/TrustedLibrary/LibcxxMrg.cpp)
add_untrusted_executable(didasgx SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
add_dependencies(didasgx enclave-sign)
target_link_libraries(didasgx ${DIDA_LIBRARIES} gomp dl)
