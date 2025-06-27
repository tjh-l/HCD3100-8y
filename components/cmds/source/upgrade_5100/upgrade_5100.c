#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <hcuapi/sci.h>
#include <sys/ioctl.h>
	
#include <kernel/delay.h>
#include <kernel/lib/console.h>
	
	  
	 
	 
#define BURN_SIZE       5*1024
	
int burn_5100(int argc, char *argv[])
{
	FILE *fp = NULL;
	int fd = -1;
	unsigned char *n205sram;
	int upgradeflg = 0;
	unsigned char sync[2] ={0x60,0x00};
	unsigned char Ackbuf[128] = {0};
	unsigned char execbuf[10] = {0};
	char ret;
	struct pollfd fds[1];
	nfds_t nfds = 1;
	unsigned char writebuf[128] = {0};
	unsigned char readCMD[128] = {0}; 
	int bindex = 0, FWsize = 0, frameLen = 0, j = 0;
	struct sci_setting sci;
	
	sci.parity_mode = PARITY_EVEN;
	sci.bits_mode=bits_mode_default;
#if CONFIG_5100_UART_UPGARDE == 1
	printf("uart upgrade ...\n");
	fd = open("/dev/uart0", O_RDWR);//|O_NONBLOCK
	ioctl(fd, SCIIOC_SET_BAUD_RATE_115200, NULL);
	ioctl(fd, SCIIOC_SET_SETTING, &sci);
#elif CONFIG_5100_I2C_UPGARDE == 1
	printf("i2c upgrade ...\n");
	fd = open("dev/i2c1", O_RDWR);
#endif

	fp = fopen("/media/sda1/hi5100a.bin", "r");

	if (fp == NULL) {
		printf("open usb fail\n");
		return -1;
	}

	if (fd < 0) {
		printf("open uart fail\n");
		close(fd);
		return -1;
	}

	n205sram = malloc(BURN_SIZE);
	if (n205sram == NULL) {
		printf("Cannot malloc buf\n");
		return -1;
	}

	memset(n205sram, 0, BURN_SIZE);
	fread(n205sram, BURN_SIZE, 1, fp);

	fseek(fp, 0L, SEEK_END);
	FWsize = ftell(fp);
	
	fds[0].fd = fd;
	fds[0].events  = POLLIN;
	fds[0].revents = 0;
	printf("upgrade FWare Size = %d\n", FWsize);

	while (1) {
		upgradeflg = 0;
		write(fd, sync, 1);
		ret = poll(fds, nfds, 250);
		if((ret>0)&&(fds[0].revents == POLLIN)){
			read(fd, Ackbuf, 1);
		}
		
		usleep(30*1000);//30
		if(Ackbuf[0] == 0x60){
			memset(Ackbuf, 0, 128); 

			writebuf[0] = 0x44; ///64
			readCMD[0] = 0x34; ///64
			frameLen = (1<<(writebuf[0]&0x0f))*4;
			for(bindex=0; bindex<FWsize; bindex += frameLen){	
				writebuf[1] = (unsigned char)bindex;
				writebuf[2] = (unsigned char)(bindex>>8);
				writebuf[3] = (unsigned char)(bindex>>16);
				writebuf[4] = (unsigned char)(bindex>>24);
				
				//memcpy(&writebuf[5], &hi5100flashsram[bindex], frameLen);
				memcpy(&writebuf[5], &n205sram[bindex], frameLen);
				//memcpy(&writebuf[5], &n205flash[bindex], frameLen);			
				write(fd, writebuf, frameLen+5);
				if((bindex%4096) == 0){
					usleep(800*1000);//1000
				}
				else{
					usleep(200*1000);//300
				}
				readCMD[1] = (unsigned char)bindex;
				readCMD[2] = (unsigned char)(bindex>>8);
				readCMD[3] = (unsigned char)(bindex>>16);
				readCMD[4] = (unsigned char)(bindex>>24);
				write(fd, readCMD, 5);
				usleep(200*1000);//300
				for(j=0; j<frameLen; j++){
					ret = poll(fds, nfds, 10);
					if((ret>0)&&(fds[0].revents == POLLIN)){
						read(fd, &Ackbuf[j], 1);
					}
				}

				if(memcmp(Ackbuf, &writebuf[5], frameLen) != 0){
					upgradeflg = 1;
					break;
				}
				memset(Ackbuf, 0, 128); 							
				usleep(100*1000);
			}
			if(upgradeflg == 0){
				execbuf[0] = 0x50;
				execbuf[1] = (unsigned char)FWsize;
				execbuf[2] = (unsigned char)(FWsize>>8);
				execbuf[3] = (unsigned char)(FWsize>>16);
				execbuf[4] = (unsigned char)(FWsize>>24);
				execbuf[5] = 2;
				write(fd, execbuf, 6);
				printf("upgrade success......\n");
			}
			usleep(10*1000);
			memset(Ackbuf, 0, 10);
		}		
		usleep(5*1000);
	}

	free(n205sram);
	fclose(fp);
	close(fd);

	return 0;
}



CONSOLE_CMD(upgrade_test, NULL, burn_5100, CONSOLE_CMD_MODE_SELF, "5100");

