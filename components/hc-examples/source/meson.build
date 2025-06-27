project('hichip-ffmpeg-player', 'c', version : '1.0', license : 'LGPL')

cc = meson.get_compiler('c')

ffplayer_version = meson.project_version()

api_version = '1.0'

cflags = [ '-D_GNU_SOURCE', '-Werror' , '-fsigned-char' , '-W' , '-Wno-unused-result' ]

avformat_dep =  dependency('libavformat', version : '>=58.76.100', required : true, fallback : ['avformat', 'avformat_dep'])
avcodec_dep =  dependency('libavcodec', version : '>=58.134.100', required : true, fallback : ['avcodec', 'avcodec_dep'])
avutil_dep =  dependency('libavutil', version : '>=56.70.100', required : true, fallback : ['avutil', 'avutil_dep'])
swscale_dep =  dependency('libswscale', version : '>=5.9.100', required : true, fallback : ['swscale', 'swscale_dep'])

auto_config_path = get_option('AUTO_CONFIG_PATH')
add_project_arguments('-I' + auto_config_path, language : 'c')

ldflags = [ '-lpthread', '-lffplayer', '-lge', '-lm', '-lglist' ]
player_examples = [
	'console.c',
	'ffplayer_examples.c',
	'es_decoder.c',
	'h264_es_player.c',
	'dis_test.c',
	'ge_draw_subtitle.c',
	'vin_dvp_test.c',
]

if get_option('video_pbp_test') == true

cflags = [ '-D_GNU_SOURCE', '-fsigned-char' , '-W' , '-Wno-unused-result' ]

player_examples += [
	'video_pbp/pbp_slot_mgr.c',
	'video_pbp/video_pbp_test.c',
	]

if get_option('cast_wifi_test') == true
	ldflags += [
			'-ldlna', 
			'-lmiracast', 
			'-lmdns', 
			'-lcjson', 
			'-lhccast-com',
			'-lwpa_client', 
			'-lhccast-net',
			'-lhccast-wl',
			'-lhudi',  
			'-lcrypto',
			]
	player_examples += [
		'hccast_test/wifi_cast_test.c',
		'hccast_test/hccast_test_com.c',
	]
endif

if get_option('cast_usb_test') == true

	ldflags += [
			'-lhccast-um',
			'-lssl',
			'-lcrypto',
			'-lusb-1.0',
			]
	player_examples += [
		'hccast_test/usb_cast_test.c',
	]

endif

if get_option('hdmirx_test') == true
	player_examples += [
		'hdmi_rx/hdmi_rx_test.c',
	]
endif	

if get_option('tvdec_test') == true
	player_examples += [
		'tv_decoder/tv_dec_test.c',
	]
endif	

executable('multi_dis', player_examples, c_args: cflags, link_args: ldflags, 
	dependencies : [avformat_dep, avcodec_dep, avutil_dep, swscale_dep], install : true)

else

executable('mplayer', player_examples, c_args: cflags, link_args: ldflags, 
	dependencies : [avformat_dep, avcodec_dep, avutil_dep, swscale_dep], install : true)

endif

if get_option('video_pbp_test') == false

ldflags = [ '-lpthread' ]

if get_option('hdmirx_test') == true
cflags = [ '-D_GNU_SOURCE', '-fsigned-char' , '-W' , '-Wno-unused-result' ]
hdmi_rx_test = [
	'console.c',
	'wav.c',
	'hdmi_rx/hdmi_rx_test.c',
]
executable('hdmi_rx', hdmi_rx_test, c_args: cflags, link_args: ldflags, install : true)
endif	

if get_option('tvdec_test') == true
cflags = [ '-D_GNU_SOURCE', '-fsigned-char' , '-W' , '-Wno-unused-result' ]
tv_decoder_test = [
	'console.c',
	'tv_decoder/tv_dec_test.c',
]
executable('tv_dec', tv_decoder_test, c_args: cflags, link_args: ldflags, install : true)
endif	

endif


ldflags = [ '-lpthread', '-lffplayer', '-lspectrum']
i2s_record_test = [
	'console.c',
	'wav.c',
	'i2s_record_test.c',
]
executable('i2s_recorder', i2s_record_test, c_args: cflags, link_args: ldflags, install : true)

if get_option('hcge_test') == true
	ldflags = [ '-lpthread', '-lge', '-lm', '-Ige/customer_api']
	hcge_fill_rect_test = [
		'ge/hcge_fill_rect_test.c',
	]
	executable('hcge_fill_rect_test', hcge_fill_rect_test, c_args: cflags, link_args: ldflags, install : true)

	hcge_rotate_test = [
		'ge/hcge_rotate_test.c',
		'ge/customer_api/mi_gfx.c',
	]
	executable('hcge_rotate_test', hcge_rotate_test, c_args: cflags, link_args: ldflags, install : true)

	hcge_cust_api_test = [
		'ge/hcge_cust_api_test.c',
		'ge/customer_api/mi_gfx.c',
	]
	executable('hcge_cust_api_test', hcge_cust_api_test, c_args: cflags, link_args: ldflags, install : true)

	hcge_display_rgb = [
		'ge/hcge_display_rgb.c',
	]
	executable('hcge_display_rgb', hcge_display_rgb, c_args: cflags, link_args: ldflags, install : true)
endif


cflags_hdmi_switch = [ '-D_GNU_SOURCE', '-Wall' ]
ldflags_hdmi_switch = [ '-lpthread', '-lffplayer' ]
hdmi_switch = [
	'console.c',
	'hdmi_switch/hdmi_switch.c',
]
executable('hdmi_switch', hdmi_switch, c_args: cflags_hdmi_switch, link_args: ldflags_hdmi_switch, install : true)


cflags_pq_tool = [ '-D_GNU_SOURCE', '-Wall' ]
pq_tool = [
	'pq_tool/main.c',
	'pq_tool/hc_pq_reg.c'
]
executable('pq_tool', pq_tool, c_args: cflags_pq_tool, install : true)

cflags_usb_otg_daemon= [ '-D_GNU_SOURCE', '-Wall' ]
usb_otg_daemon = [
	'usb_otg_daemon/main.c'
]
executable('usb_otg_daemon', usb_otg_daemon, c_args: cflags_usb_otg_daemon, install : true)

cflags_usb_cmds_main= [ '-D_GNU_SOURCE', '-Wall' ]
usb_cmds_main = [
	'usb_cmds_main.c',
	'console.c',
]
executable('usb_cmds_main', usb_cmds_main, c_args: cflags_usb_cmds_main, install : true)

toe_ldflags = [ '-lpthread' ]
cflags_toe_sample= [ '-D_GNU_SOURCE', '-Wall' ]
toe_sample = [
	'toe_sample.c',
	'console.c',
]
executable('toe_sample', toe_sample, c_args: cflags_toe_sample, link_args: toe_ldflags, install : true)

cflags_hcfb_test= [ '-D_GNU_SOURCE', '-Wall' ]
hcfb_test_daemon = [
	'fb_test.c',
	'console.c',
]
executable('hcfb_test_daemon', hcfb_test_daemon, c_args: cflags_hcfb_test, install : true)

cflags_hcfb_dither_test= [ '-D_GNU_SOURCE', '-Wall' ]
hcfb_dither_daemon = [
	'fb_dither_test.c',
	'console.c',
]
executable('hcfb_dither_daemon', hcfb_dither_daemon, c_args: cflags_hcfb_dither_test, install : true)


cflags_v4l2grab = [ '-D_GNU_SOURCE', '-Wall' ]
v4l2grab = [
	'v4l2grab/v4l2grab.c',
]
#executable('v4l2grab', v4l2grab, c_args: cflags_v4l2grab, install : false)

if get_option('display_sink') == true
	ds_cflags = ['-Werror'] 
	ds_ldflags = ['-lpthread']
	ds_sources = [
		'hdmi_wireless_display/display_sink.c',
		'hdmi_wireless_display/decoder.c',
		]
	executable('display_sink', ds_sources, c_args: ds_cflags, link_args: ds_ldflags, install: true)
endif

if get_option('display_source') == true
	ds_cflags = ['-Werror'] 
	ds_ldflags = ['-lpthread']
	ds_sources = [
		'hdmi_wireless_display/display_source.c',
		'hdmi_wireless_display/queue.c',
		]
	executable('display_source', ds_sources, c_args: ds_cflags, link_args: ds_ldflags, install: true)
endif
if get_option('dsc') == true
	ds_cflags = ['-Werror'] 
	ds_ldflags = []
	dsc_test_sources = [
		'dsc/dsc_test.c',
		]
	executable('dsc_test', dsc_test_sources, c_args: ds_cflags, link_args: ds_ldflags, install: true)
endif

if get_option('wifi_test') == true
	wifi_test_cflags = [ '-D_GNU_SOURCE', '-Wall' ]
	wifi_test_ldflags = ['-lpthread', '-lhccast-net', '-lwpa_client', '-lhccast-com']
	wifi_test_sources = [
		'console.c',
		'wifi_test/wifi_test.c',
		]
	executable('wifi_test', wifi_test_sources, c_args: wifi_test_cflags, link_args: wifi_test_ldflags, install: true)
endif

if get_option('gpio_map_test') == true
	gpio_map_cflags = [ '-D_GNU_SOURCE', '-Wall' ]
	gpio_map_ldflags = []
	gpio_map_sources = [
		'gpio-map/gpio.c',
		'gpio-map/pinmux.c',
	]
	executable('gpio_map', gpio_map_sources, c_args: gpio_map_cflags, link_args: gpio_map_ldflags, install: true)
endif

cflags_bootlogo_replace = [ '-D_GNU_SOURCE', '-Wall' ]
ldflags_bootlogo_replace = [ '-lpthread' ]
bootlogo_replace = [
	'bootlogo_replace/bootlogo_replace.c',
]
executable('bootlogo_replace', bootlogo_replace, c_args: cflags_bootlogo_replace, link_args: ldflags_bootlogo_replace, install : true)

if get_option('hudi_cec_test') == true
	ldflags = [ '-lpthread', '-lhudi' ]
	hudi_cec_test = [
		'console.c',
		'hudi/hudi_cec_sample.c',
	]
	executable('hudi_cec', hudi_cec_test, c_args: cflags, link_args: ldflags, install : true)
endif

if get_option('bluetooth_test') == true
	ldflags = [ '-lpthread', '-lbluetooth' ]
	bluetooth_test = [
		'console.c',
		'bluetooth/bluetooth_test.c',
	]
	executable('bluetooth', bluetooth_test, c_args: cflags, link_args: ldflags, install : true)
endif

if get_option('screenshot_test') == true
screenshot_cflags = [ '-D_GNU_SOURCE', '-Wall' ]
	screenshot_test = [
		'console.c',
		'screenshot_test/screenshot.c',
	]
	executable('screenshot', screenshot_test, c_args: screenshot_cflags, install : true)
endif
