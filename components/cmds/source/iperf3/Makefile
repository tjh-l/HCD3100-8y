#
# -= Makefile for module compile =-
#
# Usage:
# . Name this file as "Makefile";
#   Put it in the same directory as module's source code.
# . Change the ROOT definition, if necessary;
#   Make it point to the root of the software tree.
# . Define the Module Name in MOD as you like;
#   There should be NO duplicated module names in the whole project.
# . List all files want to be compiled in ASMS and/or SRCS;
#   Including files, such as header files, must NOT be listed here.
INCLUDE = .
RM = rm -f

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#CFLAGS  = -g -Wall -static #-fno-exceptions

cflags-y += -g -Wall -static -I$(srctree)components/kernel/source/third_party/lwip-2.1.2/src/include/compat/posix/ -I$(STAGING_DIR)/usr/lib/ 
cflags-y += -Wno-sign-compare -Wno-unused-variable -Wno-format-nonliteral -Wno-format-security -Wno-incompatible-pointer-types -Wno-implicit-fallthrough 
cflags-y += -Wno-implicit-function-declaration -Wno-int-conversion -Wno-implicit-function-declaration -D__TR_SW__ -Wno-misleading-indentation -Wno-unused-but-set-variable
cflags-y += -Wno-unused-parameter -Wno-format -Wno-unused-function
		
lib_iperf-y = iperf.o dscp.o iperf_api.o iperf_client_api.o iperf_error.o iperf_locale.o iperf_sctp.o iperf_server_api.o iperf_tcp.o iperf_udp.o iperf_util.o net.o timer.o units.o iperf_time.o
obj-y += lib_iperf.o
