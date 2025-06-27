cmake_minimum_required(VERSION 3.0.0)
project (hctinyplayer)

if(HCRTOS)
add_definitions(-DHC_RTOS)
set(LIB_TYPE STATIC)
else()
add_definitions(-DHC_LINUX)
set(LIB_TYPE SHARED)
endif()

file (GLOB SRC_FILE
	hctinyplayer_registry.c
)

if(BR2_PACKAGE_HCTINYPLAYER_TEST)
file (GLOB TEST_SRC_FILE
	hctinyplayer_test.c
)
endif()

################################# add protocols #################################
set (MY_PROTOCOLS "static const URLProtocol * const url_protocols[] = {\n")
if(BR2_PACKAGE_HCTINYPLAYER_PROTOCOL_FILE)
	set (MY_PROTOCOLS "${MY_PROTOCOLS}\t&ff_file_protocol,\n")
endif()
set (MY_PROTOCOLS "${MY_PROTOCOLS}\tNULL\n};")
file(WRITE "protocol_list.c" "${MY_PROTOCOLS}")

################################# add demuxers  #################################
set (MY_DEMUXERS "static const AVInputFormat * const demuxer_list[] = {\n")
if(BR2_PACKAGE_HCTINYPLAYER_DEMUXER_FLV)
	set (MY_DEMUXERS "${MY_DEMUXERS}\t&ff_flv_demuxer,\n")
	set (MY_DEMUXERS "${MY_DEMUXERS}\t&ff_live_flv_demuxer,\n")
	set (MY_DEMUXERS "${MY_DEMUXERS}\t&ff_kux_demuxer,\n")
endif()
if(BR2_PACKAGE_HCTINYPLAYER_DEMUXER_MJPEG)
	set (MY_DEMUXERS "${MY_DEMUXERS}\t&ff_mjpeg_demuxer,\n")
endif()
set (MY_DEMUXERS "${MY_DEMUXERS}\tNULL\n};")
file(WRITE "demuxer_list.c" "${MY_DEMUXERS}")

#################################  add parsers  ##################################
set (MY_PARSERS "static const AVCodecParser * const parser_list[] = {\n")
if(BR2_PACKAGE_HCTINYPLAYER_PARSER_MPEGAUDIO)
	set (MY_PARSERS "${MY_PARSERS}\t&ff_mpegaudio_parser,\n")
endif()
if(BR2_PACKAGE_HCTINYPLAYER_PARSER_H264)
	set (MY_PARSERS "${MY_PARSERS}\t&ff_h264_parser,\n")
endif()
set (MY_PARSERS "${MY_PARSERS}\tNULL\n};")
file(WRITE "parser_list.c" "${MY_PARSERS}")

##################################################################################

include_directories(${CMAKE_SOURCE_DIR} $(STAGING_DIR)/usr/include)
add_library(hctinyplayer_registry ${LIB_TYPE} ${SRC_FILE} ${TEST_SRC_FILE})
install(TARGETS hctinyplayer_registry LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
