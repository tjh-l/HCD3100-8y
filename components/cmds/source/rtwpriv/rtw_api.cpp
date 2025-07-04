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
extern "C" {

#include "rtwpriv.h"
#include "rtw_api.h"
/*------------------------------------------------------------------*/
/*
 * Open a socket.
 * Depending on the protocol present, open the right socket. The socket
 * will allow us to talk to the driver.
 */

const char* HW_rateindex_Array[] = { "1M","2M","5.5M","11M","6M","9M","12M","18M","24M","36M","48M","54M",
				     "HTMCS0","HTMCS1","HTMCS2","HTMCS3","HTMCS4","HTMCS5","HTMCS6","HTMCS7",
				     "HTMCS8","HTMCS9","HTMCS10","HTMCS11","HTMCS12","HTMCS13","HTMCS14","HTMCS15",
				     "HTMCS16","HTMCS17","HTMCS18","HTMCS19","HTMCS20","HTMCS21","HTMCS22","HTMCS23",
				     "HTMCS24","HTMCS25","HTMCS26","HTMCS27","HTMCS28","HTMCS29","HTMCS30","HTMCS31",
				     "VHT1MCS0","VHT1MCS1","VHT1MCS2","VHT1MCS3","VHT1MCS4","VHT1MCS5","VHT1MCS6","VHT1MCS7","VHT1MCS8","VHT1MCS9",
				     "VHT2MCS0","VHT2MCS1","VHT2MCS2","VHT2MCS3","VHT2MCS4","VHT2MCS5","VHT2MCS6","VHT2MCS7","VHT2MCS8","VHT2MCS9",
				     "VHT3MCS0","VHT3MCS1","VHT3MCS2","VHT3MCS3","VHT3MCS4","VHT3MCS5","VHT3MCS6","VHT3MCS7","VHT3MCS8","VHT3MCS9",
				     "VHT4MCS0","VHT4MCS1","VHT4MCS2","VHT4MCS3","VHT4MCS4","VHT4MCS5","VHT4MCS6","VHT4MCS7","VHT4MCS8","VHT4MCS9",
					"HE1MCS0", "HE1MCS1", "HE1MCS2", "HE1MCS3", "HE1MCS4", "HE1MCS5",
					"HE1MCS6", "HE1MCS7", "HE1MCS8", "HE1MCS9", "HE1MCS10", "HE1MCS11",
					"HE2MCS0", "HE2MCS1", "HE2MCS2", "HE2MCS3", "HE2MCS4", "HE2MCS5",
					"HE2MCS6", "HE2MCS7", "HE2MCS8", "HE2MCS9", "HE2MCS10", "HE2MCS11",
					"HE3MCS0", "HE3MCS1", "HE3MCS2", "HE3MCS3", "HE3MCS4", "HE3MCS5",
					"HE3MCS6", "HE3MCS7", "HE3MCS8", "HE3MCS9", "HE3MCS10", "HE3MCS11",
					"HE4MCS0", "HE4MCS1", "HE4MCS2", "HE4MCS3", "HE4MCS4", "HE4MCS5",
					"HE4MCS6", "HE4MCS7", "HE4MCS8", "HE4MCS9", "HE4MCS10", "HE4MCS11"};
int iw_sockets_open(void)
{
	static const int families[] = {
		AF_INET,
		AF_IPX,
#ifdef PLATFORM_LINUX
		AF_AX25,
#endif
		AF_APPLETALK
	};
	unsigned int	i;
	int		sock;
	/*
	 * Now pick any (exisiting) useful socket family for generic queries
	 * Note : don't open all the socket, only returns when one matches,
	 * all protocols might not be valid.
	 * Workaround by Jim Kaba <jkaba@sarnoff.com>
	 * Note : in 99% of the case, we will just open the inet_sock.
	 * The remaining 1% case are not fully correct...
	 */
	/* Try all families we support */
	for (i = 0; i < sizeof(families)/sizeof(int); ++i) {
		/* Try to open the socket, if success returns it */
		sock = socket(families[i], SOCK_DGRAM, 0);
		if (sock >= 0)
			return sock;
	}
	return -1;
}



/*------------------------------------------------------------------*/
/*
 * Close the socket used for ioctl.
 */
void iw_sockets_close(int skfd)
{
	close(skfd);
}

int wlan_ioctl_mp(int skfd, char *ifname, void *pBuffer, unsigned int BufferSize)
{
	int err;
	struct iwreq iwr;

	err = 0;

	memset(&iwr, 0, sizeof(struct iwreq));
	strncpy(iwr.ifr_ifrn.ifrn_name, ifname, strlen(ifname));

	iwr.u.data.pointer = pBuffer;
	iwr.u.data.length = (unsigned short)BufferSize;

	err = ioctl(skfd, RTW_IOCTL_MP, &iwr);

	if (iwr.u.data.length == 0)
		*(char *)pBuffer = 0;

	return err;
}

int rtw_io_ctrl(char *ifname, char *inbuf, bool isudp)
{
	int skfd, err = 0;
	char cp_inbuf[BUF_SIZE];
	char in_cmd[BUF_SIZE];
	sscanf(inbuf, "%s ", in_cmd);
	sprintf(cp_inbuf, "%s", inbuf);
	skfd = iw_sockets_open();
	err = wlan_ioctl_mp(skfd, ifname, inbuf, strlen(inbuf)+1);
	iw_sockets_close(skfd);
	if (err < 0) {
		RTW_FPRINT(stderr, "Interface doesn't accept private ioctl...\n");
		RTW_FPRINT(stderr, "%s: %s\n", cp_inbuf, rtw_strerror(errno));
	} else {
		if (strlen(inbuf) != 0)
			printf("%-8.16s %s:%s\n", ifname, in_cmd, inbuf);
		err = 0;
	}
	if (err < 0) {
		printf("CMD ioctrl Error = %s\n", in_cmd);
	} else {
		if (isudp != TRUE)
			RTW_MEMSET(inbuf, 0, BUF_SIZE);
	}
	return err;
}

void rtw_io_ctrl_nochk(char *ifname, char *inbuf)
{
	int skfd = 0;
	char cp_inbuf[BUF_SIZE];
	char in_cmd[32];
	sscanf(inbuf, "%s ", in_cmd);
	sprintf(cp_inbuf, "%s", inbuf);
	skfd = iw_sockets_open();
	wlan_ioctl_mp(skfd, ifname, inbuf, strlen(inbuf)+1);
	iw_sockets_close(skfd);
	return;
}


u8 key_char2num(u8 ch)
{
	if ((ch>='0')&&(ch<='9'))
		return ch - '0';
	else if ((ch>='a')&&(ch<='f'))
		return ch - 'a' + 10;
	else if ((ch>='A')&&(ch<='F'))
		return ch - 'A' + 10;
	else
		return 0xff;
}

u8 key_2char2num(u8 hch, u8 lch)
{
	return ((key_char2num(hch) << 4) | key_char2num(lch));
}

void macstr2num(u8 *dst, u8 *src)
{
	int	jj, kk;
	for (jj = 0, kk = 0; jj < 6; jj++, kk += 3) {
		dst[jj] = key_2char2num(src[kk], src[kk + 1]);
	}
}

void dump_buf(char *buf, int len)
{
	int i;
	printf("-----------------Len %d----------------\n", len);
	for (i=0; i<len; i++)
		printf("%2.2x-", *(buf+i));
	printf("\n");
}

int randInRange( int min, int max )
{
	double scale = 1.0 / ((double)RAND_MAX + 1);
	double range = max - min + 1;
	return min + (int) ( rand() * scale * range );
}

UCHAR randomByte()
{
	return (UCHAR) randInRange( 0, 255 );
}

void split(char **arr, char *str, char *del)
{
	char *s = strtok(str, del);
	while (s != NULL) {
		*arr++ = s;
		s = strtok(NULL, del);
	}
}

int wifirate2_ratetbl_inx(int rate)
{
	int	inx = 0;
	rate = rate & 0x7f;
	switch (rate) {
	case 54*2:
		inx = 11;
		break;
	case 48*2:
		inx = 10;
		break;
	case 36*2:
		inx = 9;
		break;
	case 24*2:
		inx = 8;
		break;
	case 18*2:
		inx = 7;
		break;
	case 12*2:
		inx = 6;
		break;
	case 9*2:
		inx = 5;
		break;
	case 6*2:
		inx = 4;
		break;
	case 11*2:
		inx = 3;
		break;
	case 11:
		inx = 2;
		break;
	case 2*2:
		inx = 1;
		break;
	case 1*2:
		inx = 0;
		break;
	}
	return inx;
}

u8 HwRateToMPTRate(u8 rate)
{
	u8	ret_rate = RATE_CCK_1M;
	switch (rate) {
	case DESC_RATE1M:
		ret_rate = RATE_CCK_1M;
		break;
	case DESC_RATE2M:
		ret_rate = RATE_CCK_2M;
		break;
	case DESC_RATE5_5M:
		ret_rate = RATE_CCK_55M;
		break;
	case DESC_RATE11M:
		ret_rate = RATE_CCK_11M;
		break;
	case DESC_RATE6M:
		ret_rate = RATE_OFDM_6M;
		break;
	case DESC_RATE9M:
		ret_rate = RATE_OFDM_9M;
		break;
	case DESC_RATE12M:
		ret_rate = RATE_OFDM_12M;
		break;
	case DESC_RATE18M:
		ret_rate = RATE_OFDM_18M;
		break;
	case DESC_RATE24M:
		ret_rate = RATE_OFDM_24M;
		break;
	case DESC_RATE36M:
		ret_rate = RATE_OFDM_36M;
		break;
	case DESC_RATE48M:
		ret_rate = RATE_OFDM_48M;
		break;
	case DESC_RATE54M:
		ret_rate = RATE_OFDM_54M;
		break;
	case DESC_RATEMCS0:
		ret_rate = RATE_MCS0;
		break;
	case DESC_RATEMCS1:
		ret_rate = RATE_MCS1;
		break;
	case DESC_RATEMCS2:
		ret_rate = RATE_MCS2;
		break;
	case DESC_RATEMCS3:
		ret_rate = RATE_MCS3;
		break;
	case DESC_RATEMCS4:
		ret_rate = RATE_MCS4;
		break;
	case DESC_RATEMCS5:
		ret_rate = RATE_MCS5;
		break;
	case DESC_RATEMCS6:
		ret_rate = RATE_MCS6;
		break;
	case DESC_RATEMCS7:
		ret_rate = RATE_MCS7;
		break;
	case DESC_RATEMCS8:
		ret_rate = RATE_MCS8;
		break;
	case DESC_RATEMCS9:
		ret_rate = RATE_MCS9;
		break;
	case DESC_RATEMCS10:
		ret_rate = RATE_MCS10;
		break;
	case DESC_RATEMCS11:
		ret_rate = RATE_MCS11;
		break;
	case DESC_RATEMCS12:
		ret_rate = RATE_MCS12;
		break;
	case DESC_RATEMCS13:
		ret_rate = RATE_MCS13;
		break;
	case DESC_RATEMCS14:
		ret_rate = RATE_MCS14;
		break;
	case DESC_RATEMCS15:
		ret_rate = RATE_MCS15;
		break;
	case DESC_RATEMCS16:
		ret_rate = RATE_MCS16;
		break;
	case DESC_RATEMCS17:
		ret_rate = RATE_MCS17;
		break;
	case DESC_RATEMCS18:
		ret_rate = RATE_MCS18;
		break;
	case DESC_RATEMCS19:
		ret_rate = RATE_MCS19;
		break;
	case DESC_RATEMCS20:
		ret_rate = RATE_MCS20;
		break;
	case DESC_RATEMCS21:
		ret_rate = RATE_MCS21;
		break;
	case DESC_RATEMCS22:
		ret_rate = RATE_MCS22;
		break;
	case DESC_RATEMCS23:
		ret_rate = RATE_MCS23;
		break;
	case DESC_RATEMCS24:
		ret_rate = RATE_MCS24;
		break;
	case DESC_RATEMCS25:
		ret_rate = RATE_MCS25;
		break;
	case DESC_RATEMCS26:
		ret_rate = RATE_MCS26;
		break;
	case DESC_RATEMCS27:
		ret_rate = RATE_MCS27;
		break;
	case DESC_RATEMCS28:
		ret_rate = RATE_MCS28;
		break;
	case DESC_RATEMCS29:
		ret_rate = RATE_MCS29;
		break;
	case DESC_RATEMCS30:
		ret_rate = RATE_MCS30;
		break;
	case DESC_RATEMCS31:
		ret_rate = RATE_MCS31;
		break;
	case DESC_RATEVHTSS1MCS0:
		ret_rate = RATE_VHT1SS_MCS0;
		break;
	case DESC_RATEVHTSS1MCS1:
		ret_rate = RATE_VHT1SS_MCS1;
		break;
	case DESC_RATEVHTSS1MCS2:
		ret_rate = RATE_VHT1SS_MCS2;
		break;
	case DESC_RATEVHTSS1MCS3:
		ret_rate = RATE_VHT1SS_MCS3;
		break;
	case DESC_RATEVHTSS1MCS4:
		ret_rate = RATE_VHT1SS_MCS4;
		break;
	case DESC_RATEVHTSS1MCS5:
		ret_rate = RATE_VHT1SS_MCS5;
		break;
	case DESC_RATEVHTSS1MCS6:
		ret_rate = RATE_VHT1SS_MCS6;
		break;
	case DESC_RATEVHTSS1MCS7:
		ret_rate = RATE_VHT1SS_MCS7;
		break;
	case DESC_RATEVHTSS1MCS8:
		ret_rate = RATE_VHT1SS_MCS8;
		break;
	case DESC_RATEVHTSS1MCS9:
		ret_rate = RATE_VHT1SS_MCS9;
		break;
	case DESC_RATEVHTSS2MCS0:
		ret_rate = RATE_VHT2SS_MCS0;
		break;
	case DESC_RATEVHTSS2MCS1:
		ret_rate = RATE_VHT2SS_MCS1;
		break;
	case DESC_RATEVHTSS2MCS2:
		ret_rate = RATE_VHT2SS_MCS2;
		break;
	case DESC_RATEVHTSS2MCS3:
		ret_rate = RATE_VHT2SS_MCS3;
		break;
	case DESC_RATEVHTSS2MCS4:
		ret_rate = RATE_VHT2SS_MCS4;
		break;
	case DESC_RATEVHTSS2MCS5:
		ret_rate = RATE_VHT2SS_MCS5;
		break;
	case DESC_RATEVHTSS2MCS6:
		ret_rate = RATE_VHT2SS_MCS6;
		break;
	case DESC_RATEVHTSS2MCS7:
		ret_rate = RATE_VHT2SS_MCS7;
		break;
	case DESC_RATEVHTSS2MCS8:
		ret_rate = RATE_VHT2SS_MCS8;
		break;
	case DESC_RATEVHTSS2MCS9:
		ret_rate = RATE_VHT2SS_MCS9;
		break;
	case DESC_RATEVHTSS3MCS0:
		ret_rate = RATE_VHT3SS_MCS0;
		break;
	case DESC_RATEVHTSS3MCS1:
		ret_rate = RATE_VHT3SS_MCS1;
		break;
	case DESC_RATEVHTSS3MCS2:
		ret_rate = RATE_VHT3SS_MCS2;
		break;
	case DESC_RATEVHTSS3MCS3:
		ret_rate = RATE_VHT3SS_MCS3;
		break;
	case DESC_RATEVHTSS3MCS4:
		ret_rate = RATE_VHT3SS_MCS4;
		break;
	case DESC_RATEVHTSS3MCS5:
		ret_rate = RATE_VHT3SS_MCS5;
		break;
	case DESC_RATEVHTSS3MCS6:
		ret_rate = RATE_VHT3SS_MCS6;
		break;
	case DESC_RATEVHTSS3MCS7:
		ret_rate = RATE_VHT3SS_MCS7;
		break;
	case DESC_RATEVHTSS3MCS8:
		ret_rate = RATE_VHT3SS_MCS8;
		break;
	case DESC_RATEVHTSS3MCS9:
		ret_rate = RATE_VHT3SS_MCS9;
		break;
	case DESC_RATEVHTSS4MCS0:
		ret_rate = RATE_VHT4SS_MCS0;
		break;
	case DESC_RATEVHTSS4MCS1:
		ret_rate = RATE_VHT4SS_MCS1;
		break;
	case DESC_RATEVHTSS4MCS2:
		ret_rate = RATE_VHT4SS_MCS2;
		break;
	case DESC_RATEVHTSS4MCS3:
		ret_rate = RATE_VHT4SS_MCS3;
		break;
	case DESC_RATEVHTSS4MCS4:
		ret_rate = RATE_VHT4SS_MCS4;
		break;
	case DESC_RATEVHTSS4MCS5:
		ret_rate = RATE_VHT4SS_MCS5;
		break;
	case DESC_RATEVHTSS4MCS6:
		ret_rate = RATE_VHT4SS_MCS6;
		break;
	case DESC_RATEVHTSS4MCS7:
		ret_rate = RATE_VHT4SS_MCS7;
		break;
	case DESC_RATEVHTSS4MCS8:
		ret_rate = RATE_VHT4SS_MCS8;
		break;
	case DESC_RATEVHTSS4MCS9:
		ret_rate = RATE_VHT4SS_MCS9;
		break;
	default:
		printf("hw_rate_to_m_rate(): Non supported Rate [%x]!!!\n", rate);
		break;
	}
	return ret_rate;
}


int rtw_mpRateParseFunc(char *targetStr)
{
	int i=0;
	int mptRate = 108;
	int rateidx = 0;
	for (i = 0; i <= 83; i++) {
		if (strcmp(targetStr, HW_rateindex_Array[i]) == 0) {
			DBG("index = %d \n",i);
			rateidx = i;
			return rateidx;
		}
	}
	if (rateidx == 0 && strcmp(targetStr, "1M") != 0) {
		mptRate = atoi(targetStr);
		if (mptRate <= 127)
			rateidx = wifirate2_ratetbl_inx(mptRate);
		else if (mptRate < 200)
			rateidx = (mptRate - 128 + 12);
		return rateidx;
	}
	printf("\n	#### Please input a Data RATE String as:######\n");
	for (i=0; i<=83; i++) {
		printf("%s ",HW_rateindex_Array[i]);
		if ((i%10 == 0) && (i != 0))
			printf("\n");
	}
	printf("\n Can't find rate index, Press any key to exit & input again...");
	getchar();
	exit(0);
}

u16 rtw_ant_strpars(char *targetStr)
{
	u16 CurrentAntenna;
	if (strcmp(targetStr, "d") == 0) {
		CurrentAntenna = ANTENNA_D;
	} else if (strcmp(targetStr, "c") == 0) {
		CurrentAntenna = ANTENNA_C;
	} else if (strcmp(targetStr, "cd") == 0) {
		CurrentAntenna = ANTENNA_CD;
	} else if (strcmp(targetStr, "b") == 0) {
		CurrentAntenna = ANTENNA_B;
	} else if (strcmp(targetStr, "bd") == 0) {
		CurrentAntenna = ANTENNA_BD;
	} else if (strcmp(targetStr, "bc") == 0) {
		CurrentAntenna = ANTENNA_BC;
	} else if (strcmp(targetStr, "bcd") == 0) {
		CurrentAntenna = ANTENNA_BCD;
	} else if (strcmp(targetStr, "a") == 0) {
		CurrentAntenna = ANTENNA_A;
	} else if (strcmp(targetStr, "ad") == 0) {
		CurrentAntenna = ANTENNA_AD;
	} else if (strcmp(targetStr, "ac") == 0) {
		CurrentAntenna = ANTENNA_AC;
	} else if (strcmp(targetStr, "acd") == 0) {
		CurrentAntenna = ANTENNA_ACD;
	} else if (strcmp(targetStr, "ab") == 0) {
		CurrentAntenna = ANTENNA_AB;
	} else if (strcmp(targetStr, "abd") == 0) {
		CurrentAntenna = ANTENNA_ABD;
	} else if (strcmp(targetStr, "abc") == 0) {
		CurrentAntenna = ANTENNA_ABC;
	} else if (strcmp(targetStr, "abcd") == 0) {
		CurrentAntenna = ANTENNA_ABCD;
	} else {
		CurrentAntenna = ANTENNA_A;
	}
	DBG("%s :Config CurrentAntenna %d\n", __func__, CurrentAntenna);
	return CurrentAntenna;
}


int Read_Parsing_file(PRT_PMAC_TX_INFO pPMacTxInfo2, u16 *Antenna, u8 *UnicastDID)
{
	int pktLength = 1000, pktInterval = 100, pktPattern = 0x00, stbc = 0, ldpc = 0, ndp_sound = 0,SPreambleSGI = 0;
	char line[1024];
	char FileName[] = "rtw_Config.txt";
	FILE *fp = fopen(FileName,"r");
	if (!fp)
		return 0;
	else {
		printf("Try to Open %s file , parsing config file !!!\n",FileName);
		while (fgets(line, 1024, fp)) {
			DBG("read Str %s\n",line);
			if ( strncmp(line, "ConfigSetting=off", 18) == 0) {
				printf("off parsing config!!!\n");
				return 0;
			} else if ( strncmp(line, "PacketLength=", 13) == 0) {
				if (sscanf(line, "	PacketLength=%d", &pktLength) > 0) {
					printf("PacketLength= %d\n", pktLength);
					pPMacTxInfo2->PacketLength = pktLength;
				} else
					pPMacTxInfo2->PacketLength = 1000;
			} else if ( strncmp(line, "PacketPeriod=", 13) == 0) {
				if (sscanf(line, "PacketPeriod=%d", &pktInterval) > 0) {
					printf("PacketPeriod= %d\n", pktInterval);
					pPMacTxInfo2->PacketPeriod = pktInterval;
				} else
					pPMacTxInfo2->PacketPeriod = 100;
			} else if ( strncmp(line, "PacketPattern=", 13) == 0) {
				if ( strncmp(line, "PacketPattern=rand", 18) == 0) {
					printf("PacketPattern= rand\n");
					pPMacTxInfo2->PacketPattern = randomByte();
				} else if (sscanf(line, "PacketPattern=%x", &pktPattern) > 0) {
					printf("PacketPattern= 0x%x\n", pktPattern);
					pPMacTxInfo2->PacketPattern = pktPattern;
				} else {
					printf("Unkonw PacketPattern,Defualt Random Pattern\n");
					pPMacTxInfo2->PacketPattern = randomByte();
				}
			} else if ( strncmp(line, "LDPC=", 5) == 0) {
				if (sscanf(line, "LDPC=%d", &ldpc) > 0) {
					printf("LDPC= %d\n", ldpc);
					pPMacTxInfo2->bLDPC = ldpc;
				}
			} else if ( strncmp(line, "STBC=", 5) == 0) {
				if (sscanf(line, "STBC=%d", &stbc) > 0) {
					printf("STBC= %d\n", stbc);
					pPMacTxInfo2->bSTBC = stbc;
				}
			} else if ( strncmp(line, "SPreambleSGI=", 13) == 0) {
				if (sscanf(line, "SPreambleSGI=%d", &SPreambleSGI) > 0) {
					printf("SPreamble & SGI= %d\n", SPreambleSGI);
					pPMacTxInfo2->bSPreamble = SPreambleSGI;
					pPMacTxInfo2->bSGI = SPreambleSGI;
				}
			} else if ( strncmp(line, "NDP_sound=", 10) == 0) {
				if (sscanf(line, "NDP_sound=%d", &ndp_sound) > 0) {
					printf("NDP_sound= %d\n", ndp_sound);
					pPMacTxInfo2->NDP_sound = ndp_sound;
				}
			} else if ( strncmp(line, "TxAnt=", 6) == 0) {
				const char *delim = "=";
				char * pch;
				u8 i;
				u16 antenna = 0;
				pch = strtok(line,delim);
				pch = strtok (NULL, delim);
				for (i = 0; i < strlen(pch); i++) {
					switch (pch[i]) {
					case 'A':
					case 'a':
						antenna |= ANTENNA_A;
						break;
					case 'B':
					case 'b':
						antenna |= ANTENNA_B;
						break;
					case 'C':
					case 'c':
						antenna |= ANTENNA_C;
						break;
					case 'D':
					case 'd':
						antenna |= ANTENNA_D;
						break;
					}
				}
				*Antenna = antenna;
				printf("%s :Config Current Antenna %d\n", __func__, *Antenna);
			} else if ( strncmp(line, "	destaddr=", 10) == 0) {
				const char *delim = "=";
				char * pch;
				//printf ("Splitting string \"%s\" into tokens:\n",line);
				pch = strtok(line,delim);
				pch = strtok (NULL, delim);
				//printf ("%s\n",pch);
				macstr2num(UnicastDID,(u8 *) pch);
			}
		}
		fclose(fp);
		return 1;
	}
	fclose(fp);
	return 0;
}

void rtw_help ()
{
	printf("#####[ 1. SW TX mode ]#####\n");
	printf("\t Input the parameters like this: rtwpriv wlan0 mp_XXX XXX\n");
	printf("	1. mp_start				## enter to MP Mode\n \
	2. mp_bandwidth 40M=[Num]		## input [Num], 0=20M, 1=40M, 2=80M\n \
	3. mp_channel [Num]			## input [Num] = 1~167 channel number\n \
	4. mp_rate [Rate Id Num] or [RateID]\n");
	printf("	5. mp_ant_tx [Path]			## input Antenna [PARH] = a,b,c,d,ab,ac,ad...\n\
	6. mp_txpower patha=[index],pathb=[index],pathc=[index],pathd=[index] 	## input power index range [index] = 1~63\n\
	7. mp_get_txpower [PATH Num]		## Input Antenna PATH Num 0~3 , A = 0, B = 1 , C = 2 , D = 3\n\
	8. mp_ctx [Tx Mode]			##input [ Tx Mode ]\n\
				[a].Continuous Packet Tx = 'background,pkt'\n\
				[b].Continuous Tx = 'background'\n\
				[c].Count Packet Tx = 'count=[num],pkt'\n\
				[d].Carrier suppression testing = 'background,cs'\n\
				[e].Single Tone TX testing = 'background,stone'\n\
	   mp_ctx stop				## Stop Tx Mode\n\
	9. mp_query				## Get Tx/Rx Packet Counter.\n\n");
	printf("##### [ 2. HW TX mode ] VHT/HE IC Suppport Only #####\n");
	printf("\tInput the parameters like this:\n\trtwpriv wlan0 [Channel] [Bandwidth] [ANT_PAH] [RateID] [TxMode] [Packet Interval] [PacketLength] [Packet Count] [Packet Pattern]\n\
	[Channel]: 1~177\n\
	[BW]: 0 = 20M, 1 = 40M, 2 = 80M\n \
	[ANT_PAH]: a: PATH A, b: PATH B, c: PATH C, d: PATH D, ab : PATH AB 2x2 ....\n\
	[RateID]:\n\t");
	{
		int bg_idx = 12, n_idx = 32, vht_idx = 40;
		
		for (int i = 0; i <= 131; i++) {
			if (strncmp(HW_rateindex_Array[i], "HE1MCS0", 7) == 0)
				printf("\n\n\t[ 80211AX Support Rate Lists]:\n\t");
			else if (strncmp(HW_rateindex_Array[i], "VHT1MCS0", 8) == 0)
				printf("\n\t[ 80211AC Support Rate Lists]:\n\t");
			printf("%s ", HW_rateindex_Array[i]);
			if ( i == 11)
				printf("\n\t");
			else if (i > 11 && i <= 43) {
				int n_cur =  i - bg_idx;

				if (n_cur % 16 == 0 && n_cur != 0)
					printf("\n\t");
				else if (n_cur == 31)
					printf("\n\t");
			} else if (i > 43 && i <= 83) {
				int vht_cur =  i - (bg_idx + n_idx);

				if ( vht_cur % 9 == 0 && vht_cur != 0)
					printf("\n\t");
			} else if (i > 83) {
				int he_cur =  i - (bg_idx + vht_idx + n_idx);
				if ( he_cur % 11 == 0 && he_cur != 0)
					printf("\n\t");
			}
		}
	}
	printf("\n\n	[ TxMode ]: 1: PACKET Tx 2:CONTINUOUS TX 3:OFDM Single Tone TX\n");
	printf("\t[ PacketLength] (Option): Packet of payload data length, default 1500\n");
	printf("\t[ Packet Count] (Option): count the number of packet to Tx , set 0 for CONTINUOUS Packet TX ,default 0\n");
	printf("\t[ Packet Interval] (Option): 1~65535 us,default 2000\n");
	printf("\t[ Packet Pattern] (Option): 00~ff(hex) ,default random hex\n\n");

	printf("\n##### [ 3. (80211AX) HW TX mode ] AX IC Suppport Only #####\n\n");
	printf("\tInput the parameters like this(CMD format):\n\t\
	rtwpriv wlan0 [Channel] [Bandwidth] [ANT_PAH] [RateID] [TxMode] [ru_tone=(int) [ppdu=(int)]\n\n\
	RU Tone support list(Refer Coding:LDPC):\n\
	RU Tone CMD: [ mp_plcp_data ru_tone=(int) ]\n\
	0 : [26-Tone]\n\
	1 : [52-Tone]\n\
	2 : [106-Tone]\n\
	3 : [242-Tone]\n\
	4 : [484-Tone]\n\
	5 : [966-Tone]\n\n\
	Conifg PPDU Type: 0 = CCK , 1 = LEGACY, 2 = HT_MF, 3 = HT_GF, 4 = VHT, 5 = HE_SU, 6 = HE_ER_SU, 7 = HE_MU_OFDMA, 8 = HE_TB\n\
	Coding CMD:	[mp_plcp_user coding=(int)] ( 0:BCC , 1:LDPC )\n\
	RU Alloc CMD:	[ mp_plcp_user ru_alloc=(int) ]\n");

	printf("\n#####[ 4. RX mode ] #####\n");
	printf("	1. mp_start				## enter to MP Mode\n \
	2. mp_bandwidth 40M=[Num]		## input [Num], 0=20M, 1=40M, 2=80M\n \
	3. mp_channel [Num]			## input [Num] = 1~167 channel number\n \
	5. mp_ant_rx [Path]			## input Antenna [PARH] = a,b,c,d,ab,ac,ad...\n\
	6. mp_arx start				## Enter Rx packet mode\n \
	7. mp_arx phy				## Query Rx Phy Packet Count\n\
	8. mp_reset_stats			## reset all Count\n\n");
	printf("\n#####[ 5. Read/Write RF reg ] #####\n");
	printf("	1. rfr [RF_Path] [Reg_Addr]  ,return hex value, # RF Path= 0/1/2/3 , Reg Addr hex = 0xXX.\n \
	2. rfw [RF_Path] [Reg_Addr] [Value] ,# RF Path= 0/1/2/3 , Reg Addr hex = 0xXX , Value Hex = 0x00000 .\n \
	3. read_rf [RF_Path],[reg_Addr] ,return decimal value ,# RF Path= 0/1/2/3 , Reg Addr hex = 0xXX .\n \
	4. write_rf [RF_Path],[Reg_Addr],[Value] ,# RF Path= 0/1/2/3 , Reg Addr hex = 0xXX , Value Hex = 0x00000 .\n\n");
	printf("\n#####[ 6. Read/Write mac/bb reg ] #####\n");
	printf("	1. read [data width],[Reg_Addr]  ,return hex value, # Data Width= 1/2/4 , Reg Addr hex = 0xXX .\n \
	2. write [data width],[Reg_Add],[Value] ,# Data Width= 1/2/4 , Reg Addr hex = 0xXX , Value Hex = 0x00000 .\n");
}

void ByteToBit(
	UCHAR	*out,
	BOOL	*in,
	UCHAR	in_size
)
{
	UCHAR i = 0, j= 0;
	for (i = 0; i < in_size; i++) {
		for (j = 0; j < 8; j++) {
			if (in[8*i+j])
				out[i] |= (1 << j);
		}
	}
}

void CRC8_generator(
	BOOL *out,				// crc8 output
	BOOL *in,				// binary input
	UCHAR in_size			// size of binary input signal
)
{
	UCHAR i=0;
	BOOL temp=0, reg[]= {1, 1, 1, 1, 1, 1, 1, 1};
	for (i=0; i<in_size; i++) { // take one's complement and bit reverse
		temp=in[i]^reg[7];
		reg[7]	= reg[6];
		reg[6]	= reg[5];
		reg[5]	= reg[4];
		reg[4]	= reg[3];
		reg[3]	= reg[2];
		reg[2]	= reg[1] ^ temp;
		reg[1]	= reg[0] ^ temp;
		reg[0]	= temp;
	}
	for	(i=0; i<8; i++)	// take one's complement and bit reverse
		out[i]=reg[7-i]^1;
}

void CRC16_generator(
	BOOL *out,				// crc16 output
	BOOL *in,				// binary input
	UCHAR in_size			// size of binary input signal
)
{
	UCHAR i = 0;
	BOOL temp = 0, reg[]= {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	for (i=0; i<in_size; i++) { // take one's complement and bit reverse
		temp=in[i]^reg[15];
		reg[15]	= reg[14];
		reg[14]	= reg[13];
		reg[13]	= reg[12];
		reg[12]	= reg[11];
		reg[11]	= reg[10];
		reg[10]	= reg[9];
		reg[9]	= reg[8];
		reg[8]	= reg[7];
		reg[7]	= reg[6];
		reg[6]	= reg[5];
		reg[5]	= reg[4];
		reg[4]	= reg[3];
		reg[3]	= reg[2];
		reg[2]	= reg[1];
		reg[1]	= reg[0];
		reg[12]	= reg[12] ^ temp;
		reg[5]	= reg[5] ^ temp;
		reg[0]	= temp;
	}
	for	(i=0; i<16; i++)	// take one's complement and bit reverse
		out[i]=1-reg[15-i];
}


//========================================
//	SFD		SIGNAL	SERVICE	LENGTH	CRC
//	16 bit	8 bit	8 bit	16 bit	16 bit
//========================================
void CCK_generator(
	PRT_PMAC_TX_INFO	pPMacTxInfo,
	PRT_PMAC_PKT_INFO	pPMacPktInfo)
{
	double	ratio = 0;
	BOOL	crc16_in[32] = {0}, crc16_out[16] = {0};
	if (pPMacTxInfo->bSPreamble)
		pPMacTxInfo->SFD = 0x05CF;
	else
		pPMacTxInfo->SFD = 0xF3A0;
	switch (pPMacPktInfo->MCS) {
	case 0:
		pPMacTxInfo->SignalField=0xA;
		ratio= 8;
		//CRC16_in(1,0:7)=[0 1 0 1 0 0 0 0]
		crc16_in[1] = crc16_in[3] = 1;
		break;
	case 1:
		pPMacTxInfo->SignalField=0x14;
		ratio=4;
		//CRC16_in(1,0:7)=[0 0 1 0 1 0 0 0];
		crc16_in[2] = crc16_in[4] = 1;
		break;
	case 2:
		pPMacTxInfo->SignalField=0x37;
		ratio=8.0/5.5;
		//CRC16_in(1,0:7)=[1 1 1 0 1 1 0 0];
		crc16_in[0] = crc16_in[1] = crc16_in[2] = crc16_in[4] = crc16_in[5] =1;
		break;
	case 3:
		pPMacTxInfo->SignalField =0x6E;
		ratio=8.0/11.0;
		//CRC16_in(1,0:7)=[0 1 1 1 0 1 1 0];
		crc16_in[1] = crc16_in[2] = crc16_in[3] = crc16_in[5] = crc16_in[6] =1;
		break;
	}
	BOOL LengthExtBit;
	double LengthExact = pPMacTxInfo->PacketLength*ratio;
	double LengthPSDU = ceil(LengthExact);
	if (	(pPMacPktInfo->MCS == 3) &&
		((LengthPSDU-LengthExact)>=0.727 || (LengthPSDU-LengthExact)<=-0.727))
		LengthExtBit=1;
	else
		LengthExtBit=0;
	pPMacTxInfo->LENGTH = (UINT)LengthPSDU;
	// CRC16_in(1,16:31) = LengthPSDU[0:15]
	for	(UCHAR i = 0; i<16; i++)
		crc16_in[i+16]	= (pPMacTxInfo->LENGTH >>i)& 0x1;
	if (LengthExtBit == 0) {
		pPMacTxInfo->ServiceField=0x0;
		//CRC16_in(1,8:15)=[0 0 0 0 0 0 0 0];
	} else {
		pPMacTxInfo->ServiceField = 0x80;
		//CRC16_in(1,8:15)=[0 0 0 0 0 0 0 1];
		crc16_in[15] = 1;
	}
	CRC16_generator(crc16_out,crc16_in,32);
	RTW_MEMSET(pPMacTxInfo->CRC16, 0, 2);
	ByteToBit(pPMacTxInfo->CRC16, crc16_out, 2);
}

//========================================
//	L-SIG	Rate	R	Length	P	Tail
//			4b		1b	12b		1b	6b
//========================================
void L_SIG_generator(
	UINT	N_SYM,		// Max: 750
	PRT_PMAC_TX_INFO	pPMacTxInfo,
	PRT_PMAC_PKT_INFO	pPMacPktInfo)
{
	BOOL	sig_bi[24]= {0};	// 24 BIT
	UINT	mode,LENGTH;
	if (IS_OFDM_RATE(pPMacTxInfo->TX_RATE)) {
		mode = pPMacPktInfo->MCS;
		LENGTH = pPMacTxInfo->PacketLength;
	} else {
		UCHAR	N_LTF;
		double	T_data;
		UINT	OFDM_symbol;
		mode	= 0;
		//	Table 20-13 Num of HT-DLTFs request
		if (pPMacPktInfo->Nsts <= 2)
			N_LTF = pPMacPktInfo->Nsts;
		else
			N_LTF = 4;
		if (pPMacTxInfo->bSGI)
			T_data=3.6;
		else
			T_data=4.0;
		//	(L-SIG, HT-SIG, HT-STF, HT-LTF....HT-LTF, Data)
		if (IS_VHT_RATE(pPMacTxInfo->TX_RATE))
			OFDM_symbol=(UINT)ceil((double)(8+4+N_LTF*4+N_SYM*T_data+4)/4.);
		else
			OFDM_symbol=(UINT)ceil((double)(8+4+N_LTF*4+N_SYM*T_data)/4.);
		//printf(" OFDM_symbol =%d\n", OFDM_symbol );
		LENGTH=OFDM_symbol*3-3;
		//printf(" LENGTH =%d\n", LENGTH );
	}
	//	Rate Field
	switch	(mode)	{
	case	0:	//B
		sig_bi[0]=1;
		sig_bi[1]=1;
		sig_bi[2]=0;
		sig_bi[3]=1;
		break;
	case	1:	//F
		sig_bi[0]=1;
		sig_bi[1]=1;
		sig_bi[2]=1;
		sig_bi[3]=1;
		break;
	case	2:	//A
		sig_bi[0]=0;
		sig_bi[1]=1;
		sig_bi[2]=0;
		sig_bi[3]=1;
		break;
	case	3:	//E
		sig_bi[0]=0;
		sig_bi[1]=1;
		sig_bi[2]=1;
		sig_bi[3]=1;
		break;
	case	4:	//9
		sig_bi[0]=1;
		sig_bi[1]=0;
		sig_bi[2]=0;
		sig_bi[3]=1;
		break;
	case	5:	//D
		sig_bi[0]=1;
		sig_bi[1]=0;
		sig_bi[2]=1;
		sig_bi[3]=1;
		break;
	case	6:	//8
		sig_bi[0]=0;
		sig_bi[1]=0;
		sig_bi[2]=0;
		sig_bi[3]=1;
		break;
	case	7:	//C
		sig_bi[0]=0;
		sig_bi[1]=0;
		sig_bi[2]=1;
		sig_bi[3]=1;
		break;
	}
	//	Reserved bit
	sig_bi[4]	= 0;
	//	Length Field
	for	(int i=0; i<12; i++) {
		sig_bi[i+5]=(LENGTH>>i) & 1;
	}
	//	Parity Bit
	sig_bi[17]=0;
	for	(int i=0; i<17; i++)
		sig_bi[17]+=sig_bi[i];
	sig_bi[17]%=2;
	//	Tail Field
	for	(int i=18; i<24; i++)
		sig_bi[i]=0;
	//dump_buf((char*)sig_bi,24);
	RTW_MEMSET(pPMacTxInfo->LSIG, 0, 3);
	ByteToBit(pPMacTxInfo->LSIG, sig_bi, 3);
}

//================================================================================
//	HT-SIG1	MCS	CW	Length		24BIT + 24BIT
//			7b	1b	16b
//	HT-SIG2	Smoothing	Not sounding	Rsvd		AGG	STBC	FEC	SGI	N_ELTF	CRC	Tail
//			1b			1b			1b		1b	2b		1b	1b	2b		8b	6b
//================================================================================
void HT_SIG_generator(
	PRT_PMAC_TX_INFO	pPMacTxInfo,
	PRT_PMAC_PKT_INFO	pPMacPktInfo)
{
	UINT i = 0;
	BOOL sig_bi[48] = {0}, crc8[8] = {0};
	//	MCS Field
	for	(i=0; i<7; i++)
		sig_bi[i] = (pPMacPktInfo->MCS >> i) & 0x1;
	//	Packet BW Setting
	sig_bi[7] = pPMacTxInfo->BandWidth;
	//	HT-Length Field
	for	(i=0; i<16; i++)
		sig_bi[i+8] = (pPMacTxInfo->PacketLength >> i) & 0x1;
	//	Smoothing;	1->allow smoothing
	sig_bi[24] = 1;
	//	Not Sounding
	sig_bi[25] = 1-pPMacTxInfo->NDP_sound;
	//	Reserved bit
	sig_bi[26] = 1;
	//	Aggregate
	sig_bi[27] = 0;
	//	STBC Field
	if (pPMacTxInfo->bSTBC) {
		sig_bi[28]	= 1;
		sig_bi[29]	= 0;
	} else {
		sig_bi[28]	= 0;
		sig_bi[29]	= 0;
	}
	//	Advance Coding,	0: BCC, 1: LDPC
	sig_bi[30] = pPMacTxInfo->bLDPC;
	//	Short GI
	sig_bi[31] = pPMacTxInfo->bSGI;
	//	N_ELTFs
	if (pPMacTxInfo->NDP_sound==FALSE) {
		sig_bi[32]	= 0;
		sig_bi[33]	= 0;
	} else {
		int	N_ELTF = pPMacTxInfo->Ntx - pPMacPktInfo->Nss;
		for	(i=0; i<2; i++)
			sig_bi[32+i] = (N_ELTF>>i)%2;
	}
	//	CRC-8
	CRC8_generator(crc8,sig_bi,34);
	for (i=0; i<8; i++)
		sig_bi[34+i] = crc8[i];
	//	Tail
	for	(i=42; i<48; i++)
		sig_bi[i]	= 0;
	RTW_MEMSET(pPMacTxInfo->HT_SIG, 0, 6);
	ByteToBit(pPMacTxInfo->HT_SIG, sig_bi, 6);
}

//======================================================================================
//	VHT-SIG-A1
//	BW	Reserved	STBC	G_ID	SU_Nsts	P_AID	TXOP_PS_NOT_ALLOW	Reserved
//	2b	1b			1b		6b	3b	9b		1b		2b					1b
//	VHT-SIG-A2
//	SGI	SGI_Nsym	SU/MU coding	LDPC_Extra	SU_NCS	Beamformed	Reserved	CRC	Tail
//	1b	1b			1b				1b			4b		1b			1b			8b	6b
//======================================================================================
void VHT_SIG_A_generator(
	PRT_PMAC_TX_INFO	pPMacTxInfo,
	PRT_PMAC_PKT_INFO	pPMacPktInfo)
{
	UINT i =0;
	BOOL sig_bi[48], crc8[8];
	RTW_MEMSET(sig_bi, 0, 48);
	RTW_MEMSET(crc8, 0, 8);
	//	BW Setting
	for	(i=0; i<2; i++)
		sig_bi[i]	= (pPMacTxInfo->BandWidth>>i)& 0x1;
	//	Reserved Bit
	sig_bi[2]		= 1;
	//	STBC Field
	sig_bi[3]		= pPMacTxInfo->bSTBC;
	//	Group ID:	Single User -> A value of 0 or 63 indicates an SU PPDU.
	for	(i=0; i<6; i++)
		sig_bi[4+i]	= 0;
	//	N_STS/Partial AID
	for	(i=0; i<12; i++) {
		if	(i<3)
			sig_bi[10+i]	= ((pPMacPktInfo->Nsts - 1)>>i) & 0x1;
		else
			sig_bi[10+i]	= 0;
	}
	//	TXOP_PS_NOT_ALLPWED
	sig_bi[22]	= 0;
	//	Reserved Bits
	sig_bi[23]	= 1;
	//	Short GI
	sig_bi[24]	= pPMacTxInfo->bSGI;
	if	(pPMacTxInfo->bSGI > 0 && (pPMacPktInfo->N_sym%10)==9)
		sig_bi[25]	= 1;
	else
		sig_bi[25]	= 0;
	//	SU/MU[0] Coding
	sig_bi[26]	= pPMacTxInfo->bLDPC;		//	0:BCC, 1:LDPC
	sig_bi[27]	= pPMacPktInfo->SIGA2B3;	//	Record Extra OFDM Symols is added or not when LDPC is used
	//	SU MCS/MU[1-3] Coding
	for	(i=0; i<4; i++)
		sig_bi[28+i]	= (pPMacPktInfo->MCS>>i) & 0x1;
	//	SU Beamform
	sig_bi[32]			= 0;	//packet.TXBF_en;
	//	Reserved Bit
	sig_bi[33]			= 1;
	//	CRC-8
	CRC8_generator(crc8,sig_bi,34);
	for	(i=0; i<8; i++)
		sig_bi[34+i]	= crc8[i];
	//	Tail
	for	(i=42; i<48; i++)
		sig_bi[i]		= 0;
	RTW_MEMSET(pPMacTxInfo->VHT_SIG_A, 0, 6);
	ByteToBit(pPMacTxInfo->VHT_SIG_A, sig_bi, 6);
}

//======================================================================================
//	VHT-SIG-B
//	Length				Resesrved	Trail
//	17/19/21 BIT		3/2/2 BIT	6b
//======================================================================================
void VHT_SIG_B_generator(PRT_PMAC_TX_INFO	pPMacTxInfo)
{
	BOOL sig_bi[32], crc8_bi[8];
	UINT i = 0, len = 0, res = 0, tail=6, crc8_in_len = 0;
	//UINT total_len = 0;
	RTW_MEMSET(sig_bi, 0, 32);
	RTW_MEMSET(crc8_bi, 0, 8);
	//	Sounding Packet
	if (pPMacTxInfo->NDP_sound==1) {
		if (pPMacTxInfo->BandWidth== 0) {
			BOOL sigb_temp[26]= {0,0,0,0,0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0};
			RTW_MEMCPY(sig_bi, sigb_temp, 26);
		} else if	(pPMacTxInfo->BandWidth== 1) {
			BOOL sigb_temp[27]= {1,0,1,0,0,1,0,1,1,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0};
			RTW_MEMCPY(sig_bi, sigb_temp, 27);
		} else if	(pPMacTxInfo->BandWidth== 2) {
			BOOL sigb_temp[29]= {0,1,0,1,0,0,1,1,0,0,1,0,1,1,1,1,1,1,1,0,0,1,0,0,0,0,0,0,0};
			RTW_MEMCPY(sig_bi, sigb_temp, 29);
		}
	} else { // Not NDP Sounding
		if (pPMacTxInfo->BandWidth== 0) {
			len	= 17;
			res	= 3;
		} else if	(pPMacTxInfo->BandWidth== 1) {
			len	= 19;
			res	= 2;
		} else if	(pPMacTxInfo->BandWidth== 2) {
			len	= 21;
			res	= 2;
		} else {
			len	= 21;
			res	= 2;
		}
		//total_len		= len+res+tail;
		crc8_in_len		= len+res;
		// Length Field
		UINT sigb_len=(pPMacTxInfo->PacketLength +3) >> 2;
		for	(i=0; i<len; i++)
			sig_bi[i]=(sigb_len>>i) & 0x1;
		// Reserved Field
		for	(i=0; i<res; i++)
			sig_bi[len+i]=1;
		// CRC-8
		CRC8_generator(crc8_bi,sig_bi,crc8_in_len);
		// Tail
		for	(i=0; i<tail; i++)
			sig_bi[len+res+i]=0;
	} // if	(NDP_sound==1)
	RTW_MEMSET(pPMacTxInfo->VHT_SIG_B, 0, 4);
	ByteToBit(pPMacTxInfo->VHT_SIG_B, sig_bi, 4);
	pPMacTxInfo->VHT_SIG_B_CRC = 0;
	ByteToBit(&(pPMacTxInfo->VHT_SIG_B_CRC), crc8_bi, 1);
}


//=======================
// VHT Delimiter
//=======================
void VHT_Delimiter_generator(PRT_PMAC_TX_INFO	pPMacTxInfo)
{
	BOOL sig_bi[32] = {0}, crc8[8] = {0};
	UINT crc8_in_len = 16;
	UINT PacketLength = pPMacTxInfo->PacketLength;
	// Delimiter[0]: EOF
	sig_bi[0]	= 1;
	// Delimiter[1]: Reserved
	sig_bi[1]	= 0;
	// Delimiter[3:2]: MPDU Length High
	sig_bi[2]	= ((PacketLength -4) >> 12) % 2;
	sig_bi[3]	= ((PacketLength -4)>> 13) % 2;
	// Delimiter[15:4]: MPDU Length Low
	for (int j=4; j<16; j++)
		sig_bi[j]	= ((PacketLength -4) >> (j-4)) % 2;
	CRC8_generator(crc8, sig_bi, crc8_in_len);
	for (int j=16; j<24; j++) // Delimiter[23:16]: CRC 8
		sig_bi[j]	= crc8[j-16];
	for (int j=24; j<32; j++) // Delimiter[31:24]: Signature ('4E' in Hex, 78 in Dec)
		sig_bi[j]	= (78 >> (j-24)) % 2;
	RTW_MEMSET(pPMacTxInfo->VHT_Delimiter, 0, 4);
	ByteToBit(pPMacTxInfo->VHT_Delimiter, sig_bi, 4);
}

UINT LDPC_parameter_generator(
	UINT N_pld_int,				// number of initial information bits
	UINT N_CBPSS,				// number of coded bits per OFDM symbol per stream
	UINT N_SS,					// number of spatial-stream
	UINT R,						// coding rate
	UINT m_STBC,				// STBC indicator
	UINT N_TCB_int				// number of total coded bits
)
{
	double	CR = 0.;
	double	N_pld = static_cast<double>(N_pld_int);
	double	N_TCB = static_cast<double>(N_TCB_int);
	double	N_CW = 0.;//, N_spcw=0., N_fshrt=0.;
	double	L_LDPC = 0., K_LDPC= 0., N_shrt=0., N_punc=0.;//, L_LDPC_info=0.;
	UINT	VHTSIGA2B3  = 0;		// extra symbol from VHT-SIG-A2 Bit 3
	if (R==0)	CR	= 0.5;
	else if (R==1)	CR	= 2./3.;
	else if (R==2)	CR	= 3./4.;
	else if (R==3)	CR	= 5./6.;
	if (N_TCB <= 648.) {
		N_CW	= 1.;
		if (N_TCB >= N_pld+912.*(1.-CR))
			L_LDPC	= 1296.;
		else
			L_LDPC	= 648.;
	} else if	(N_TCB <= 1296.) {
		N_CW	= 1.;
		if (N_TCB >= static_cast<double>(N_pld)+1464.*(1.-CR))
			L_LDPC	= 1944.;
		else
			L_LDPC	= 1296.;
	} else if	(N_TCB <= 1944.) {
		N_CW	= 1.;
		L_LDPC	= 1944.;
	} else if (N_TCB <= 2592.) {
		N_CW	= 2.;
		if (N_TCB >= N_pld+2916.*(1.-CR))
			L_LDPC	= 1944.;
		else
			L_LDPC	= 1296.;
	} else {
		N_CW=ceil(N_pld/1944./CR);
		L_LDPC	= 1944.;
	}
	//	Number of information bits per CW
	K_LDPC	= L_LDPC*CR;
	//	Number of shortening bits					max(0, (N_CW * L_LDPC * R) - N_pld)
	N_shrt	= (N_CW*K_LDPC-N_pld)>0. ? (N_CW*K_LDPC-N_pld) : 0.;
	//	Number of shortening bits per CW			N_spcw = floor(N_shrt/N_CW)
	//N_spcw	= floor(N_shrt/N_CW);
	//	The first N_fshrt CWs shorten 1 bit more
	//N_fshrt	= static_cast<double>(static_cast<int>(N_shrt) % static_cast<int>(N_CW));
	//	Number of data bits for the last N_CW-N_fshrt CWs
	//L_LDPC_info	= K_LDPC-N_spcw;
	//	Number of puncturing bits
	N_punc=(N_CW*L_LDPC-N_TCB-N_shrt)>0. ? (N_CW*L_LDPC-N_TCB-N_shrt) : 0.;
	if	(  ( (N_punc>.1*N_CW*L_LDPC*(1.-CR)) && (N_shrt<1.2*N_punc*CR/(1.-CR)) ) ||
		   (N_punc>0.3*N_CW*L_LDPC*(1.-CR))	) {
		//cout << "*** N_TCB and N_punc are Recomputed ***" << endl;
		VHTSIGA2B3 = 1;
		N_TCB		+= static_cast<double>(N_CBPSS*N_SS*m_STBC);
		N_punc		= (N_CW*L_LDPC-N_TCB-N_shrt)>0. ? (N_CW*L_LDPC-N_TCB-N_shrt) : 0.;
	} else
		VHTSIGA2B3 = 0;
	return VHTSIGA2B3;
}	// function end of LDPC_parameter_generator


// Add codes here.


//========================================
//	Data field of PPDU
//	Get N_sym and SIGA2BB3
//========================================
void PMAC_Nsym_generator(
	PRT_PMAC_TX_INFO	pPMacTxInfo,
	PRT_PMAC_PKT_INFO	pPMacPktInfo)
{
	UINT	SIGA2B3 = 0;
	UCHAR	TX_RATE = pPMacTxInfo->TX_RATE;
	UINT R = 0,R_list[10]= {0,0,2,0,2,1,2,3,2,3};
	double CR = 0.;
	UINT N_SD = 0, N_BPSC_list[10]= {1,2,2,4,4,6,6,6,8,8};
	UINT N_BPSC = 0,N_CBPS = 0,N_DBPS = 0,N_ES = 0, N_SYM = 0,N_pld = 0,N_TCB = 0;
	//	N_SD
	if ( pPMacTxInfo->BandWidth == 0)
		N_SD=52;
	else if ( pPMacTxInfo->BandWidth == 1)
		N_SD=108;
	else
		N_SD=234;
	if (IS_HT_RATE(TX_RATE)) {
		UCHAR MCS_temp;
		if (pPMacPktInfo->MCS > 23)
			MCS_temp=pPMacPktInfo->MCS-24;
		else if (pPMacPktInfo->MCS > 15)
			MCS_temp=pPMacPktInfo->MCS-16;
		else if (pPMacPktInfo->MCS > 7)
			MCS_temp=pPMacPktInfo->MCS-8;
		else
			MCS_temp=pPMacPktInfo->MCS;
		R=R_list[MCS_temp];
		switch (R) {
		case 0:
			CR= .5;
			break;
		case 1:
			CR=2./3.;
			break;
		case 2:
			CR=3./4.;
			break;
		case 3:
			CR=5./6.;
			break;
		}
		N_BPSC=N_BPSC_list[MCS_temp];
		N_CBPS=N_BPSC*N_SD*pPMacPktInfo->Nss;
		N_DBPS=(UINT)((double)N_CBPS*CR);
		if (pPMacTxInfo->bLDPC == FALSE) {
			N_ES=(UINT)ceil((double)(N_DBPS* pPMacPktInfo->Nss)/4./300.);
			//	N_SYM = m_STBC* (8*length+16+6*N_ES) / (m_STBC*N_DBPS)
			N_SYM = pPMacTxInfo->m_STBC*(UINT)ceil((double)(pPMacTxInfo->PacketLength*8+16+N_ES*6)/
							       (double)(N_DBPS*pPMacTxInfo->m_STBC));
		} else {
			N_ES = 1;
			//	N_pld = length * 8 + 16
			N_pld = pPMacTxInfo->PacketLength*8+16;
			N_SYM = pPMacTxInfo->m_STBC*(UINT)ceil((double)(N_pld)/
							       (double)(N_DBPS*pPMacTxInfo->m_STBC));
			//	N_avbits = N_CBPS *m_STBC *(N_pld/N_CBPS*R*m_STBC)
			N_TCB = N_CBPS*N_SYM;
			SIGA2B3 = LDPC_parameter_generator(N_pld, N_CBPS, pPMacPktInfo->Nss, R, pPMacTxInfo->m_STBC, N_TCB);
			N_SYM = N_SYM + SIGA2B3*pPMacTxInfo->m_STBC;
		}
	} else if (IS_VHT_RATE(TX_RATE)) {
		R=R_list[pPMacPktInfo->MCS];
		switch (R) {
		case 0:
			CR=.5;
			break;
		case 1:
			CR=2./3.;
			break;
		case 2:
			CR=3./4.;
			break;
		case 3:
			CR=5./6.;
			break;
		}
		N_BPSC=N_BPSC_list[pPMacPktInfo->MCS];
		N_CBPS=N_BPSC*N_SD*pPMacPktInfo->Nss;
		N_DBPS=(UINT)((double)N_CBPS*CR);
		if (pPMacTxInfo->bLDPC == FALSE) {
			if (pPMacTxInfo->bSGI)
				N_ES=(UINT)ceil((double)(N_DBPS)/3.6/600.);
			else
				N_ES=(UINT)ceil((double)(N_DBPS)/4./600.);
			//	N_SYM = m_STBC* (8*length+16+6*N_ES) / (m_STBC*N_DBPS)
			N_SYM = pPMacTxInfo->m_STBC*(UINT)ceil((double)(pPMacTxInfo->PacketLength*8+16+N_ES*6)/(double)(N_DBPS*pPMacTxInfo->m_STBC));
			SIGA2B3=0;
		} else {
			N_ES=1;
			//	N_SYM = m_STBC* (8*length+N_service) / (m_STBC*N_DBPS)
			N_SYM = pPMacTxInfo->m_STBC*(UINT)ceil((double)(pPMacTxInfo->PacketLength*8+16)/(double)(N_DBPS*pPMacTxInfo->m_STBC));
			//	N_avbits = N_sys_init * N_CBPS
			N_TCB= N_CBPS * N_SYM;
			//	N_pld = N_sys_init * N_DBPS
			N_pld=N_SYM * N_DBPS;
			SIGA2B3 = LDPC_parameter_generator(N_pld, N_CBPS, pPMacPktInfo->Nss, R, pPMacTxInfo->m_STBC, N_TCB);
			N_SYM = N_SYM + SIGA2B3*pPMacTxInfo->m_STBC;
		}
		int D_R;
		switch (R) {
		case 0:
			D_R=2;
			break;
		case 1:
			D_R=3;
			break;
		case 2:
			D_R=4;
			break;
		case 3:
			D_R=6;
			break;
		}
		if (((N_CBPS/N_ES)%D_R)!=0) {
			//cout << "MCS=" << pPMacTxInfo->MCS << " is not supported when Nss=" << pPMacTxInfo->Nss << " and BW=" << pPMacTxInfo->BandWidth << "!!\n";
			return;
		}
	}
	pPMacPktInfo->N_sym = N_SYM;
	pPMacPktInfo->SIGA2B3 = SIGA2B3;
}


void PMAC_Get_Pkt_Param(
	PRT_PMAC_TX_INFO	pPMacTxInfo,
	PRT_PMAC_PKT_INFO	pPMacPktInfo)
{
	UCHAR		TX_RATE_HEX = 0, MCS = 0;
	UCHAR		TX_RATE = pPMacTxInfo->TX_RATE;
	//	TX_RATE & Nss
	if (IS_2SS_RATE(TX_RATE))
		pPMacPktInfo->Nss = 2;
	else if (IS_3SS_RATE(TX_RATE))
		pPMacPktInfo->Nss = 3;
	else if (IS_4SS_RATE(TX_RATE))
		pPMacPktInfo->Nss = 4;
	else
		pPMacPktInfo->Nss = 1;
	DBG("PMacTxInfo.Nss =%d\n", pPMacPktInfo->Nss);
	//	MCS & TX_RATE_HEX
	if (IS_CCK_RATE(TX_RATE)) {
		switch (TX_RATE) {
		case RATE_CCK_1M:
			TX_RATE_HEX = MCS = 0;
			break;
		case RATE_CCK_2M:
			TX_RATE_HEX = MCS = 1;
			break;
		case RATE_CCK_55M:
			TX_RATE_HEX = MCS = 2;
			break;
		case RATE_CCK_11M:
			TX_RATE_HEX = MCS = 3;
			break;
		}
	} else if (IS_OFDM_RATE(TX_RATE)) {
		MCS = TX_RATE - RATE_OFDM_6M;
		TX_RATE_HEX = MCS + 4;
	} else if (IS_HT_RATE(TX_RATE)) {
		MCS = TX_RATE - RATE_MCS0;
		TX_RATE_HEX = MCS + 12;
	} else if (IS_VHT_RATE(TX_RATE)) {
		TX_RATE_HEX = TX_RATE - RATE_VHT1SS_MCS0 + 44;
		if (IS_VHT_2S_RATE(TX_RATE))
			MCS = TX_RATE - RATE_VHT2SS_MCS0;
		else if (IS_VHT_3S_RATE(TX_RATE))
			MCS = TX_RATE - RATE_VHT3SS_MCS0;
		else if (IS_VHT_4S_RATE(TX_RATE))
			MCS = TX_RATE - RATE_VHT4SS_MCS0;
		else
			MCS = TX_RATE - RATE_VHT1SS_MCS0;
	}
	pPMacPktInfo->MCS = MCS;
	pPMacTxInfo->TX_RATE_HEX = TX_RATE_HEX;
	DBG(" MCS=%d ,TX_RATE_HEX =0x%x\n", MCS, pPMacTxInfo->TX_RATE_HEX);
	//	mSTBC & Nsts
	pPMacPktInfo->Nsts = pPMacPktInfo->Nss;
	if (pPMacTxInfo->bSTBC) {
		if (pPMacPktInfo->Nss == 1) {
			pPMacTxInfo->m_STBC = 2;
			pPMacPktInfo->Nsts = pPMacPktInfo->Nss*2;
		} else
			pPMacTxInfo->m_STBC = 1;
	} else
		pPMacTxInfo->m_STBC = 1;
}

void PMAC_Enter(PRT_PMAC_TX_INFO	pPMacTxInfo)
{
	RT_PMAC_PKT_INFO	PMacPktInfo;
	PMAC_Get_Pkt_Param(pPMacTxInfo, &PMacPktInfo);
	if (IS_CCK_RATE(pPMacTxInfo->TX_RATE)) {
		CCK_generator(pPMacTxInfo, &PMacPktInfo);
	} else {
		PMAC_Nsym_generator(pPMacTxInfo, &PMacPktInfo);
		// 24 BIT
		L_SIG_generator(PMacPktInfo.N_sym, pPMacTxInfo, &PMacPktInfo);
	}
	//	48BIT
	if (IS_HT_RATE(pPMacTxInfo->TX_RATE))
		HT_SIG_generator(pPMacTxInfo, &PMacPktInfo);
	else if (IS_VHT_RATE(pPMacTxInfo->TX_RATE)) {
		//	48BIT
		VHT_SIG_A_generator(pPMacTxInfo, &PMacPktInfo);
		//	26/27/29 BIT  & CRC 8 BIT
		VHT_SIG_B_generator(pPMacTxInfo);
		// 32 BIT
		VHT_Delimiter_generator(pPMacTxInfo);
	}
}



void PMAC_Notify(PRT_PMAC_TX_INFO pPMacTxInfo)
{
	if (pPMacTxInfo->bEnPMacTx)
		PMAC_Enter(pPMacTxInfo);
}


int psd_analysis(char *ifname, char *psdcmd, int psdlen)
{
	int skfd;
	char *p;
	const char *f = " ";
	int psd_hex[1024];
	int V_2[1024];
	int i, j, err = 0;
	int iter_var = 3;
	float min_db; /* Minimum_Calulation_db */
	float max_db; /* Maximum_except_Interference_db */
	float min_lin; /* Minimum_Calulation_Lin */
	float max_lin; /* Maximum_except_Interference_Lin */
	int total_power = 0; /* Total_Power_including_Interference */
	int noise_floor = 0; /* System_Noise_Floor */
	int cnt_total_power;
	int cnt_noise_floor;
	float total_power_db;
	float noise_floor_db;
	//char psd_data[]="1 1 1 1 1 1 2 2 3 1 1 0 1 1 0 0 1 1 2 1 0 1 0 0 1 1 1 1 1 0 1 1 1 1 1 0 1 1 0 1 1 1 0 1 0 1 1 1 1 1 0 1 1 1 1 1 0 0 1 1 0 1 1 1 1 3 0 1 1 1 1 2 2 1 0 3 3 3 1 1 1 1 1 2 1 3 1 3 2 3 4 2 3 1 1 1 1 1 1 2 1 1 1 0 3 1 1 1 1 1 3 3 1 1 1 1 0 1 1 1 0 0 1 1 0 1 1 1 1 0 1 1 0 1 0 1 1 1 0 0 1 1 0 0 0 1 1 1 3 1 1 1 1 1 1 1 1 1 0 1 0 0 1 0 0 1 1 1 1 1 1 1 0 1 1 0 0 1 1 1 1 1 1 0 0 1 1 1 1 1 1 1 1 b 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 0 1 1 0 0 1 1 1 1 1 1 9 3 7 d 2 3 1 1 1 1 0 1 1 4 1 1 0 1 1 0 1 1 1 0 1 1 1 1 1 1 1 1 0 0 1 1 1 1 1 1 1 0 1 0 1 0 1 0 0 0 0 1 5 1 1 0 1 1 1 1 1 1 1 4 2 3 1 1 0 1 1 1 0 1 0 1 1 6 1 1 0 1 1 1 1 0 1 0 1 1 1 1 1 1 1 1 1 4 0 1 0 1 1 1 1 1 1 1 1 1 1 0 1 0 6 0 1 1 1 1 1 1 1 1 1 9 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 0 9 1 2 1 0 3 1 1 1 4 4 1 1 4 3 2 1 6 4 5 2 2 2 1 5 1 4 2 1 1 4 3 6 2 1 5 4 2 1 1 4 5 3 5 6 6 3 39 5 8 3 5 a b 6 9 c 3 56 52 a 6 5 25 4 3 f 6 10 3 2 3 5 6 1b 4 6 22 b 3 3 19 26 25 38 21 1f 5 22 d f 2c bc 2b 35 23 5 6 6 e 9 f e 17 4 1 1c 1 5 18 1f 12 7 13 20 2 31 d 11 2 1c b 13 1df 15 a 31 1b a2 a7 77 96 c0 15c 135 1a 1f 1fb 10 13e 1d6 67 24 113 a c 1c 8 e f 21 24 a a 14 28 57 c 7 9 4 1e 4 19 9 10 229 10 1a 10 13 292 10 137 133 d2 7 1 e 61 76 1c f6 66 10 d f fc f 7 8 c 12 18 3d 6 1 9 1 8 d 9 74 13 a a 10 c 3 c 4 7 15 7 8 10 7 11 1a 70 148 13 6 158 4 1a d 5b 1d5 18 6 79 21 4 3c 7 c 8 5 3 6 4 5 c a 4 1 1 3 2 6 2 1 4 1 9 4 3 1 1 0 1 a 1 1 2 1 3 1 1 1 1 1 1 1 1 215 1 1 284 283 1fe 1 0 1 3 16 1 0 1 1 8 4 3 1 2 1 1 1 53 0 1 1 1 5 2 1 1 0 1 1 1 1 1 4 1 1 1 1 1 0 1 124 1 1 8 3 1 1 1 11 3 1 1 1 1 1 1 1 1 4 1 1 1 1b 18 1 12 1 55 15 0 9 c c 1 e 11 1 b a b 0 1 1 1 1 0 3e 1 0 0 7 6 44 11 1 1 0 1 1 1 0 0 1 1 0 0 1 1 0 0 0 1 2 3 1 1 0 1 0 1 1 1 1 1 0 1 1 8 1 1 1 1 b 1 1 1 4 1 1 1 0 1 1 1 1 1 0 0 2 2 2 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 1 1 0 1 1 1 1 1 1 2 9 1 1 0 1 2 1 1 0 2 1 1 0 2 1 0 0 0 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 2 1 1 0 1 0 1 1 1 3 1 0 1 1 1 0 1 1 0 0 1 0 1 1 1 1 1 0 1 1 1 1 0 1 1 1 1 1 1 0 1 1 0 1 1 1 1 1 0 2 1 1 1 1 0 1 1 1 0 1 1 1 0 1 1 1 1 1 1 1 1 0 1 0 1 1 1 1 1 1 1 0 0 1 1 1 1 0 1 0 0 1 1 1 1 0 1 1 2 1 0 1 1 1 1 1 1 0 1 1 3 2 3 1 1 0 1 1 0 0 1 0 1 0 0 0 1 1 1 0 1 1 0 0 1 1 0 2 2 1 1 1 1 1 1 1 1 1";
#if 1
	char input[1024 * 8];
	char psd_cmd_1024[]="mp_psd analysis,pts=1024,start=512,stop=1536";
	char psd_cmd_512[]="mp_psd analysis,pts=512,start=256,stop=768";
	char psd_cmd_256[]="mp_psd analysis,pts=256,start=128,stop=384";
	char psd_cmd_128[]="mp_psd analysis,pts=128,start=64,stop=192";
	/* get PSD Data */
	if (psdlen==1024)
		sprintf(input, psd_cmd_1024, strlen(psd_cmd_1024));
	else if (psdlen==512)
		sprintf(input, psd_cmd_512, strlen(psd_cmd_512));
	else if (psdlen==256)
		sprintf(input, psd_cmd_256, strlen(psd_cmd_256));
	else if (psdlen==128)
		sprintf(input, psd_cmd_128, strlen(psd_cmd_128));
	else {
		sprintf(input, psd_cmd_256, strlen(psd_cmd_256));
	}
	DBG("psd_len = %d\n", psdlen);
	DBG("psd_cmd input = %s\n", input);
	skfd = iw_sockets_open();
	err = wlan_ioctl_mp(skfd, ifname, input, strlen(input) + 1);
	iw_sockets_close(skfd);
	if (err < 0) {
		RTW_FPRINT(stderr, "Interface doesn't accept private ioctl...\n");
		RTW_FPRINT(stderr, "%s: %s\n", psdcmd, rtw_strerror(errno));
		return -1;
	}
	if (strlen(input) == 0) {
		printf("Error: no PSD result, please check commands again\n");
		return -1;
	}
	DBG("\n\n\n");
	DBG("%-8.16s %s:%s\n", ifname, psdcmd, input);
#else
	p = strtok(psd_data, d);
	printf("%s..strtok\n", p);
	printf("min=%d, max=%d\n", min_lin, max_lin);
#endif
	i = 0;
	p = strtok(input, f);
	while (p) {
		psd_hex[i]= (unsigned int) strtol (p, NULL, 16);
		V_2[i] = pow(psd_hex[i], 2);
		DBG("%d ", V_2[i]);
		i++;
		p = strtok(NULL, f);
	}
	DBG("\n\n\n");
	min_db = 20;
	max_db = 35;
	for (i=0; i<10; i++) {
		min_lin = pow(10, (min_db / 10));
		max_lin = pow(10, (max_db / 10));
		total_power = 0;
		noise_floor = 0;
		cnt_total_power = 0;
		cnt_noise_floor = 0;
		for (j=0; j < psdlen; j++) {
			if (V_2[j] > min_lin) {
				total_power += V_2[j];
				cnt_total_power++;
				if (V_2[j] < max_lin) {
					noise_floor += V_2[j];
					cnt_noise_floor++;
				}
			}
		}
		assert(cnt_total_power);
		assert(cnt_noise_floor);
		total_power_db = log10(total_power / cnt_total_power) * 10;
		noise_floor_db = log10(noise_floor / cnt_noise_floor) * 10;
		DBG("min_db =%f\n", min_db);
		DBG("max_db =%f\n", max_db);
		DBG("min_lin =%f\n", min_lin);
		DBG("max_lin =%f\n", max_lin);
		DBG("total_power_db=%f\n", total_power_db);
		DBG("noise_floor_db=%f\n", noise_floor_db);
		DBG("Sensitivity_Degradation =%f\n", total_power_db - noise_floor_db);
		min_db = noise_floor_db - iter_var;
		if (min_db <= 20)
			min_db = 20;
		max_db = noise_floor_db + iter_var;
		if (max_db >= 35)
			max_db = 35;
	}
	printf("System noise: %f dB\n", noise_floor_db);
	printf("Desensitization: %f dB\n", total_power_db - noise_floor_db);
	return 0;
}

int do_io_ctrl(char *ifname, char *pBufdata)
{
	int skfd, err = 0;
	skfd = iw_sockets_open();
	err = wlan_ioctl_mp(skfd, ifname, pBufdata, strlen(pBufdata) + 1);
	iw_sockets_close(skfd);
	if (err < 0) {
		RTW_FPRINT(stderr, "Interface doesn't accept private ioctl...\n");
		RTW_FPRINT(stderr, "%s\n", rtw_strerror(errno));
		return -1;
	}
	return err;
}


unsigned int crc32b(char *message)
{
	int i, j;
	unsigned int byte, crc, mask;
	i = 0;
	crc = 0xFFFFFFFF;
	while (message[i] != 0) {
		byte = message[i];
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {
			mask = -1 * (crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
		i = i + 1;
	}
	return ~crc;
}



int parse_payload(char *input, char *payload, int *dut_cmd, int *number)
{
	unsigned int i = 0, j = 0, crc32, crc32_temp = 0;
	int err = 0;
	char *p;
	const char *f = ":";
	char search[12] = "RealLoveTek";
	char search_after[12] = "TekLoveReal";
	char str_tmp[1024] = {0}, data[1024] = {0}, checksum[4] = {0};
	const char *space = " ";
	//printf("input=%s\n", input);
	p = strtok(input, f);
	while (p) {
		str_tmp[i] = strtol(p, NULL, 16);
		//printf("%c", str_tmp[i]);
		i++;
		p = strtok(NULL, f);
	}
	str_tmp[i] = '\0';
	for (i = 0; i < sizeof(str_tmp); i++) {
		if (!memcmp(&str_tmp[i], search, sizeof(search) - 1))
			break;
	}
	//printf("i = %d\n", i);
	if (i != sizeof(str_tmp)) {
		for (j = i + 1; j < sizeof(str_tmp); j++) {
			if (!memcmp(&str_tmp[j], search_after, sizeof(search_after) - 1))
				break;
		}
		//printf("j = %d\n", j);
		if (j == sizeof(str_tmp))
			return -1;
		RTW_MEMCPY(payload, (&str_tmp[i] + sizeof(search) - 1), (j - i - (sizeof(search) - 1)));
		//printf("payload = %s\n", payload);
		RTW_MEMCPY(data, &str_tmp[i], (j - i + (sizeof(search_after) - 1)));
		//printf("data = %s\n", data);
		crc32 = crc32b(data);
		//printf("crc32 = 0x%x\n", crc32);
		RTW_MEMCPY(checksum, (&str_tmp[j] + sizeof(search_after) - 1), 4);
		for (int k = 0; k < 4; k++)
			crc32_temp = crc32_temp + (((unsigned char)checksum[k]) << (24 - (8 * k)));
		if (crc32 != crc32_temp)
			return -1;
	} else
		return -1;
	i = 0;
	RTW_MEMCPY(str_tmp, payload, sizeof(str_tmp));
	p = strtok(str_tmp, space);
	while (p) {
		//printf("%s ", p);
		sscanf(p, "%d", &dut_cmd[i]);
		i++;
		p = strtok(NULL, space);
	}
	*number = i;
	//printf("dut_cmd number = %d\n", *number);
	return err;
}

int set_efuse(char *ifname, int *dut_cmd, int number)
{
	int err = 0;
	char input[1024 * 8];
	int i;
	if (dut_cmd[1] == 0) {
		/*Set efuse to driver fake */
		sprintf( input, "efuse_set wldumpfake");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
		/*Get driver fake */
		//sprintf( input, "efuse_get wlrfkmap");
		//printf("%s\n", input);
		//err = do_io_ctrl(ifname, input);
	}
	/*Set vaule to driver fake */
	if (dut_cmd[1] == 1) {
		for (i = 3; i < (number - 1); i++) {
			if (dut_cmd[i] > 63) {
				printf("dut_cmd[%d]=%d > 63\n", i, dut_cmd[i]);
				return err;
			}
		}
		sprintf(input, "efuse_set wlwfake,%2x,", dut_cmd[2]);
		for (i = 3; i < (number - 1); i++) {
			printf("dut_cmd[%d]=0x%x\n", i, dut_cmd[i]);
			if (dut_cmd[i] != 0)
				sprintf(input, "%s%x", input, dut_cmd[i]);
		}
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
	}
	/*Set driver fake to efues */
	if (dut_cmd[1] == 2) {
		sprintf( input, "efuse_set wlfk2map");
		printf("%s\n", input);
		//err = do_io_ctrl(ifname, input);
	}
	return err;
}


int get_dut_tx_idx(char *ifname, int *dut_cmd, int *tx_idx)
{
	int err = 0;
	char txpath[2], rxpath[2];
	char input[1024 * 8];
	char *p;
	char *q;
	const char *comma = ",";
	char str_tmp[100] = {0};
	int i;
	/*Set arx stop CMD */
	sprintf( input, "mp_arx stop");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set channel CMD */
	sprintf( input, "%s %d", "mp_channel", dut_cmd[1]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set BW CMD */
	sprintf( input, "mp_bandwidth 40M=%d,shortGI=0", dut_cmd[2]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx Ant CMD */
	if (dut_cmd[3] == 8)
		strcpy(txpath, "a");
	else if (dut_cmd[3] == 4)
		strcpy(txpath, "b");
	else if (dut_cmd[3] == 2)
		strcpy(txpath, "c");
	else if (dut_cmd[3] == 1)
		strcpy(txpath, "d");
	sprintf( input, "mp_ant_tx %s", txpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Rx Ant CMD */
	if (dut_cmd[4] == 8)
		strcpy(rxpath, "a");
	else if (dut_cmd[4] == 4)
		strcpy(rxpath, "b");
	else if (dut_cmd[4] == 2)
		strcpy(rxpath, "c");
	else if (dut_cmd[4] == 1)
		strcpy(rxpath, "d");
	sprintf( input, "mp_ant_rx %s", rxpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx Rate CMD */
	sprintf( input, "mp_rate %d", dut_cmd[5]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx idx CMD */
	if (dut_cmd[6] == 1) {
		sprintf( input, "mp_txpower patha=%d,pathb=%d", dut_cmd[7], dut_cmd[8]);
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
	} else {
		sprintf(input, "mp_get_txpower");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
		//printf("input:%s\n", input);
		p = strtok(input, comma);
		for (i = 0; p != NULL; i++) {
			sscanf(p, "%s", str_tmp);
			//printf("%s\n", str_tmp);
			q = strchr(str_tmp, '=');
			//printf("%s\n", q + 1);
			sscanf(q + 1, "%d", &tx_idx[i]);
			p = strtok(NULL, comma);
			if (i >= 4)
				break;
		}
		sprintf( input, "mp_txpower patha=%d,pathb=%d",
			 tx_idx[0], tx_idx[1]);
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
	}
	return err;
}


int set_dut_idle(char *ifname, int pre_ch, int pre_bw, int pre_ant)
{
	int err = 0;
	char input[1024 * 8];
	char txpath[2] = {0};
	/*Set channel CMD */
	sprintf( input, "mp_channel %d", pre_ch);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	sprintf( input, "mp_txpower patha=10,pathb=10");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set BW CMD */
	sprintf( input, "mp_bandwidth 40M=%d,shortGI=0", pre_bw);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx Ant CMD */
	if (pre_ant == 8)
		strcpy(txpath, "a");
	else if (pre_ant == 4)
		strcpy(txpath, "b");
	else if (pre_ant == 2)
		strcpy(txpath, "c");
	else if (pre_ant == 1)
		strcpy(txpath, "d");
	/*Set Tx Ant CMD */
	sprintf( input, "mp_ant_tx %s", txpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Rx Ant CMD */
	sprintf( input, "mp_ant_rx %s", txpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx Rate CMD */
	sprintf( input, "mp_rate 12");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set arx start CMD */
	sprintf( input, "mp_arx start");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	return err;
}

int secd_command(char *payload, char *send_cmd)
{
	int err = 0, i, cmd_ack_length;
	char header_str[12] = "RealLoveTek";
	char header_str_after[12] = "TekLoveReal";
	char cmd_tx[1024] = "mp_link txdata,";
	char cmd_tmp[1024] = {0};
	unsigned int crc32, cmd_ack[1024] = {0};
	sprintf(cmd_tmp, "%s%s%s", header_str, payload,header_str_after);
	//printf("cmd_ack=%s\n", cmd_tmp);
	crc32 = crc32b(cmd_tmp);
	for (i = 0; cmd_tmp[i] != 0; i++)
		cmd_ack[i] = (unsigned int)(cmd_tmp[i] & 0xff);
	cmd_ack[i] = (crc32 & 0xff000000) >> 24;
	cmd_ack[i + 1] = (crc32 & 0xff0000) >> 16;
	cmd_ack[i + 2] = (crc32 & 0xff00) >> 8;
	cmd_ack[i + 3] = (crc32 & 0xff);
	cmd_ack[i + 4] = '\0';
	cmd_ack_length = i + 4;
	//for (i = 0; cmd_ack[i] != 0; i++)
	//	printf("%x ", cmd_ack[i] );
	//printf("\n");
	sprintf(send_cmd, "%s", cmd_tx);
	for (i = 0; i < cmd_ack_length; i++) {
		if ((i * 2 + strlen(cmd_tx)) < (sizeof(char) * 1024))
			sprintf(&send_cmd[i * 2 + strlen(cmd_tx)], "%02x", cmd_ack[i]);
	}
	//printf("cmd ack=%s\n", send_cmd);
	return err;
}


int recv_command(char *ifname, char *payload, int *dut_cmd, int *number)
{
	char input[1024 * 8];
	char cmd_rx[]="mp_link rxdata";
	//char cmd_ack_or[]="mp_link txdata,5265616c4c6f766554656b61636b54656b4c6f76655265616c436111c8";
	int err = 0, i = 0, j, enable = 0;
	char send_cmd[1024] = {0};
	char cmd_ack[1024] = {0};
	//int tx_idx[4] = {0};
	/*Set Tx Ant CMD */
	sprintf( input, "mp_ant_tx a");
	//printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Rx Ant CMD */
	sprintf( input, "mp_ant_rx a");
	//printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	usleep(50000);
	sprintf(input, cmd_rx, strlen(cmd_rx));
	err = do_io_ctrl(ifname, input);
	if (input[0] != 0) {
		enable = 1;
		//printf("\ninput=%s\n", input);
	}
	while (enable) {
		/* Rx Data Parsing */
		if (parse_payload(input, payload, dut_cmd, number) != -1) {
			printf("\ni = %d   payload=%s\n", i, payload);
			if (dut_cmd[0] != 1 && dut_cmd[0] != 2 && dut_cmd[0] != 9 &&
			    dut_cmd[0] != 13 && dut_cmd[0] != 15)
				break;
			//get_dut_tx_idx(ifname, dut_cmd, tx_idx);
			//set_dut_idle(ifname);
			if (dut_cmd[0] == 1)
				sprintf(cmd_ack, "3ack%d", dut_cmd[*number - 1]);
			else if (dut_cmd[0] == 2)
				sprintf(cmd_ack, "4ack%d", dut_cmd[*number - 1]);
			else if (dut_cmd[0] == 9)
				sprintf(cmd_ack, "3ack%d", dut_cmd[*number - 1]);
			else if (dut_cmd[0] == 13)
				sprintf(cmd_ack, "14ack%d", dut_cmd[*number - 1]);
			else if (dut_cmd[0] == 15)
				sprintf(cmd_ack, "16ack%d", dut_cmd[*number - 1]);
			/*Set arx stop CMD */
			sprintf( input, "mp_arx stop");
			printf("%s\n", input);
			err = do_io_ctrl(ifname, input);
			if (dut_cmd[0] == 15) {
				/*Set Tx start CMD */
				printf("===================================================================\n");
				sprintf( input, "mp_ctx background,pkt");
				printf("%s\n", input);
				err = do_io_ctrl(ifname, input);
				for (int j = 0; j < 5; j++)
					usleep(100000);
				sprintf( input, "mp_ctx stop");
				printf("%s\n", input);
				err = do_io_ctrl(ifname, input);
				printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
			}
			secd_command(cmd_ack, send_cmd);
			printf("Send %s\n", cmd_ack);
			printf("Send %s\n", send_cmd);
			err = do_io_ctrl(ifname, send_cmd);
			secd_command(cmd_ack, send_cmd);
			printf("Send %s\n", cmd_ack);
			printf("Send %s\n", send_cmd);
			err = do_io_ctrl(ifname, send_cmd);
			secd_command(cmd_ack, send_cmd);
			printf("Send %s\n", cmd_ack);
			printf("Send %s\n", send_cmd);
			err = do_io_ctrl(ifname, send_cmd);
			/*Set arx start CMD */
			sprintf( input, "mp_arx start");
			printf("%s\n", input);
			err = do_io_ctrl(ifname, input);
			for (j = 0; j < 20; j++) {
				usleep(10000);
				RTW_MEMSET(input, 0, sizeof(input));
				sprintf(input, cmd_rx, strlen(cmd_rx));
				err = do_io_ctrl(ifname, input);
				if (input[0] != 0) {
					printf("Golden not recv ack input:%s\n", input);
					break;
				}
			}
			if (input[0] == 0)
				enable = 0;
			i++;
		} else {
			break;
		}
	}
	return err;
}


int set_dut_tx(char *ifname, int *dut_cmd, int *tx_idx)
{
	int err = 0;
	char txpath[2], rxpath[2];
	char input[1024 * 8];
	char *p;
	char *q;
	const char *comma = ",";
	char str_tmp[100] = {0};
	int i, j;
	/*Set arx stop CMD */
	sprintf( input, "mp_arx stop");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set BW CMD */
	sprintf( input, "mp_bandwidth 40M=%d,shortGI=0", dut_cmd[2]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set channel CMD */
	sprintf( input, "%s %d", "mp_channel", dut_cmd[1]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx Ant CMD */
	if (dut_cmd[3] == 8)
		strcpy(txpath, "a");
	else if (dut_cmd[3] == 4)
		strcpy(txpath, "b");
	else if (dut_cmd[3] == 2)
		strcpy(txpath, "c");
	else if (dut_cmd[3] == 1)
		strcpy(txpath, "d");
	sprintf( input, "mp_ant_tx %s", txpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Rx Ant CMD */
	if (dut_cmd[4] == 8)
		strcpy(rxpath, "a");
	else if (dut_cmd[4] == 4)
		strcpy(rxpath, "b");
	else if (dut_cmd[4] == 2)
		strcpy(rxpath, "c");
	else if (dut_cmd[4] == 1)
		strcpy(rxpath, "d");
	sprintf( input, "mp_ant_rx %s", rxpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx Rate CMD */
	sprintf( input, "mp_rate %d", dut_cmd[5]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx idx CMD */
	if (dut_cmd[6] == 1) {
		sprintf( input, "mp_txpower patha=%d,pathb=%d", dut_cmd[7], dut_cmd[8]);
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
	} else {
		sprintf(input, "mp_get_txpower");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
		//printf("input:%s\n", input);
		p = strtok(input, comma);
		for (i = 0; p != NULL; i++) {
			sscanf(p, "%s", str_tmp);
			//printf("%s\n", str_tmp);
			q = strchr(str_tmp, '=');
			//printf("%s\n", q + 1);
			sscanf(q + 1, "%d", &tx_idx[i]);
			p = strtok(NULL, comma);
			if (i >= 4)
				break;
		}
		sprintf( input, "mp_txpower patha=%d,pathb=%d",
			 tx_idx[0], tx_idx[1]);
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
	}
	//usleep(1000000);
	//usleep(500000);
#if 1
	/*Set arx start CMD */
	printf("===================================================================\n");
	sprintf( input, "mp_ctx background,pkt");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	for (j = 0; j < 13; j++)
		usleep(100000);
	/*Path B*/
	if (dut_cmd[4] == 4) {
		for (j = 0; j < 10; j++) {
			usleep(100000);
		}
	}
	//usleep(1500000);
	//usleep(500000);
	sprintf( input, "mp_ctx stop");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
#else
	char payload[1024] = {0};
	char send_cmd[1024] = {0};
	char send_payload[1024] = {0};
	int rx_counter[2] = {0};
	int number = 0;
	for (int j = 0; j < 5; j++) {
		/*Set arx start CMD */
		printf("===================================================================\n");
		sprintf( input, "mp_ctx background,pkt");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
		usleep(50000);
		sprintf( input, "mp_ctx stop");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
		printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
		/*Set arx start CMD */
		sprintf( input, "mp_arx start");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
		//while(1) {
		usleep(50000);
		char cmd_rx[]="mp_link rxdata";
		sprintf(input, cmd_rx, strlen(cmd_rx));
		err = do_io_ctrl(ifname, input);
		printf("11111111111111111111111     %s\n", input);
		//}
		/*Set arx start CMD */
		sprintf( input, "mp_arx stop");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
	}
#endif
	/*Set arx start CMD */
	sprintf( input, "mp_arx start");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	return err;
}



int set_dut_rx(char *ifname, int *dut_cmd, int *rx_counter)
{
	int err = 0;
	char txpath[2], rxpath[2];
	char input[1024 * 8];
	char *p;
	char *q;
	const char *phy_rx_ok = "packet OK:";
	const char *phy_crc_error= " CRC error:";
	const char *driver_rx_ok = "Rx OK:";
	const char *driver_crc_error= ", CRC error:";
	//int rx_counter = 0;
	/*Set arx stop CMD */
	sprintf( input, "mp_arx stop");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set BW CMD */
	sprintf( input, "mp_bandwidth 40M=%d,shortGI=0", dut_cmd[2]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set channel CMD */
	sprintf( input, "%s %d", "mp_channel", dut_cmd[1]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Tx Ant CMD */
	if (dut_cmd[3] == 8)
		strcpy(txpath, "a");
	else if (dut_cmd[3] == 4)
		strcpy(txpath, "b");
	else if (dut_cmd[3] == 2)
		strcpy(txpath, "c");
	else if (dut_cmd[3] == 1)
		strcpy(txpath, "d");
	sprintf( input, "mp_ant_tx %s", txpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set Rx Ant CMD */
	if (dut_cmd[4] == 8)
		strcpy(rxpath, "a");
	else if (dut_cmd[4] == 4)
		strcpy(rxpath, "b");
	else if (dut_cmd[4] == 2)
		strcpy(rxpath, "c");
	else if (dut_cmd[4] == 1)
		strcpy(rxpath, "d");
	sprintf( input, "mp_ant_rx %s", rxpath);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/* Set Tx Rate CMD */
	sprintf( input, "mp_rate %d", dut_cmd[5]);
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/* Reset Counter */
	sprintf( input, "mp_reset_stats");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	/*Set arx start CMD */
	sprintf( input, "mp_arx start");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	printf("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
	//sleep(1);
	usleep(2000000);
	sprintf( input, "mp_arx stop");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXxXXXX\n");
	sprintf(input, "mp_query");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	printf("%s\n", input);
	p = strstr(input, driver_rx_ok);
	q = strstr(input, driver_crc_error);
	//printf("%s\n", q);
	strncpy (q,"\0",1);
	//printf("%s\n", p + sizeof("Rx OK:") - 1);
	sscanf((p + sizeof("Rx OK:") - 1), "%d", &rx_counter[0]);
	//printf("%d\n", rx_counter[0]);
	sprintf(input, "mp_arx phy");
	printf("%s\n", input);
	err = do_io_ctrl(ifname, input);
	printf("%s\n", input);
	p = strstr(input, phy_rx_ok);
	q = strstr(input, phy_crc_error);
	//printf("%s\n", q);
	strncpy (q,"\0",1);
	sscanf((p + sizeof("packet OK:") - 1), "%d", &rx_counter[1]);
	//printf("%d\n", rx_counter[1]);
	return err;
}




int recv_ack_command(char *ifname, int dut_command)
{
	char input[1024 * 8];
	char cmd_rx[] = "mp_link rxdata";
	char cmd_6ack[5] = "6ack";
	char cmd_8ack[5] = "8ack";
	char payload[1024] = {0} ;
	int dut_cmd[1024] = {0};
	int err = 0, number = 0;
	sprintf(input, cmd_rx, strlen(cmd_rx));
	err = do_io_ctrl(ifname, input);
	//if (input[0] != 0)
	{
		//printf("\ninput=%s\n", input);
		parse_payload(input, payload, dut_cmd, &number);
		//printf("payload=%s\n", payload);
		//printf("cmd_ack=%s\n", cmd_ack);
		if (dut_command == 1) {
			if (!memcmp(payload, cmd_8ack, sizeof(cmd_8ack) - 1))
				printf("Recv 8ack\n");
			else {
				printf("Not Recv 8ack\n");
				err = -1;
			}
		} else if (dut_command == 2) {
			if (!memcmp(payload, cmd_6ack, sizeof(cmd_6ack) - 1))
				printf("Recv 6ack\n");
			else {
				printf("Not Recv 6ack\n");
				err = -1;
			}
		}
	}
	return err;
}



int mp_test(char *ifname)
{
	unsigned int i, j = 0, k = 0;
	int err = 0;
	char input[1024 * 8];
	//char cmd_tx[]="mp_link txdata,0011223344556677889900112233445566778899";
	//char cmd_tx[]="mp_link txdata,5265616c4c6f766554656b0011223354656b4c6f76655265616c";
	//RealLoveTek00112233TekLoveReal
	//char cmd_tx_ack[] = "ACK";
	char payload[1024] = {0};
	char send_cmd[1024] = {0};
	char send_payload[1024] = {0};
	int dut_cmd[1024] = {0};
	int rx_counter[2] = {0};
	int tx_idx[4] = {0};
	int number = 0;
	int pre_ch = 7, pre_bw = 0, pre_ant = 8;
#if 0
	while (1) {
		/* Tx Data */
		usleep(300000);
		//sleep(3);
		sprintf(input, cmd_tx, strlen(cmd_tx));
		err = do_io_ctrl(ifname, input);
	}
#endif
#if 0
	{
		char *p;
		char *q;
		const char *rx_ok = "Rx OK:";
		const char *crc_error= ", CRC error:";
		char str_tmp[100] = {0};
		int i, rx_counter;
		sprintf(input, "mp_query");
		printf("%s\n", input);
		err = do_io_ctrl(ifname, input);
		printf("input:%s\n", input);
		p = strstr(input, rx_ok);
		q = strstr(input, crc_error);
		printf("%s\n", q);
		strncpy (q,"\0",1);
		printf("%s\n", p + sizeof("Rx OK:") - 1);
	}
#endif
	set_dut_idle(ifname, pre_ch, pre_bw, pre_ant);
	while (1) {
		RTW_MEMSET(payload, 0, sizeof(char) * 1024);
		RTW_MEMSET(dut_cmd, 0, sizeof(int) * 1024);
		if (j == 20) {
			set_dut_idle(ifname, pre_ch, pre_bw, pre_ant);
			j = 0;
		} else {
			recv_command(ifname, payload, dut_cmd, &number);
			j++;
		}
#if 1
		if (payload[0] != 0) {
			printf("\npayload=%s\n", payload);
			j = 0;
			/* Tx CMD */
			//sprintf(input, cmd_tx_ack, strlen(cmd_tx_ack));
			//err = do_io_ctrl(ifname, input);
			if (dut_cmd[0] == 1) {
				set_dut_tx(ifname, dut_cmd, tx_idx);
				//set_dut_idle(ifname);
				k++;
				//for (i = 0; i < 10; i++)
				//	printf("dut_cmd[%d]=%d\n", i, dut_cmd[i]);
				//if (k == 2) {
				//	pre_ch = dut_cmd[1];
				//	pre_bw = dut_cmd[2];
				//	pre_ant = dut_cmd[3];
				//	k = 0;
				//}
#if 0
				if (dut_cmd[6] != 1) {
					i = 0;
					do {
						/*Set arx start CMD */
						sprintf( input, "mp_arx stop");
						printf("%s\n", input);
						err = do_io_ctrl(ifname, input);
						//usleep(10000);
						sprintf( send_payload, "7,%d,%d", tx_idx[0], tx_idx[1]);
						secd_command(send_payload, send_cmd);
						err = do_io_ctrl(ifname, send_cmd);
						printf("Send Tx Index PathA:%d PathB:%d\n", tx_idx[0], tx_idx[1]);
						sprintf( input, "mp_arx start");
						//printf("%s\n", input);
						err = do_io_ctrl(ifname, input);
						i++;
						usleep(50000);
					} while ((recv_ack_command(ifname, dut_cmd[0]) == -1) && i < 13);
				}
#endif
			}
			if (dut_cmd[0] == 2) {
				set_dut_rx(ifname, dut_cmd, rx_counter);
				//set_dut_idle(ifname);
				i = 0;
				do {
					/*Set arx start CMD */
					sprintf( input, "mp_arx stop");
					printf("%s\n", input);
					err = do_io_ctrl(ifname, input);
					/*Set Tx Ant CMD */
					sprintf( input, "mp_ant_tx a");
					printf("%s\n", input);
					err = do_io_ctrl(ifname, input);
					/*Set Rx Ant CMD */
					sprintf( input, "mp_ant_rx a");
					printf("%s\n", input);
					err = do_io_ctrl(ifname, input);
					sprintf( send_payload, "5,%d,%d",  rx_counter[0],  rx_counter[1]);
					secd_command(send_payload, send_cmd);
					err = do_io_ctrl(ifname, send_cmd);
					printf("Send Rx Counter Query:%d Phy:%d\n",  rx_counter[0],  rx_counter[1]);
					sprintf( input, "mp_arx start");
					//printf("%s\n", input);
					err = do_io_ctrl(ifname, input);
					i++;
					usleep(50000);
				} while ((recv_ack_command(ifname, dut_cmd[0]) == -1) && i < 13);
			}
			if (dut_cmd[0] == 9) {
				set_efuse(ifname, dut_cmd, number);
			}
			if (dut_cmd[0] == 13) {
				/*rtwpriv wlan0 mp_link setbt,scbd,1 or 0*/
				sprintf( input, "mp_link setbt,scbd,1");
				printf("%s\n", input);
				err = do_io_ctrl(ifname, input);
				sprintf( input, "mp_link setbt,testmode,2");
				printf("%s\n", input);
				err = do_io_ctrl(ifname, input);
				usleep(100000);
				sprintf( input, "mp_link setbt,testmode,1");
				printf("%s\n", input);
				err = do_io_ctrl(ifname, input);
				//adb shell setenforce 0
				sprintf( input, "setenforce 0");
				system(input);
				printf("%s\n", input);
				sprintf( input, "am startservice -n com.bluetooth.spptest/com.bluetooth.spptest.MyService");
				system(input);
				printf("%s\n", input);
			}
			if (payload[0] != 15) {
			}
			RTW_MEMSET(payload, 0, sizeof(payload));
		} else {
			//printf("Rev error\n");
		}
#endif
	}
#if 0
	/*Set channel CMD */
	sprintf( input, "%s %d", "mp_channel", channel);
	printf("%s ", input);
	err = do_io_ctrl(ifname, input);
	/* Tx CMD */
	sprintf(input, cmd_tx, strlen(cmd_tx));
	err = do_io_ctrl(ifname, input);
#endif
	return err;
}
}
