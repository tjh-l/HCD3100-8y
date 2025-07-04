#
# Makefile
#
CC ?= gcc
CUR_APP_DIR ?= ${shell pwd}

SYS_DIR = -I$(CUR_APP_DIR)/hctinyplayer_app \
		-I$(CUR_APP_DIR)/hctinyplayer_app/include \
		-I$(STAGING_DIR)/usr/include \
		-I$(STAGING_DIR)/usr/include/hcuapi

LVGL_HEADERS = -I$(STAGING_DIR)/usr/include/lvgl/ \
		-I$(STAGING_DIR)/usr/include/lvgl/lvgl


MY_DEF =

FFMPEG_SWSCALE_SUPPORT := $(shell echo $(HCTINYPLAYERAPP_DEF) | grep -o FFMPEG_SWSCALE_SUPPORT)


CFLAGS ?= -O3 $(SYS_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
CFLAGS += $(LVGL_HEADERS) -include $(AUTO_CONFIG)
LDFLAGS ?= -lm -llvgl -Wl,-Map,$@.map

DEMO_LIBS = -lffplayer -lglist -lpthread -lge -Wl,--start-group -lstdc++ -Wl,--end-group
DEMO_LIBS += -lhcfota -lhudi

ifeq ($(FFMPEG_SWSCALE_SUPPORT),FFMPEG_SWSCALE_SUPPORT)
	#MY_DEF += -DFFMPEG_SWSCALE_SUPPORT
	DEMO_LIBS += -lswscale
endif

include $(CUR_APP_DIR)/hctinyplayer_app/hctinyplayer.mk
DEMO_NAME = hctinyplayer

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
HCTINYPLAYER_OBJS = $(HCTINYPLAYER_SRC:.c=$(OBJEXT))
HCTINYPLAYER_OBJS += $(HCTINYPLAYER_CPP_SRC:.cpp=$(OBJEXT))


## MAINOBJ -> OBJFILES

all_depends := $(patsubst %.c,%.d,$(CSRCS) $(HCTINYPLAYER_SRC))
all_depends +=$(patsubst %.cpp,%.d,$(HCTINYPLAYER_CPP_SRC))
all_depends += $(patsubst %.S,%.d,$(ASRCS))

all: $(DEMO_NAME)

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

$(DEMO_NAME):	$(COBJS) $(HCTINYPLAYER_OBJS)
	@$(CC) -o $@ $(HCTINYPLAYER_OBJS) $(COBJS) $(LDFLAGS) $(DEMO_LIBS)
	
clean: 
	rm -f $(AOBJS) $(COBJS) $(HCTINYPLAYER_OBJS)
	@find ./ -type f -name "*.o" |xargs rm -rf
	rm -rf $(DEMO_NAME)

install:
	@echo "install ......"
