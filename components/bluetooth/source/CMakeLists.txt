cmake_minimum_required(VERSION 3.0.0)
project(bluetooth)
set(CMAKE_BUILD_TYPE "release")

message(STATUS "Install Prefix (CMAKE_INSTALL_PREFIX):     ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Binary Directory (BIN_INSTALL_DIR):        ${BIN_INSTALL_DIR}")
message(STATUS "Library Directory (LIB_INSTALL_DIR):       ${LIB_INSTALL_DIR}")
message(STATUS "Include Directory (INCLUDE_INSTALL_DIR):   ${INCLUDE_INSTALL_DIR}")
message(STATUS "Documentation Directory (DOC_INSTALL_DIR): ${DOC_INSTALL_DIR}")
message(STATUS "Man Pages Directory (MAN_INSTALL_DIR):     ${MAN_INSTALL_DIR}")
message(STATUS "Build Type (CMAKE_BUILD_TYPE):             ${CMAKE_BUILD_TYPE}")
message(STATUS "Build static libraries:                    ${BUILD_STATIC_LIBS}")
message(STATUS "Build shared library:                      ${BUILD_SHARED_LIBS}")
message(STATUS "Project source dir::                       ${PROJECT_SOURCE_DIR}")

if(HCRTOS)
add_definitions(-DHC_RTOS)
set(LIB_TYPE STATIC)
else()
add_definitions(-DHC_LINUX)
set(LIB_TYPE SHARED)
endif()

if(BLUETOOTH_AD6956F)
add_definitions(-DBLUETOOTH_AD6956F)	
endif()

if(BLUETOOTH_AC6955F_RT)
add_definitions(-DBLUETOOTH_AC6955F_RT)	
endif()

if(BLUETOOTH_AC6955F_GX)
add_definitions(-DBLUETOOTH_AC6955F_GX)	
endif()

if(BLUETOOTH_AC6956C_GX)
add_definitions(-DBLUETOOTH_AC6956C_GX)	
endif()

include_directories(
                    ./inc
                   )

file(GLOB BLUETOOTH
     bluetooth_weak.c
     ac6955f/ac6955frt.c
     ac6955f/ac6955fgx.c
     ac6955f/ac6956cgx.c
     ad6956f/ad6956f.c
     )

add_library(bluetooth ${LIB_TYPE} ${BLUETOOTH})
install(TARGETS bluetooth LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
