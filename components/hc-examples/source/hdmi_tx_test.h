#ifndef __HDMITX_TEST__
#define __HDMITX_TEST__

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <hcuapi/dis.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

int hdmi_get_edid_all_video_res(int argc , char *argv[]);
int hdmi_send_cec_cmd(int argc , char *argv[]);
int hdmi_set_cec_onoff(int argc , char *argv[]);
int hdmi_get_cec_onoff(int argc , char *argv[]);
int hdmi_send_logical_addr(int argc , char *argv[]);
#endif