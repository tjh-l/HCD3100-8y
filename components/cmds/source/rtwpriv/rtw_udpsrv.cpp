/******************************************************************************
 *
 * Copyright(c) 2007 - 2021 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

#include "rtw_udpsrv.h"

char buf[BUFLEN], buf_tmp[BUFLEN], pre_result[BUFLEN];
char strread[]="wlan0     read_reg:\n";
char strreadrf[]="wlan0     read_rf:\n";
char strpsd[]="wlan0     mp_psd:\n";
char strpsd1[]="wlan1     mp_psd:\n";

char wlan0str[]="wlan0";
char wlan1str[]="wlan1";

void get_read_reg_value( FILE *fp,char *buf, int maxlen )
{
	int cget,value,start;
	char *p, *p2;
	const char a = '\n';
	p=strchr(buf, a);
	if (p == NULL) return;
	p2 = p;
	value=0;
	start=0;
	while ( (cget=fgetc(fp))!=EOF ) {
		//printf( "get=%c\n", cget );
		if ( (cget=='\n') || (cget==' ') ) {
			if (start) p2 += sprintf( (char*)p2, "%d ", value );
			//printf( "start=%d, value=%d, buf=(%s)\n", start, value, buf );
			value=0;
			start=0;
		} else if ( isxdigit(cget) ) {
			start=1;
			//printf( "value=%d,", value );
			if ( cget>='0' && cget<='9' )
				value=value*16+(cget-'0');
			else if ( cget>='a' && cget<='f' )
				value=value*16+(10+cget-'a');
			else if ( cget>='A' && cget<='F' )
				value=value*16+(10+cget-'A');
			//printf( "new value=%d\n", value );
		} else {
			//error
			sprintf(p, "\n");
			return;
		}
	}
	*p2=0;
}

#if 1//def IF_NAMING
char *str_replace(char *orig, char *rep, char *with)
{
	char *result; // the return string
	char *ins;    // the next insert point
	char *tmp;    // varies
	int len_rep;  // length of rep
	int len_with; // length of with
	int len_front; // distance between rep and end of last rep
	int count;    // number of replacements
	if ( !orig )
		return NULL;
	if ( !rep )
		rep = NULL;
	len_rep = strlen(rep);
	if ( !with )
		with = NULL;
	len_with = strlen(with);
	ins = orig;
	for (count = 0; (tmp = strstr(ins, rep)); ++count) {
		ins = tmp + len_rep;
	}
	// first time through the loop, all the variable are set correctly
	// from here on,
	//    tmp points to the end of the result string
	//    ins points to the next occurrence of rep in orig
	//    orig points to the remainder of orig after "end of rep"
	tmp = result = (char*)malloc(strlen(orig) + (len_with - len_rep) * count + 1);
	if (!result)
		return NULL;
	while (count--) {
		ins = strstr(orig, rep);
		len_front = ins - orig;
		tmp = strncpy(tmp, orig, len_front) + len_front;
		tmp = strcpy(tmp, with) + len_with;
		orig += len_front + len_rep; // move to next "end of rep"
	}
	strcpy(tmp, orig);
	return result;
}

void cmd_open_retstr(char *cmdbuf)
{
	#if 0
	char cmd[128]= {};
	FILE *fp;
	char path[BUFLEN];
	
	bzero(path, BUFLEN);
	printf("direct doing ( %s )\n", cmdbuf);
	sprintf(cmd, "%s\n", cmdbuf);
	/* Open the command for reading. */
	fp = popen(cmd, "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}
	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path)-1, fp) != NULL) {
		printf("fget = %s", path);
		sprintf(buf_tmp, "%s%s", buf_tmp, path);
	}
	/* close */
	pclose(fp);
	#endif
}

int system_if(char *buf, BOOL bretstr)
{
	char *pwlan1=NULL, *pwlan2=NULL, *pwlan3=NULL, cmd[128]= {};
	int cat_test = 1;
	int scan_wlanstr_id = 0;
	if (buf == NULL)
		return -1;
	cat_test = strncmp(buf, "cat", 3);
	if (cat_test == 0) {
		printf("cat doing ( %s )\n", buf);
		sprintf(cmd, "%s\n", buf);
		cmd_open_retstr(cmd);
		return 1;
	} else {
		printf("prepare in system_if \n");
		char wlan0[] = "wlan0";
		pwlan1 = str_replace(buf, wlan0, wlan0str);//replace "wlan0" with "wl1"
		printf("wlan0 convert ( %s )\n", pwlan1);
		sscanf(buf, "iwpriv wlan%d mp_", &scan_wlanstr_id);
		if (scan_wlanstr_id == 1) {
			char wlan1[] = "wlan1";
			pwlan2 = str_replace(pwlan1, wlan1, wlan0str);//replace "wlan1" with "wl0"
			printf("pwlan2 ( %s )\n", pwlan2);
		} else
			pwlan2 = pwlan1;
		if (strstr(pwlan2, "iwpriv")) {
			char iwpriv[] = "iwpriv";
			char rtwpriv[] = "rtwpriv";
			pwlan3 = str_replace(pwlan2, iwpriv, rtwpriv);//replace "wlan1" with "wl0"
			sprintf(cmd, "%s\n", pwlan3);
			if (bretstr) {
				printf("cmd_open_retstr doing ( %s )\n", cmd);
				cmd_open_retstr(cmd);
				strcpy(buf, buf_tmp);
			} else {
				printf("system direct doing ( %s )\n", cmd);
				system(cmd);
			}
		} else if (strstr(pwlan2, "ifconfig")) {
			sprintf(cmd, "%s\n", pwlan2);
			system(cmd);
		} else {
			sprintf(cmd, "%s\n", buf);
			printf("cmd_open_retstr doing ( %s )\n", cmd);
			cmd_open_retstr(cmd);
		}
	}
	return 1;
}
#else
#define system_if(buf) system(buf)
#endif

int do_ioctrl(char *substr)
{
	int ret = 0;
	char split[]= ":";
	char tmpc[2048];
	// char *substr = "efuse_get";
	char *s = strstr(buf, substr);
	printf("%s\n", s);
	RTW_MEMCPY(buf_tmp, s, strlen(s));
	ret = rtw_io_ctrl(wlan0str, buf_tmp, TRUE);
	if (ret != 0) {
		RTW_MEMSET(buf, 0, BUFLEN);
		RTW_MEMSET(buf_tmp, 0, BUFLEN);
		strcpy(buf, "ioctrl fail\n");
		strcpy(buf_tmp, "ioctrl fail\n");
	}
	usleep(1000);
	printf("io return buf= %s\n", buf_tmp);
	strcpy(tmpc, substr);
	strcat(tmpc, split);
	strcat(tmpc, buf_tmp);
	strcpy(buf_tmp, tmpc);
	printf("final return = %s\n", buf_tmp);
	return ret;
}

//int main(int argc, char **argv) {
int rtw_udpsrv_start(char* wlanname)
{
	int sockfd;                     				// socket descriptors
	struct sockaddr_in my_addr;     		// my address information
	struct sockaddr_in their_addr;  			// connector\A1\A6s address information
	socklen_t addr_len;
	int numbytes;
	FILE *fp;				// buffer that stores message
	static char cmdWrap[BUFFLEN_MAX];
	static int rwHW=0;
	static int isflashread=0;
	char *pbuf;
	int ret = 0;
	char wlan0[]= "wlan0";
#if 0
	if ((strcmp("-v", argv[1]) == 0) || (strcmp("v", argv[1]) == 0)) {
		printf("Udp Server Version:%s\n", udpSrv_version);
		return 0;
	}
	if (argc < 2) {
		printf("Please Input parameter wlan interface: wlan0\n");
		return 0;
	}
#endif
//	if (argc > 1) {
	strcpy(wlan0str, wlanname);
	printf(" Got wlan name:%s \n", wlan0str);
//	}
	// create a socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	my_addr.sin_family = AF_INET;         		// host byte order
	my_addr.sin_port = htons(MYPORT);     	// short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; 	// automatically fill with my IP
	RTW_MEMSET(&(my_addr.sin_zero), '\0', 8); 	// zero the rest of the struct
	// bind the socket with the address
	if (bind(sockfd, (struct sockaddr *)&my_addr,
		 sizeof(struct sockaddr)) == -1) {
		perror("bind");
		close(sockfd);
		exit(1);
	}
	addr_len = sizeof(struct sockaddr);
	printf("MP AUTOMATION daemon (ver %s), Port %d\n", udpSrv_version, MYPORT);
	// main loop : wait for the client
	while (1) {
		//receive the command from the client
		RTW_MEMSET(buf, 0, BUFLEN);
		RTW_MEMSET(buf_tmp, 0, BUFLEN);
		RTW_MEMSET(cmdWrap, 0, BUFFLEN_MAX);
		rwHW = 0;
		usleep(5000);
		if ((numbytes = recvfrom(sockfd, buf, BUFLEN, 0,
					 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			RTW_FPRINT(stderr,"Receive failed!!!\n");
			close(sockfd);
			exit(1);
		}
#ifdef CONFIG_DBG
		RTW_FPRINT(stderr,"received command (%s) from IP:%s\n", buf, inet_ntoa(their_addr.sin_addr));
#endif
		if (!memcmp(buf, "iwpriv wlan0 mp_ther", 20)) {
			strcpy(buf, pre_result);
			char mp_str[] = "mp_ther";
			strcpy(buf_tmp, mp_str);
			ret = rtw_io_ctrl(wlan0str, buf_tmp, TRUE);
			RTW_MEMSET(buf, 0, BUFLEN);
			if (ret == 0)
				strcpy(buf, buf_tmp);
			else
				strcpy(buf, "mp_query ioctrl fail\n");
			printf("buf= %s ..............\n", buf_tmp);
			usleep(1000);
		} else if (!memcmp(buf, "iwpriv wlan0 mp_rssi", 20) && buf[20] == ' ') {
			//RTW_MEMCPY(buf_tmp, buf+21, strlen(buf)-21);
			//rtw_io_ctrl(wlan0str, buf_tmp, MP_QUERY_RSSI);
			//strcpy(buf, buf_tmp);
			//printf("rssi buf= %s\n",buf);
			strcpy(buf, "Error , Not supported !!!!.");
			usleep(1000);
			ret = -1;
		} else if (!memcmp(buf, "iwpriv wlan0 mp_query", 21)) {
			strcpy(buf, pre_result);
			char mp_str[] = "mp_query";
			RTW_MEMCPY(buf_tmp, mp_str, strlen(mp_str));
			strcpy(buf_tmp, "mp_query\n");
			ret = rtw_io_ctrl(wlan0str, buf_tmp, TRUE);
			if (ret == 0)
				strcpy(buf, buf_tmp);
			else
				strcpy(buf, "mp_query ioctrl fail\n");
			usleep(1000);
			printf("w0 2b= %s\n",buf);
		} else if (!memcmp(buf, "iwpriv wlan0 mp_start", 21)) {
			//strcpy(buf, pre_result);
			char mp_str[] = "mp_start";
			RTW_MEMCPY(buf_tmp, mp_str, strlen(mp_str));
			strcpy(buf_tmp, "mp_start\n");
			ret = rtw_io_ctrl(wlan0str, buf_tmp, TRUE);
			if (ret != 0)
				strcpy(buf, "mp_start ioctrl fail\n");
			else
				sprintf(buf, "%s ok", buf);
			usleep(1000);
			printf("w0 2b= %s\n", buf);
		} else if (!memcmp(buf, "iwpriv wlan0 mp_arx phy", 23)) {
			strcpy(buf, pre_result);
			char mp_str[] = "mp_arx phy";
			RTW_MEMCPY(buf_tmp, mp_str, strlen(mp_str));
			strcpy(buf_tmp, "mp_arx phy\n");
			ret = rtw_io_ctrl(wlan0str, buf_tmp, TRUE);
			if (ret == 0)
				strcpy(buf, buf_tmp);
			else
				strcpy(buf, "mp_arx phy ioctrl fail\n");
			usleep(1000);
			printf("w0 2b= %s\n",buf);
		} else if (!memcmp(buf, "iwpriv wlan0 mp_arx rssi", 24)) {
			system_if(buf, true);
		} else if (!memcmp(buf, "iwpriv wlan0 mp_get_tsside", 26)) {
			system_if(buf, true);
		} else {
			if ((!memcmp(buf, "flash read", 10)) ) {
				if ((fp = fopen("/tmp/MP.txt", "r")) == NULL)
					RTW_FPRINT(stderr, "opening MP.txt failed !\n");
				if (fp) {
					isflashread=1;
					fgets(buf, BUFLEN, fp);
					buf[BUFLEN-1] = '\0';
					{
						//fix read_reg bug
						if ( strncmp(buf,strread,strlen(strread))==0 )
							get_read_reg_value( fp, buf, BUFLEN );
						if ( strncmp(buf,strreadrf,strlen(strreadrf))==0 )
							get_read_reg_value( fp, buf, BUFLEN );
						if ( strncmp(buf,strpsd,strlen(strpsd))==0 ) {
							get_read_reg_value( fp, buf, BUFLEN );
						}
						if ( strncmp(buf,strpsd1,strlen(strpsd1))==0 ) {
							get_read_reg_value( fp, buf, BUFLEN );
						}
					}
					fclose(fp);
				}
				pbuf = str_replace(buf, wlan0str, wlan0);
				sprintf(pre_result, "data:%s", pbuf);
			} else if (!memcmp(buf, "flash get", 9))
				sprintf(pre_result, "%s > /tmp/MP.txt ok", buf);
			else
				sprintf(pre_result, "%s ok", buf);
			if (!memcmp(buf, "iwpriv wlan0 mp_brx stop", 24)) {
				strcpy(buf, "stop");
				//MP_get_ext(wlan0str, buf);
			} else if (!memcmp(buf, "iwpriv wlan0 mp_tx", 18) && buf[18] == ' ') {
				char* buf_src_tmp = buf+19;
				RTW_MEMCPY(buf_tmp, buf_src_tmp, strlen(buf)-19);
				//MP_get_ext(wlan0str, buf_tmp, MP_TX_PACKET);
				strcpy(buf, buf_tmp);
			} else if (!memcmp(buf, "iwpriv wlan0 read_reg", 21)) {
				char substr[] = "read_reg";
				do_ioctrl(substr);
			} else if (!memcmp(buf, "iwpriv wlan0 read_rf", 20)) {
				char substr[] = "read_rf";
				do_ioctrl(substr);
			} else if (!memcmp(buf, "iwpriv wlan0 efuse_get", 22)) {
				char substr[] = "efuse_get";
				do_ioctrl(substr);
			} else if (!memcmp(buf, "iwpriv wlan0 efuse_set", 22)) {
				strcat(buf, " > /tmp/MP.txt");
				system_if(buf, false);
			} else if (!memcmp(buf, "iwpriv wlan0 efuse_sync", 23)) {
				char substr[] = "efuse_update";
				do_ioctrl(substr);
			} else if (!memcmp(buf, "iwpriv wlan0 mp_psd", 19)) {
				char substr[] = "mp_psd";
				do_ioctrl(substr);
			} else if (!memcmp(buf, "iwpriv wlan0 mp_pwrctldm", 24)) {
				char substr[] = "mp_pwrctldm";
				do_ioctrl(substr);
			} else if (!memcmp(buf, "iwpriv wlan0 mp_drvquery autoload", 32)) {
				char substr[] = "mp_drvquery";
				do_ioctrl(substr);
			} else if (!memcmp(buf, "probe", 5))
				strcpy(buf, "ack");
			else {
				if (!memcmp(buf, "flash get", 9)) {
					sprintf(cmdWrap, "flash gethw %s", buf+10);
					rwHW = 1;
					////strcat(buf, " > /tmp/MP.txt");
					strcat(cmdWrap, " > /tmp/MP.txt");
				}
				if (!memcmp(buf, "flash set", 9)) {
					sprintf(cmdWrap, "flash sethw %s", buf+10);
					rwHW = 1;
					//printf("1 sent command (%s) to IP:%s\n", pre_result, inet_ntoa(their_addr.sin_addr));
					if ((numbytes = sendto(sockfd, pre_result, strlen(pre_result), 0,
							       (struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) {
						RTW_FPRINT(stderr, "send failed\n");
						close(sockfd);
						exit(1);
					}
					//printf("2 sent command (%s) to IP:%s\n", pre_result, inet_ntoa(their_addr.sin_addr));
				}
				if (rwHW == 1) {
					system_if(cmdWrap, false);
				} else {
					//if (memcmp(buf, "wlan0     read_reg", 18) && memcmp(buf, "wlan0     read_rf", 17) && memcmp(buf, "wlan1     read_reg", 18) && memcmp(buf, "wlan1     read_rf", 17))
					if (isflashread==0)
						system_if(buf, false);
					else
						isflashread=0;
				}
			}
			if (strlen(buf_tmp) == 0)
				strcpy(buf_tmp, pre_result);
			strcpy(pre_result, buf);
			strcpy(buf, buf_tmp);
		}
		//printf("buf = (%s) len %d \n", buf, strlen(buf));
		if (memcmp(buf, "flash set", 9) != 0) {
			//printf("return sent command (%s) len %d, to IP:%s\n", buf, strlen(buf), inet_ntoa(their_addr.sin_addr));
			if ((numbytes = sendto(sockfd, buf, strlen(buf), 0,
					       (struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) {
				RTW_FPRINT(stderr, "send failed\n");
				close(sockfd);
				exit(1);
			}
			//printf("2 sent command (%s) to IP:%s\n", buf, inet_ntoa(their_addr.sin_addr));
			printf("buf %s , strlen buf = (%d) numbytes %d \n", buf, (int)strlen(buf), numbytes);
		}
	}//While 1 end
	return 0;
}


