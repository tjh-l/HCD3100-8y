# the compiler: gcc for C program, define as g++ for C++
#CC = g++

#ARM32
#CC=arm-linux-androideabi-g++
#ARM64
#CC=aarch64-linux-android-g++
#x86
#CC=i686-linux-android-g++
#x86_64
#CC=x86_64-linux-android-g++
#mips
CC=mipsel-linux-android-g++
#mips64
#CC=mips64el-linux-android-g++


INCLUDE = .
RM = rm -f

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#CFLAGS  = -g -Wall -static #-fno-exceptions

cflags-y += -g -Wall -static -Wno-unused-function -Wno-format-nonliteral -Wno-unused-parameter -Wno-maybe-uninitialized

EXTRA_CFLAGS += -Drtw_mpRateParseFunc=rtwpriv_rtw_mpRateParseFunc \
								-DHwRateToMPTRate=rtwpriv_HwRateToMPTRate \
								-DHwRateToMPTRate=rtwpriv_HwRateToMPTRate \
								-Dkey_2char2num=rtwpriv_key_2char2num \
								-Dmacstr2num=rtwpriv_macstr2num \
								-Dwifirate2_ratetbl_inx=rtwpriv_wifirate2_ratetbl_inx \
								-Diw_sockets_open=rtwpriv_iw_sockets_open
		
lib_rtwpriv-y = rtwpriv.o rtw_api.o rtw_udpsrv.o
obj-y += lib_rtwpriv.o
