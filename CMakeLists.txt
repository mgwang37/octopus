cmake_minimum_required (VERSION 2.8)

project (octopus-socks5)

#set(CMAKE_CXX_FLAGS "-g -Wall -D DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wall -D DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-g -Wall")

add_subdirectory(src bin)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

aux_source_directory (./src DIR_SRCS)

install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/octopus-socks5 DESTINATION bin)

set(CPACK_PACKAGE_NAME "octopus-socks5")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "socks5 proxy")
set(CPACK_PACKAGE_VENDOR "NO")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RPM_PACKAGE_GROUP "octopus")
set(CPACK_RPM_PACKAGE_URL "https://github.com/mgwang37/octopus")
set(CPACK_RPM_PACKAGE_DESCRIPTION "a socks5 proxy")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_LICENSE "GPL")

set(CPACK_GENERATOR "RPM")

include(CPack)
