#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <hcuapi/fb.h>
#include <kernel/fb.h>
#include <hcuapi/dis.h>
#include <kernel/lib/console.h>
#include <kernel/lib/gzip.h>

#pragma pack(push, 1)  // 确保结构按1字节对齐
typedef struct tagBITMAPFILEHEADER {
	unsigned short bfType;
	unsigned long bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	unsigned long biSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)  // 恢复默认的结构对齐方式

static int isGzFile(const char *filename)
{
	ssize_t len = strlen(filename);
	if (len < 0)
		return 0;
	if (!strcmp(&filename[len - 3], ".gz"))
		return 1;
	return 0;
}

static int osdlogo_bmp(void *bmp)
{
	BITMAPFILEHEADER fileHeader;
	memcpy(&fileHeader, bmp, sizeof(fileHeader));

	// 检查文件类型是否为BMP
	if (fileHeader.bfType != 0x4D42) {  // 'BM' in little-endian
		printf("Not a BMP file.\n");
		return 1;
	}

	// 跳过文件头，读取位图信息头
	BITMAPINFOHEADER infoHeader;
	memcpy(&infoHeader, bmp + sizeof(BITMAPFILEHEADER), sizeof(infoHeader));

	// 打印图像的宽度和高度
	printf("Width: %ld pixels\n", infoHeader.biWidth);
	printf("Height: %ld pixels\n", infoHeader.biHeight);

	// 根据biBitCount确定每个像素所需的字节数
	int bytesPerPixel;
	switch (infoHeader.biBitCount) {
		case 24:
			bytesPerPixel = 3; // RGB
			break;
		case 32:
			bytesPerPixel = 4; // RGBA
			break;
			// 其他情况可能需要额外的处理或错误检查
		default:
			printf("Unsupported bit depth: %u\n", infoHeader.biBitCount);
			return 1;
	}

	if (bytesPerPixel != 4)
		return 1;

	// 计算图像数据的大小
	long biHeight = infoHeader.biHeight;
	if (biHeight < 0)
		infoHeader.biHeight = -biHeight;
	size_t imageDataSize = infoHeader.biWidth * infoHeader.biHeight * bytesPerPixel;
	unsigned char *imageData;

	// 读取图像数据（位图位）
	imageData = bmp + fileHeader.bfOffBits;

	// 此时imageData指向的缓冲区包含了图像的RGB数据
	// 根据需要处理这些数据，例如保存到文件或进行图像处理

	int fd = open("/dev/fb0", O_RDWR);
	ioctl(fd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_NO_CACHE);
	void *fb_base = (uint32_t *)mmap(NULL, imageDataSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!fb_base) {
		close(fd);
		return 1;
	}

	do {
		hcfb_scale_t scale_param = { infoHeader.biWidth, infoHeader.biHeight, 320, 240 };
		struct dis_screen_info screen = { 0 };
		int disfd = open("/dev/dis", O_RDWR);
		if (disfd < 0) {
			break;
		}

		screen.distype = DIS_TYPE_HD;
		if (ioctl(disfd, DIS_GET_SCREEN_INFO, &screen)) {
			close(disfd);
			printf("open /dev/dis failed\n");
			break;
		}
		close(disfd);
		scale_param.h_mul = screen.area.w;
		scale_param.v_mul = screen.area.h;
		ioctl(fd, HCFBIOSET_SCALE, &scale_param);
	} while (0);

	if (biHeight > 0) {
		int linesize = infoHeader.biWidth * bytesPerPixel;
		void *ptr = imageData + (infoHeader.biHeight - 1) * linesize;
		for(int i = infoHeader.biHeight - 1; i >= 0; i--) {
			memcpy(fb_base, ptr, linesize);
			fb_base += linesize;
			ptr -= linesize;
		}
	} else {
		memcpy(fb_base, imageData, imageDataSize);
	}

	ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK);

	return 0;
}

int osdlogo(int argc, char *argv[])
{
	struct stat sb;
	FILE *file = NULL;
	const char *filename;
	void *bmp = NULL;
	void *tmp = NULL;
	int rc = -1;
	size_t bytesRead;

	if (argc != 2) {
		printf("Usage: %s <BMP file path>\n", argv[0]);
		return -1;
	}

	filename = argv[1];
	if (stat(filename, &sb) != 0)
		return -1;

	size_t fsize = sb.st_size;
	file = fopen(filename, "rb");
	if (!file) {
		printf("Error opening file");
		return -1;
	}

	if (isGzFile(filename)) {
		unsigned long unc_len = 0x400000;
		tmp = malloc(fsize);
		if (!tmp) {
			printf("malloc fail\r\n");
			goto err_ret;
		}

		bytesRead = fread(tmp, 1, fsize, file);
		if (bytesRead != fsize) {
			printf("read fail\r\n");
			goto err_ret;
		}

		bmp = malloc(unc_len);
		if (!bmp) {
			printf("malloc fail\r\n");
			goto err_ret;
		}
		rc = gunzip(bmp, unc_len, tmp, &fsize);
		if (rc) {
			printf("gunzip fail\r\n");
			goto err_ret;
		}
	} else {
		bmp = malloc(fsize);
		if (!bmp) {
			printf("malloc fail\r\n");
			goto err_ret;
		}

		bytesRead = fread(bmp, 1, fsize, file);
		if (bytesRead != fsize) {
			printf("read fail\r\n");
			goto err_ret;
		}
	}

	rc = osdlogo_bmp(bmp);

err_ret:
	if (bmp)
		free(bmp);
	if (tmp)
		free(tmp);
	if (file)
		fclose(file);
	return rc;
}

CONSOLE_CMD(osdlogo, NULL, osdlogo, CONSOLE_CMD_MODE_SELF, "<file>")
