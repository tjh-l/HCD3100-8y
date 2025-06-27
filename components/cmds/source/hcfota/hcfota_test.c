#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <hcfota.h>
#include <kernel/lib/console.h>

#define max(a, b) ({\
		typeof(a) _a = a;\
		typeof(b) _b = b;\
		_a > _b ? _a : _b; })

static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata)
{
	unsigned char progress_sign[100 + 1];

	switch (event) {
	case HCFOTA_REPORT_EVENT_DOWNLOAD_PROGRESS: {
		int per = param;
		int i;

		for (i = 0; i < 100; i++) {
			if (i < per) {
				progress_sign[i] = '=';
			} else if (per == i) {
				progress_sign[i] = '>';
			} else {
				progress_sign[i] = ' ';
			}
		}

		progress_sign[sizeof(progress_sign) - 1] = '\0';

		printf("Downloading: [%s] %03d%%\033[1A", progress_sign, per);

		break;
	}

	case HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS: {
		int per = param;
		int i;

		for (i = 0; i < 100; i++) {
			if (i < per) {
				progress_sign[i] = '=';
			} else if (per == i) {
				progress_sign[i] = '>';
			} else {
				progress_sign[i] = ' ';
			}
		}

		progress_sign[sizeof(progress_sign) - 1] = '\0';

		printf("\033[1A");
		fflush(stdout);
		printf("\033[K");
		fflush(stdout);
		printf("Upgrading: [%s] %03d%%\n", progress_sign, per);

		break;
	}

	default:
		break;
	}

	return 0;
}

static void print_usage(const char *prog)
{
	printf("Usage: %s <file-path>\n", prog);
	puts("  upgrade from local file\n");
	printf("Usage: exflash %s <file-path>\n", prog);
	puts("  upgrade system partition to external flash\n");
	printf("Usage: exflash_data %s <file-path>\n", prog);
	puts("  upgrade system data to external flash\n");
	printf("Usage: %s <url>\n", prog);
	puts("  upgrade from network file\n");
	printf("Usage: %s info <file-path>\n", prog);
	puts("  Show information of local file\n");
	printf("Usage: %s infomem <addr> <size>\n", prog);
	puts("  Show information from fota in memory\n");
	printf("Usage: %s infomem2 <url>\n", prog);
	puts("  Show information of local file via data-request piece-by-piece\n");
	printf("Usage: %s info <url>\n", prog);
	puts("  Show information of network file\n");
	printf("Usage: %s download <url> <file-path>\n", prog);
	puts("  Download network file to local file\n");
	printf("Usage: %s reboot <mode> <url>\n", prog);
	puts("  Reboot with ota detect mode <mode>: [none | usbdevice | usbhost | sd | network]\n");
	puts("                              <url> mus be set if <mode> is [usbhost | sd | network]\n");
}

static ssize_t hcfota_request_read(int offset, char *buf, int nbytes, unsigned long usrdata)
{
	FILE *fp = (FILE *)usrdata;
	ssize_t rc;

	fseek(fp, offset, SEEK_SET);
	rc = fread(buf, 1, nbytes, fp);
	return rc;
}

int hcfota_test(int argc, char **argv)
{
	int len;

	if (argc == 1) {
		print_usage(argv[0]);
		return -1;
	}

	len = strlen(argv[1]);

	if (!strncmp(argv[1], "infomem2", max(len, 8))) {
		if (argc != 3) {
			print_usage(argv[0]);
			return -1;
		}
		FILE *fp = fopen(argv[2], "rb");
		int rc = hcfota_info_memory2(hcfota_request_read, (unsigned long)fp);
		fclose(fp);
		return rc;
	}

	if (!strncmp(argv[1], "infomem", max(len, 7))) {
		void *fota;
		unsigned long size;
		if (argc != 4) {
			print_usage(argv[0]);
			return -1;
		}
		fota = (void *)strtoul(argv[2], NULL, 0);
		size = (unsigned long)strtoul(argv[3], NULL, 0);
		return hcfota_info_memory(fota, size);
	}

	if (!strncmp(argv[1], "info", max(len, 4))) {
		if (argc != 3) {
			print_usage(argv[0]);
			return -1;
		}
		return hcfota_info_url(argv[2]);
	}

	if (!strncmp(argv[1], "download", max(len, 8))) {
		if (argc != 4) {
			print_usage(argv[0]);
			return -1;
		}
		return hcfota_download(argv[2], argv[3], hcfota_report, 0);
	}

	if (!strncmp(argv[1], "reboot", max(len, 6))) {
		unsigned long mode = 0;

		if (argc < 3) {
			return hcfota_reboot(0);
		}

		if (!strncmp(argv[2], "none", 4))
			mode = HCFOTA_REBOOT_OTA_DETECT_NONE;
		else if (!strncmp(argv[2], "usbdevice", 9))
			mode = HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE;
		else if (!strncmp(argv[2], "usbhost", 7)) {
			mode = HCFOTA_REBOOT_OTA_DETECT_USB_HOST;
		} else if (!strncmp(argv[2], "sd", 2)) {
			mode = HCFOTA_REBOOT_OTA_DETECT_SD;
		} else if (!strncmp(argv[2], "network", 7)) {
			mode = HCFOTA_REBOOT_OTA_DETECT_NETWORK;
		}

		mode = hcfota_reboot_ota_detect_mode_priority(mode, 0);
		return hcfota_reboot(mode);
	}

	return hcfota_url(argv[1], hcfota_report, 0);

	return 0;
}

CONSOLE_CMD(hcfota, NULL, hcfota_test, CONSOLE_CMD_MODE_SELF,
		"hcfota test operations")
