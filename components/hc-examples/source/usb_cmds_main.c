#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#if 1//def __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#else
#include <kernel/lib/console.h>
#endif

static struct termios stored_settings;


static int usb_debug_speed_cmds(int argc, char **argv)
{
#define TEST_TOTAL_SIZE (160 * 1024 * 1024)    
#define BUFFER_SIZE (512 * 1024)    
	char *buf;
	int rc;
	int cnt = 0;
	struct timeval tv1, tv2;
    int cost_ms;
    FILE *fp;


    if(argc != 2){
        printf("=---> Error command\n");
        return -1;
    }


	fp = fopen(argv[1], "w+");
	if (fp == NULL) {
        printf("Error: Cannot create %s\n", argv[1]);
        return -1;
	}
    printf("=---> Create file(%s) successfully...\n", argv[1]); 


    buf = malloc(BUFFER_SIZE);    
	if (buf == NULL) {
        printf("Error: Cannot malloc 64KB buffer\n");
        return -1;
	}
    memset(buf, 0xa5, BUFFER_SIZE);




    printf("=---> Try to write file(%s)...\n", argv[1]);
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = fwrite(buf, BUFFER_SIZE, 1, fp);
        
        cnt += rc;
        //printf("=---> Write file(%s) ... (offset: %d)\n", argv[1], cnt);


        if(cnt >= TEST_TOTAL_SIZE / BUFFER_SIZE)
            break;
        
	}
	gettimeofday(&tv2, NULL);
    printf("=---> Write file(%s) successfully (offset: %d)...\n", argv[1], cnt);
    cnt *= (BUFFER_SIZE / 1024);


    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);


	printf("total bytes %d KB\n", cnt);
	printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("duration: %d ms\n", cost_ms);
    printf("speed: %d MB/s, %d KB/s\n",  
            (cnt / 1000) / (cost_ms/1000),
            cnt / (cost_ms/1000));

	cnt = 0;
	gettimeofday(&tv1, NULL);
	fseek(fp, 0, SEEK_SET);
	
	for (;;) {
		rc = fread(buf, BUFFER_SIZE,1,fp);
        if (rc < 0) {
			printf("[%s][%d] ******read error rc = %d\n",__FUNCTION__,__LINE__,rc);
			break;
		}
        cnt += (rc*BUFFER_SIZE);
        //printf("=---> read file(%s) ... (offset: %d)\n", argv[1], cnt);


        if(cnt >= TEST_TOTAL_SIZE)
            break;
        
	}
	gettimeofday(&tv2, NULL);
	cnt /= 1024;

    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);


	printf("total bytes %d KB\n", cnt);
	printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("duration: %d ms\n", cost_ms);
    printf("read speed: %d MB/s, %d KB/s\n",  
            (cnt / 1024) / (cost_ms/1000), 
            cnt / (cost_ms/1000));


	fclose(fp);
	free(buf);


	return 0;
}

static int usb_debug_speed_test_cmds(int argc, char **argv)
{
#define TEST_TOTAL_SIZE (64 * 1024 * 1024)    
#define BUFFER_SIZE (512 * 1024)    
	char *buf;
	int rc;
	int cnt = 0;
	struct timeval tv1, tv2;
    int cost_ms;
    int fp;


    if(argc != 2){
        printf("=---> Error command\n");
        return -1;
    }


	fp = open(argv[1], O_RDWR | O_CREAT | O_TRUNC,0600);
	if (fp < 0) {
        printf("Error: Cannot create %s\n", argv[1]);
        return -1;
	}
    printf("=---> Create file(%s) successfully...\n", argv[1]); 


    buf = malloc(BUFFER_SIZE);    
	if (buf == NULL) {
        printf("Error: Cannot malloc 64KB buffer\n");
        return -1;
	}
    memset(buf, 0xa5, BUFFER_SIZE);




    printf("=---> Try to write file(%s)...\n", argv[1]);
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = write(fp,buf, BUFFER_SIZE);
        if (rc != BUFFER_SIZE) {
			printf("[%s][%d] ******write error rc = %d\n",__FUNCTION__,__LINE__,rc);
			break;
		}
        cnt += rc;
        //printf("=---> Write file(%s) ... (offset: %d)\n", argv[1], cnt);


        if(cnt >= TEST_TOTAL_SIZE)
            break;
        
	}
	gettimeofday(&tv2, NULL);
    printf("=---> Write file(%s) successfully (offset: %d)...\n", argv[1], cnt);
    //cnt *= (BUFFER_SIZE / 1024);
	cnt /= 1024;

    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);


	printf("total bytes %d KB\n", cnt);
	printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("duration: %d ms\n", cost_ms);
    printf("write speed: %d MB/s, %d KB/s\n",  
            (cnt / 1024) / (cost_ms/1000), 
            cnt / (cost_ms/1000));
	cnt = 0;
	gettimeofday(&tv1, NULL);
	lseek(fp, 0, SEEK_SET);
	
	for (;;) {
		rc = read(fp,buf, BUFFER_SIZE);
        if (rc != BUFFER_SIZE) {
			printf("[%s][%d] ******read error rc = %d\n",__FUNCTION__,__LINE__,rc);
			break;
		}
        cnt += rc;
        //printf("=---> read file(%s) ... (offset: %d)\n", argv[1], cnt);


        if(cnt >= TEST_TOTAL_SIZE)
            break;
        
	}
	gettimeofday(&tv2, NULL);
	cnt /= 1024;

    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);


	printf("total bytes %d KB\n", cnt);
	printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("duration: %d ms\n", cost_ms);
    printf("read speed: %d MB/s, %d KB/s\n",  
            (cnt / 1024) / (cost_ms/1000), 
            cnt / (cost_ms/1000));
	fsync(fp);
	close(fp);
	
	free(buf);


	return 0;
}

static void exit_console (int signo) {
    (void)signo;
    tcsetattr(0 , TCSANOW , &stored_settings);
    exit(0);
}

int __storage_device_speed_test(char *path, size_t per_bytes, size_t total_bytes)
{
	char *buf;
	struct timeval tv1, tv2;
    int cost_ms, rc, cnt = 0;
    FILE *fp;

    buf = malloc(per_bytes);    
	if (buf == NULL) {
        printf("Error: Cannot malloc 64KB buffer\n");
        return -1;
	}
    memset(buf, 'k', per_bytes);

	fp = fopen(path, "w+");
	if (fp == NULL) {
        printf("Error: Cannot create %s\n", path);
        return -1;
	}
    // printf("=---> Create file(%s) successfully...\n", path); 

    printf("=---> Try to write file(%s) %ldMB test data...\n", path, total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = fwrite(buf, per_bytes, 1, fp);
        cnt += rc;
        if((cnt >= total_bytes / per_bytes) || (rc != 1))
            break;
	}
	gettimeofday(&tv2, NULL);
    printf("=---> Write file(%s) successfully (offset: %d)...\n", path, cnt);
    cnt *= (per_bytes);

    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

    printf("========================================================\n");
    printf("File: %s\n", path); 
    printf("Length per fread/fwrite: %ld (%ld KB)\n", per_bytes, per_bytes / 1024);
	printf("fwrite: total bytes %d KB, %d MB\n", cnt / 1000, cnt / 1000000);
	// printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	// printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("fwrite: duration: %d ms\n", cost_ms);
    printf("fwrite: speed: %d.%d MB/s, %d KB/s\n",  
            (cnt / 1000) / cost_ms, ((cnt) / cost_ms) % 1000,
            (cnt) / cost_ms);
    printf("========================================================\n\n");
    fclose(fp);

    /* ***************************************************** */
    cnt = 0;
	fp = fopen(path, "r");
	if (fp == NULL) {
        printf("Error: Cannot open %s\n", path);
        return -1;
	}
    printf("=---> Try to read file(%s) %ldMB test data...\n", path, total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = fread(buf, per_bytes, 1, fp);
        cnt += rc;
        if((cnt >= total_bytes / per_bytes) || (rc != 1))
            break;
	}
	gettimeofday(&tv2, NULL);
    printf("=---> Read file(%s) successfully (offset: %d)...\n", path, cnt);
    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

    printf("========================================================\n");
    printf("File: %s\n", path);
    printf("Length per fread/fwrite: %ld (%ld KB)\n", per_bytes, per_bytes / 1024);
	printf("fread: total bytes %d KB, %d MB\n", cnt / 1000, cnt / 1000000);
	// printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	// printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("fread: duration: %d ms\n", cost_ms);
    printf("fread: speed: %d.%d MB/s, %d KB/s\n",  
            (cnt / 1000) / cost_ms, ((cnt) / cost_ms) % 1000,
            (cnt) / cost_ms);
    printf("========================================================\n\n");
    fclose(fp);

    free(buf);
    return 0;
}


int __storage_device_speed_test2(char *path, size_t per_bytes, size_t total_bytes)
{
	char *buf;
	struct timeval tv1, tv2;
    int cost_ms, rc, cnt = 0;
    int fd;

    buf = malloc(per_bytes);    
	if (buf == NULL) {
        printf("Error: Cannot malloc 64KB buffer\n");
        return -1;
	}
    memset(buf, 'k', per_bytes);

	fd = open(path,O_RDWR | O_CREAT | O_TRUNC,0600);
	if (fd < 0) {
        printf("Error: Cannot create %s\n", path);
        return -1;
	}
    // printf("=---> Create file(%s) successfully...\n", path); 

    printf("\n=---> Try to write file(%s) %ldMB test data...\n", path, total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = write(fd, buf, per_bytes);
        cnt += rc;
        if(rc != per_bytes || cnt > total_bytes)
            break;
	}
	if(cnt <= total_bytes) {
		printf("[%s][%d] write file %s fail\n",__FUNCTION__,__LINE__,path);
		return -1;
	}
	gettimeofday(&tv2, NULL);
    // printf("=---> Write file(%s) successfully (offset: %d)...\n", path, cnt);

    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

    printf("========================================================\n");
    printf("File: %s\n", path);
    printf("Length per read/write: %ld (%ld KB)\n", per_bytes, per_bytes / 1024);
	printf("Write: total bytes %d MB, %d KB\n", cnt / 1000000, cnt / 1000);
	// printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	// printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("Write: duration: %d ms\n", cost_ms);
    printf("Write: speed: %d.%d MB/s, %d KB/s\n",  
            (cnt / 1000) / cost_ms, ((cnt) / cost_ms) % 1000,
            (cnt) / cost_ms);
    printf("========================================================\n\n");
    close(fd);

    /* ***************************************************** */
    cnt = 0;
    lseek(fd, 0, SEEK_SET);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
        printf("Error: Cannot open %s\n", path);
        return -1;
	}
    printf("=---> Try to read file(%s) %ldMB test data...\n", path, total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = read(fd, buf, per_bytes);
        cnt += rc;
        if(rc != per_bytes || cnt > total_bytes)
            break;
	}
	gettimeofday(&tv2, NULL);
    // printf("=---> Read file(%s) successfully (offset: %d)...\n", path, cnt);
    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

    printf("========================================================\n");
    printf("File: %s\n", path);
    printf("Length per read/write: %ld (%ld KB)\n", per_bytes, per_bytes / 1024);
	printf("Write: total bytes %d MB, %d KB\n", cnt / 1000000, cnt / 1000);
	// printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	// printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("Read: duration: %d ms\n", cost_ms);
    printf("Read: speed: %d.%d MB/s, %d KB/s\n",  
            (cnt / 1000) / cost_ms, ((cnt) / cost_ms) % 1000,
            (cnt) / cost_ms);
    printf("========================================================\n\n");
    close(fd);

    free(buf);
    return 0;
}



int storage_device_speed_test(int argc, char **argv)
{
    DIR *dir;
    struct dirent *dp;
    char file_name[512] = {0};

    dir = opendir("/media");
    if(dir == NULL){
        printf("Error: please plugin U-disk or SD/TF card\n");
        return -1;
    }

    while((dp = readdir(dir)) != NULL){
        printf("\n\t/media/%s: \n", dp->d_name);
		if(!strcmp(dp->d_name,".")||!strcmp(dp->d_name,"..")) {
			continue;
		}
        sprintf(&file_name[0], "/media/%s/hichip-test.bin", dp->d_name);
        __storage_device_speed_test2(file_name, 128 * 1024, 64 * 1024 * 1024);
        __storage_device_speed_test2(file_name, 512 * 1024, 32 * 1024 * 1024);
        __storage_device_speed_test2(file_name, 256 * 1024, 64 * 1024 * 1024);
        __storage_device_speed_test2(file_name, 128 * 1024, 64 * 1024 * 1024);
        __storage_device_speed_test2(file_name, 64 * 1024, 64 * 1024 * 1024);
        __storage_device_speed_test2(file_name, 32 * 1024, 64 * 1024 * 1024);
        __storage_device_speed_test2(file_name, 16 * 1024, 64 * 1024 * 1024);
        __storage_device_speed_test2(file_name, 4 * 1024, 64 * 1024 * 1024);
    }

    closedir(dir);
    return 0;
}

int main (int argc , char *argv[]) {
	struct termios new_settings;

    tcgetattr(0 , &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0 , TCSANOW , &new_settings);

    signal(SIGTERM , exit_console);
    signal(SIGINT , exit_console);
    signal(SIGSEGV , exit_console);
    signal(SIGBUS , exit_console);
    console_init("usb_cmds:");

    console_register_cmd(NULL , "start" , usb_debug_speed_cmds , CONSOLE_CMD_MODE_SELF , "start ");
	console_register_cmd(NULL , "speed_test" , storage_device_speed_test , CONSOLE_CMD_MODE_SELF , "speed_test");
	
    console_start();
    exit_console(0);
    (void)argc;
    (void)argv;
    return 0;
}
