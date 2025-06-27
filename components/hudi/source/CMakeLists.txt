cmake_minimum_required(VERSION 3.0.0)
project(hudi)
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
set(LIB_TYPE STATIC)
else()
set(LIB_TYPE SHARED)
endif()

include_directories(
                    ./include
                    ./common
                    ./flash
                    ./video
                    ./audio
                    ./cec
                    ./dsc			
                    ./bluetooth
                    ./hdmi
                    ./cvbsrx
                    ./hdmirx
                   )

file(GLOB COM_SRC
     common/hudi_log.c
     common/hudi_list.c
     )

if (FLASH_API)
	file(GLOB FLASH_SRC
		flash/hudi_flash.c
		flash/hudi_flash_otp.c
		flash/hudi_flash_protect.c
		)
endif()
    
if (VIDEO_API)
	file(GLOB VIDEO_SRC
		video/hudi_vdec.c
		video/hudi_vidsink.c
		)
endif()

if (AUDIO_API)
	file(GLOB AUDIO_SRC
		audio/hudi_adec.c
		audio/hudi_snd.c
		audio/hudi_snd_i2si.c		
		)
endif()

if (DISPLAY_API)
	file(GLOB DISPLAY_SRC
		display/hudi_display.c
		)
endif()

if (AVSYNC_API)
	file(GLOB AVSYNC_SRC
		avsync/hudi_avsync.c
		)
endif()

if (CEC_API)
	file(GLOB CEC_SRC
		cec/*.c
		cec/adapter/*.c
		cec/utils/*.c
		)
endif()

if (DSC_API)
	file(GLOB DSC_SRC
		dsc/hudi_dsc.c
		)
endif()

if (BLUETOOTH_API)
	file(GLOB BLUETOOTH_SRC
		bluetooth/*.c
		)
endif()

if (PERSISTENTMEM_API)
	file(GLOB PERSISTENTMEM_SRC
		persistentmem/hudi_persistentmem.c
		)
endif()

if (HDMI_API)
	file(GLOB HDMI_SRC
		hdmi/hudi_hdmi.c
		)
endif()

if (POWER_API)
	file(GLOB POWER_SRC
		power/*.c
		)
endif()

if (CVBSRX_API)
	file(GLOB CVBSRX_SRC
		cvbsrx/*.c
		)
endif()

if (SCREEN_API)
	file(GLOB SCREEN_SRC
		screen/*.c
		)
endif()

if (HDMIRX_API)
    file(GLOB HDMIRX_SRC
		hdmirx/*.c
		)
endif()

if(SOC_HC15XX)
	add_definitions(-DSOC_HC15XX)
endif()

add_library(hudi ${LIB_TYPE} ${COM_SRC} ${FLASH_SRC} ${VIDEO_SRC} ${AUDIO_SRC} ${DISPLAY_SRC} ${AVSYNC_SRC} ${CEC_SRC} ${DSC_SRC} ${BLUETOOTH_SRC} ${PERSISTENTMEM_SRC} ${HDMI_SRC} ${POWER_SRC} ${CVBSRX_SRC} ${SCREEN_SRC} ${HDMIRX_SRC})
install(TARGETS hudi LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
