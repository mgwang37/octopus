cmake_minimum_required (VERSION 2.8)

project (eversec-socks5)

set(CMAKE_CXX_FLAGS "-g -Wall")

add_subdirectory(src bin)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wno-unused-variable -pthread")

aux_source_directory (./src DIR_SRCS)

install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/eversec-socks5 DESTINATION local/bin)
#install(FILES bin/hello.a DESTINATION lib)
#install(FILES src/hello.h DESTINATION include)
#SET(CMAKE_INSTALL_PREFIX /usr/local)

set(CPACK_PACKAGE_NAME "eversec-socks5")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "socks5 proxy")
set(CPACK_PACKAGE_VENDOR "NO")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RPM_PACKAGE_GROUP "eversoc")
set(CPACK_RPM_PACKAGE_URL "http://eversec.com.cn/")
set(CPACK_RPM_PACKAGE_DESCRIPTION "eversoc")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_LICENSE "Licence")

set(CPACK_GENERATOR "RPM")

include(CPack)
