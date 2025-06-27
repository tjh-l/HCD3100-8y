#
# Makefile
#
CC ?= gcc
CUR_APP_DIR ?= ${shell pwd}

SYS_DIR = -I$(CUR_APP_DIR)/hcprojector_app \
		-I$(CUR_APP_DIR)/hcprojector_app/include \
		-I$(STAGING_DIR)/usr/include \
		-I$(STAGING_DIR)/usr/include/hcuapi

LVGL_HEADERS = -I$(STAGING_DIR)/usr/include/lvgl/ \
		-I$(STAGING_DIR)/usr/include/lvgl/lvgl


MY_DEF =

DLNA_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o DLNA_SUPPORT)
DIAL_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o DIAL_SUPPORT)
MIRACAST_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o MIRACAST_SUPPORT)
AIRCAST_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o AIRCAST_SUPPORT)
USBMIRROR_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o USBMIRROR_SUPPORT)
BLUETOOTH_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o BLUETOOTH_SUPPORT)
FFMPEG_SWSCALE_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o FFMPEG_SWSCALE_SUPPORT)
AUDIO_SPECTRUM_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o AUDIO_SPECTRUM_SUPPORT)
APPS_LIB_CURL_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o APPS_LIB_CURL_SUPPORT)
LIB_OPENSSL_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o LIB_OPENSSL_SUPPORT)
APPS_RESOLUTION_240P_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o APPS_RESOLUTION_240P_SUPPORT)
HCIPTV_YTB_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o HCIPTV_YTB_SUPPORT)
BATTERY_MONITOR_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o BATTERY_MONITOR_SUPPORT)
AIRP2P_SUPPORT := $(shell echo $(HCPROJECTORAPP_DEF) | grep -o AIRP2P_SUPPORT)

CFLAGS ?= -O3 $(SYS_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
CFLAGS += $(LVGL_HEADERS) -include $(AUTO_CONFIG)
LDFLAGS ?= -lm -llvgl -Wl,-Map,$@.map

DEMO_LIBS = -lffplayer -lglist -lpthread -lge -Wl,--start-group -lstdc++ -Wl,--end-group
DEMO_LIBS += -lhcfota -lhudi

USB_CAST_LIB = 
WIFI_CAST_LIB = 

ifeq ($(DLNA_SUPPORT),DLNA_SUPPORT)
	#MY_DEF += -DDLNA_SUPPORT
	DEMO_LIBS += -ldlna
	WIFI_CAST_LIB = 1
endif

ifeq ($(MIRACAST_SUPPORT),MIRACAST_SUPPORT)
	#MY_DEF += -DMIRACAST_SUPPORT
	DEMO_LIBS += -lmiracast
	WIFI_CAST_LIB = 1
endif

ifeq ($(AIRCAST_SUPPORT),AIRCAST_SUPPORT)
	#MY_DEF += -DAIRCAST_SUPPORT
	#DEMO_LIBS += -laircast
	WIFI_CAST_LIB = 1
endif

ifeq ($(USBMIRROR_SUPPORT),USBMIRROR_SUPPORT)
	#MY_DEF += -DUSBMIRROR_SUPPORT
	USB_CAST_LIB = 1
	DEMO_LIBS += -lhccast-um -lusb-1.0 -lssl
endif

ifeq ($(AIRP2P_SUPPORT),AIRP2P_SUPPORT)
	CFLAGS += -I$(STAGING_DIR)/usr/include/libusb-1.0
	DEMO_LIBS += -lnl-3 -lnl-genl-3 -lnl-route-3
endif

ifeq ($(WIFI_CAST_LIB),1)
	DEMO_LIBS += -lhccast-com
else	
	ifeq ($(USB_CAST_LIB),1)
		DEMO_LIBS += -lhccast-com
	endif
endif	


ifeq ($(WIFI_CAST_LIB),1)
	DEMO_LIBS += -lmdns -lcjson -lwpa_client -lhccast-net -lhccast-wl
endif	

ifeq ($(FFMPEG_SWSCALE_SUPPORT),FFMPEG_SWSCALE_SUPPORT)
	#MY_DEF += -DFFMPEG_SWSCALE_SUPPORT
	DEMO_LIBS += -lswscale
endif

ifeq ($(AUDIO_SPECTRUM_SUPPORT),AUDIO_SPECTRUM_SUPPORT)
	DEMO_LIBS += -lspectrum
endif

ifeq ($(APPS_LIB_CURL_SUPPORT),APPS_LIB_CURL_SUPPORT)
	DEMO_LIBS += -lcurl
endif

ifeq ($(LIB_OPENSSL_SUPPORT),LIB_OPENSSL_SUPPORT)
	DEMO_LIBS += -lcrypto
endif

ifeq ($(BLUETOOTH_SUPPORT),BLUETOOTH_SUPPORT)
	DEMO_LIBS += -lbluetooth
endif

ifeq ($(DIAL_SUPPORT),DIAL_SUPPORT)
	DEMO_LIBS += -lpcre -lcurl -lhccast-iptv
	WIFI_CAST_LIB = 1
endif

ifeq ($(HCIPTV_YTB_SUPPORT),HCIPTV_YTB_SUPPORT)
	DEMO_LIBS += -lpcre -lcurl -lhccast-iptv
endif

include $(CUR_APP_DIR)/hcprojector_app/projector.mk

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
HCPROJECTOR_OBJS = $(HCPROJECTOR_SRC:.c=$(OBJEXT))
HCPROJECTOR_OBJS += $(HCPROJECTOR_CPP_SRC:.cpp=$(OBJEXT))

## MAINOBJ -> OBJFILES


all_depends := $(patsubst %.c,%.d,$(CSRCS) $(HCPROJECTOR_SRC))
all_depends +=$(patsubst %.cpp,%.d,$(HCPROJECTOR_CPP_SRC))
all_depends += $(patsubst %.S,%.d,$(ASRCS))

all: $(APP_BIN_PROJECTOR)

%.o: %.c
	@$(CC) $(CFLAGS) $(MY_DEF) -c $< -o $@
	@echo "Compiling $@"
	
.PHONY: FORCE

%.o: %.cpp
	@$(CC) $(CFLAGS) $(MY_DEF) -c $< -o $@
	@echo "Compiling $@"

%.d: %.c
	@set -e;rm -rf $@;$(CC) $(CFLAGS) $(MY_DEF) -MM $^ > $@.$$$$;cp $@.$$$$ $@;rm -rf $@.$$$$

%.d: %.cpp
	@set -e;rm -rf $@;$(CC) $(CFLAGS) $(MY_DEF) -MM $^ > $@.$$$$;cp $@.$$$$ $@;rm -rf $@.$$$$

-include $(all_depends)

$(APP_BIN_PROJECTOR):	$(COBJS) $(HCPROJECTOR_OBJS)
	@$(CC) -o $@ $(HCPROJECTOR_OBJS) $(COBJS) $(LDFLAGS) $(DEMO_LIBS)
	
clean: 
	rm -f $(AOBJS) $(COBJS) $(HCPROJECTOR_OBJS)
	@find ./ -type f -name "*.o" |xargs rm -rf
	rm -rf $(APP_BIN_PROJECTOR)

install:
	@echo "install ......"
