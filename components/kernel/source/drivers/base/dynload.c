#include <generated/br2_autoconf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <cpu_func.h>
#include <kernel/ld.h>
#include <kernel/io.h>
#include <kernel/module.h>
#include <kernel/dynload.h>
#include <nuttx/mtd/mtd.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  include <sys/mount.h>
#endif
#include <kernel/lib/lzma.h>
#include <kernel/lib/gzip.h>
#ifdef BR2_PACKAGE_LIBLZO
#include <lzo1x.h>
#endif

#define OVERLAY_SECTION(a) (unsigned long)&__load_start_overlay_##a, (unsigned long)&__load_stop_overlay_##a, #a
#define DYNLOAD_SECTION(a) (unsigned long)&__load_start_dynload_##a, (unsigned long)&__load_stop_dynload_##a, (unsigned long)&__dynload_##a##_start, 0, #a

#if (defined(BR2_FIRMWARE_OVERLAY) || defined(BR2_FIRMWARE_DYNLOAD))
struct overlay_section {
	enum DYNLOAD_SECTION_ID id;
	unsigned long load_start;
	unsigned long load_stop;
	const char *name;
};

struct dynload_section {
	enum DYNLOAD_SECTION_ID id;
	unsigned long load_start;
	unsigned long load_stop;
	unsigned long load_addr;
	int is_loaded;
	const char *name;
};

#if (defined(BR2_FIRMWARE_OVERLAY))
static struct overlay_section overlay_audio_sections[] = {
	{ DYNLOAD_SECTION_ID_MP3, OVERLAY_SECTION(mp3) },
	{ DYNLOAD_SECTION_ID_AAC, OVERLAY_SECTION(aac) },
	{ DYNLOAD_SECTION_ID_AACELD, OVERLAY_SECTION(aaceld) },
	{ DYNLOAD_SECTION_ID_AC3, OVERLAY_SECTION(ac3) },
	{ DYNLOAD_SECTION_ID_EAC3, OVERLAY_SECTION(eac3) },
	{ DYNLOAD_SECTION_ID_PCM, OVERLAY_SECTION(pcm) },
	{ DYNLOAD_SECTION_ID_FLAC, OVERLAY_SECTION(flac) },
	{ DYNLOAD_SECTION_ID_VORBIS, OVERLAY_SECTION(vorbis) },
	{ DYNLOAD_SECTION_ID_WMA, OVERLAY_SECTION(wma) },
	{ DYNLOAD_SECTION_ID_WMAPRO, OVERLAY_SECTION(wmapro) },
	{ DYNLOAD_SECTION_ID_OPUS, OVERLAY_SECTION(opus) },
	{ DYNLOAD_SECTION_ID_RA, OVERLAY_SECTION(ra) },
	{ DYNLOAD_SECTION_ID_APE, OVERLAY_SECTION(ape) },
	{ DYNLOAD_SECTION_ID_PCMDVD, OVERLAY_SECTION(pcmdvd) },
	{ DYNLOAD_SECTION_ID_ALAC, OVERLAY_SECTION(alac) },
};

static struct overlay_section overlay_video_sections[] = {
	{ DYNLOAD_SECTION_ID_MPEG2, OVERLAY_SECTION(mpeg2) },
	{ DYNLOAD_SECTION_ID_MPEG4, OVERLAY_SECTION(mpeg4) },
	{ DYNLOAD_SECTION_ID_IMAGE, OVERLAY_SECTION(image) },
	{ DYNLOAD_SECTION_ID_H264, OVERLAY_SECTION(h264) },
	{ DYNLOAD_SECTION_ID_RV, OVERLAY_SECTION(rv) },
	{ DYNLOAD_SECTION_ID_VC1, OVERLAY_SECTION(vc1) },
	{ DYNLOAD_SECTION_ID_VP8, OVERLAY_SECTION(vp8) },
};

static struct overlay_section overlay_wifi_sections[] = {
	{ DYNLOAD_SECTION_ID_RTL8188FU, OVERLAY_SECTION(rtl8188fu) },
	{ DYNLOAD_SECTION_ID_RTL8188EU, OVERLAY_SECTION(rtl8188eu) },
	{ DYNLOAD_SECTION_ID_RTL8723AS, OVERLAY_SECTION(rtl8723as) },
	{ DYNLOAD_SECTION_ID_RTL8723BS, OVERLAY_SECTION(rtl8723bs) },
	{ DYNLOAD_SECTION_ID_RTL8733BU, OVERLAY_SECTION(rtl8733bu) },
	{ DYNLOAD_SECTION_ID_RTL8811CU, OVERLAY_SECTION(rtl8811cu) },
	{ DYNLOAD_SECTION_ID_RTL8822CS, OVERLAY_SECTION(rtl8822cs) },
};

static enum DYNLOAD_SECTION_ID last_overlay_audio_section = DYNLOAD_SECTION_ID_NONE;
static enum DYNLOAD_SECTION_ID last_overlay_video_section = DYNLOAD_SECTION_ID_NONE;
static enum DYNLOAD_SECTION_ID last_overlay_wifi_section = DYNLOAD_SECTION_ID_NONE;

static struct overlay_section *find_overlay_section(enum DYNLOAD_SECTION_ID id, struct overlay_section *sections, int nb_section)
{
	int i;
	for (i = 0; i < nb_section; i++) {
		if (id == sections[i].id)
			return &sections[i];
	}

	return NULL;
}
#endif

#if (defined(BR2_FIRMWARE_DYNLOAD))
static struct dynload_section dynload_sections[] = {
	{ DYNLOAD_SECTION_ID_MEDIAPLAYER, DYNLOAD_SECTION(mediaplayer) },
};

static struct dynload_section *find_dynload_section(enum DYNLOAD_SECTION_ID id, struct dynload_section *sections, int nb_section)
{
	int i;
	for (i = 0; i < nb_section; i++) {
		if (id == sections[i].id)
			return &sections[i];
	}

	return NULL;
}
#endif

static int load_and_uncompress_section(unsigned long load_buf, const char *charname)
{
	int fd;
	struct stat sb;
	ssize_t image_len;
	void *image_buf;
	int unc_len = INT_MAX;
	unsigned long decomp_zimage = 0;
	int rc;

	(void)(unc_len);

	if (stat(charname, &sb) == -1)
		return -1;

	fd = open(charname, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	image_len = (ssize_t)sb.st_size;
	lseek(fd, 0, SEEK_SET);
#ifdef BR2_FIRMWARE_DYNLOAD_COMPRESS_NONE
	if (read(fd, load_buf, image_len) != image_len) {
		close(fd);
		return -EIO;
	} else {
		close(fd);
		return 0;
	}
#endif

	image_buf = malloc(image_len);
	if (!image_buf) {
		close(fd);
		return -ENOMEM;
	}
	if (read(fd, (void *)image_buf, image_len) != image_len) {
		close(fd);
		free(image_buf);
		return -EIO;
	}

#ifdef BR2_FIRMWARE_DYNLOAD_COMPRESS_GZIP
	rc = gunzip((void *)load_buf, unc_len, image_buf, (unsigned long *)&image_len);
#endif

#ifdef BR2_FIRMWARE_DYNLOAD_COMPRESS_LZMA
	rc = lzmaBuffToBuffDecompress((unsigned char *)load_buf, (unsigned long *)&unc_len, image_buf, image_len);
#endif

#ifdef BR2_FIRMWARE_DYNLOAD_COMPRESS_LZO1X
	rc = lzo1x_decompress((const unsigned char *)image_buf, image_len, (void *)load_buf, &decomp_zimage, NULL);
#endif

	close(fd);
	free(image_buf);

	(void)(decomp_zimage);

	return rc;
}

int dynload_builtin_section(enum DYNLOAD_SECTION_ID id)
{
	char dyn_bin_path[64] = {0};
	struct overlay_section *poverlay = NULL;
	struct dynload_section *pdynload = NULL;
	unsigned long load_addr = 0;
	int rc = 0;

	(void)(poverlay);
	(void)(pdynload);

	if (id == DYNLOAD_SECTION_ID_NONE)
		return 0;

#if (defined(BR2_FIRMWARE_OVERLAY))
	if (id == last_overlay_audio_section)
		return 0;
	if (id == last_overlay_video_section)
		return 0;
	if (id == last_overlay_wifi_section)
		return 0;

	load_addr = (unsigned long)&__overlay_audio_start;
	poverlay = find_overlay_section(id, overlay_audio_sections, ARRAY_SIZE(overlay_audio_sections));
	if (!poverlay) {
		load_addr = (unsigned long)&__overlay_video_start;
		poverlay = find_overlay_section(id, overlay_video_sections, ARRAY_SIZE(overlay_video_sections));
	}

	if (!poverlay) {
		load_addr = (unsigned long)&__overlay_wifi_start;
		poverlay = find_overlay_section(id, overlay_wifi_sections, ARRAY_SIZE(overlay_wifi_sections));
	}

	if (poverlay) {
		if (poverlay->load_start == poverlay->load_stop)
			return 0;

		snprintf(dyn_bin_path, sizeof(dyn_bin_path), "/lib/overlay_%s.bin", poverlay->name);
		rc = load_and_uncompress_section(load_addr, dyn_bin_path);
		if (rc == 0) {
			cache_flush((void *)load_addr, poverlay->load_stop - poverlay->load_start);
			icache_invalidate((void *)load_addr, poverlay->load_stop - poverlay->load_start);
			if (load_addr == (unsigned long)&__overlay_audio_start)
				last_overlay_audio_section = id;
			else if (load_addr == (unsigned long)&__overlay_video_start)
				last_overlay_video_section = id;
			else if (load_addr == (unsigned long)&__overlay_wifi_start)
				last_overlay_wifi_section = id;
			printf("load from %s success\r\n", dyn_bin_path);
		} else {
			printf("load from %s fail\r\n", dyn_bin_path);
		}

		return rc;
	}
#endif
#if (defined(BR2_FIRMWARE_DYNLOAD))
	pdynload = find_dynload_section(id, dynload_sections, ARRAY_SIZE(dynload_sections));
	if (pdynload) {
		if (pdynload->load_start == pdynload->load_stop || pdynload->is_loaded)
			return 0;

		load_addr = pdynload->load_addr;
		snprintf(dyn_bin_path, sizeof(dyn_bin_path), "/lib/dynload_%s.bin", pdynload->name);
		rc = load_and_uncompress_section(load_addr, dyn_bin_path);
		if (rc == 0) {
			cache_flush((void *)load_addr, pdynload->load_stop - pdynload->load_start);
			icache_invalidate((void *)load_addr, pdynload->load_stop - pdynload->load_start);
			pdynload->is_loaded = true;
		} else {
			printf("load from %s fail\r\n", dyn_bin_path);
		}
	}
#endif

	return rc;
}

static int get_block_devpath(char *devpath, int len, const char *name)
{
	int rc;

	if ((rc = get_mtd_device_index_nm(name)) >= 0) {
		snprintf(devpath, len, "/dev/mtdblock%d", rc);
		return 0;
	}

	return -1;
}

static int dynload_module_init(void)
{
	int ret;
	char devpath[64];
#if defined(BR2_FIRMWARE_DYNLOAD_PARTITION1)
	ret = get_block_devpath(devpath, sizeof(devpath), "eromfs");
#elif defined(BR2_FIRMWARE_DYNLOAD_PARTITION2)
	ret = get_block_devpath(devpath, sizeof(devpath), "eromfs2");
#elif defined(BR2_FIRMWARE_DYNLOAD_PARTITION3)
	ret = get_block_devpath(devpath, sizeof(devpath), "eromfs3");
#endif
	if (ret >= 0) {
		ret = mount(devpath, "/lib", "romfs", MS_RDONLY, NULL);
		if (ret < 0) {
			printf("mount failed for dynload romfs\r\n");
		}
	}

	return 0;
}
module_rootfs(dynload, dynload_module_init, NULL, 0)
#else
int dynload_builtin_section(enum DYNLOAD_SECTION_ID id)
{
	return 0;
}
#endif
