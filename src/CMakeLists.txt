# configure_file(
#     "${PROJECT_SOURCE_DIR}/src/config.h.in"
#     "${PROJECT_SOURCE_DIR}/src/config.h"
# )
# aux_source_directory(. SRCS)


file(GLOB_RECURSE SRCS
    ./test.cc
    ./mpsi.cc
    ./utils.cc
)

add_executable(test ${SRCS})

find_package(coproto QUIET)
find_package(OpenSSL QUIET)

target_include_directories(test PUBLIC
                $<BUILD_INTERFACE:${MPPSI_THIRDPARTY_HINT}/include>) 

target_link_libraries(test visa::volePSI coproto::coproto)
