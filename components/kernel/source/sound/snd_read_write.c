/*
 * write all format to s16le interleaved;
 * read s16le interleaved to all format.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <nuttx/fs/fs.h>
#include <kernel/io.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <hcuapi/audsink.h>
#include <generated/br2_autoconf.h>

#define _READ(__data, __idx, __size, __shift) \
    (((__uint##__size##_t) (((const unsigned char *) (__data))[__idx])) << (__shift))

#define S16BETOS16(pin) (_READ(pin, 0, 16, 8) | _READ(pin, 1, 16, 0))
#define U16BETOS16(pin) (S16BETOS16(pin) - 0x8000)
#define S32BETOS16(pin) (_READ(pin, 0, 16, 8) | _READ(pin, 1, 16, 0))
#define U32BETOS16(pin) (S32BETOS16(pin) - 0x8000)

#define S8TOS16(in) ((int16_t)(in) << 8)
#define U8TOS16(in) ((int16_t)((in) - 0x80) << 8)
#define S16TOS16(in) ((int16_t)(in))
#define U16TOS16(in) ((int16_t)((in) - 0x8000))
#define S24LTOS16(in) (int16_t)((int32_t)(in) >> 16)
#define S24RTOS16(in) (int16_t)((int32_t)(in) >> 8)
#define U24LTOS16(in) (int16_t)((int32_t)((in) - 0x80000000) >> 16)
#define U24RTOS16(in) (int16_t)((int32_t)((in) - 0x800000) >> 8)
#define S32TOS16(in) (int16_t)((int32_t)(in) >> 16)
#define U32TOS16(in) (int16_t)((int32_t)((in) - 0x80000000) >> 16)
#define FLOATOS16(in) (int16_t)((in) * 32767.0f)

#define S16TOS16BE(pin) S16BETOS16(pin)
#define U16TOU16BE(pin) S16BETOS16(pin)
#define S16TOS32BE(pin) (_READ(pin, 0, 32, 8) | _READ(pin, 1, 32, 0))
#define U32TOU32BE_H16BITSVALID(pin) (_READ(pin, 2, 32, 8) | _READ(pin, 3, 32, 0))

#define S16TOS8(in) ((int8_t)((in) >> 8))
#define S16TOU8(in) ((uint8_t)(((in) >> 8) + 0x80))
#define S16TOU16(in) ((uint16_t)((in) + 0x8000))
#define S16TOS24L(in) (((int32_t)(in)) << 16)
#define S16TOS24R(in) (((int32_t)(in)) << 8)
#define S16TOU24L(in) (uint32_t)(S16TOS24L(in) + 0x80000000)
#define S16TOU24R(in) (uint32_t)(S16TOS24R(in) + 0x800000)
#define S16TOS32(in) S16TOS24L(in)
#define S16TOU32(in) (uint32_t)(((int32_t)((in) << 16)) + 0x80000000)
#define S16TOFLOAT(in) ((in) / 32767.0f)

//#define S16TOU16BE(pin)
//#define S16TOU32BE(pin)  

static void snd_write_s8(int16_t *dst, int8_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S8TOS16(*src);
			dst++;
			src++;
		}
	} else {
		int8_t *src2 = 0;
		if (channels == 2)
			src2 = src + frames;
		count = frames;
		while (count--) {
			*dst = S8TOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = S8TOS16(*src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_u8(int16_t *dst, uint8_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = U8TOS16(*src);
			dst++;
			src++;
		}
	} else {
		uint8_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (count--) {
			*dst = U8TOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = U8TOS16(*src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_s16(int16_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		memcpy(dst, src, frames * sizeof(int16_t) * channels);
	} else {
		int16_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (count--) {
			*dst = *src;
			dst++;
			if (channels == 2) {
				*dst = *src2;
				dst++;
			}
		}
	}

	return;
}

static void snd_write_s16_be(int16_t *dst, uint16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16BETOS16(src);
			dst++;
			src++;
		}
	} else {
		uint16_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = S16BETOS16(src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = S16BETOS16(src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_u16(int16_t *dst, uint16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = U16TOS16(*src);
			dst++;
			src++;
		}
	} else {
		uint16_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = U16TOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = U16TOS16(*src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_u16_be(int16_t *dst, uint16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = U16BETOS16(src);
			dst++;
			src++;
		}
	} else {
		uint16_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = U16BETOS16(src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = U16BETOS16(src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_s24(int16_t *dst, int32_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access, int right_align)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		if (right_align) {
			while (count--) {
				*dst = S24RTOS16(*src);
				dst++;
				src++;
			}
		} else {
			while (count--) {
				*dst = S24LTOS16(*src);
				dst++;
				src++;
			}
		}
	} else {
		int32_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		if (right_align) {
			while (frames--) {
				*dst = S24RTOS16(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst = S24RTOS16(*src2);
					dst++;
					src2++;
				}
			}
		} else {
			while (frames--) {
				*dst = S24LTOS16(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst = S24LTOS16(*src2);
					dst++;
					src2++;
				}
			}
		}
	}

	return;
}

static void snd_write_u24(int16_t *dst, uint32_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access, int right_align)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		if (right_align) {
			while (count--) {
				*dst = U24RTOS16(*src);
				dst++;
				src++;
			}
		} else {
			while (count--) {
				*dst = U24LTOS16(*src);
				dst++;
				src++;
			}
		}
	} else {
		uint32_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		if (right_align) {
			while (frames--) {
				*dst = U24RTOS16(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst = U24RTOS16(*src2);
					dst++;
					src2++;
				}
			}
		} else {
			while (frames--) {
				*dst = U24LTOS16(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst = U24LTOS16(*src2);
					dst++;
					src2++;
				}
			}
		}
	}

	return;
}

static void snd_write_s32(int16_t *dst, int32_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S32TOS16(*src);
			dst++;
			src++;
		}
	} else {
		int32_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = S32TOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = S32TOS16(*src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_s32_be(int16_t *dst, int32_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S32BETOS16(src);
			dst++;
			src++;
		}
	} else {
		int32_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = S32BETOS16(src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = S32BETOS16(src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_u32(int16_t *dst, uint32_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = U32TOS16(*src);
			dst++;
			src++;
		}
	} else {
		uint32_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = U32TOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = U32TOS16(*src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_u32_be(int16_t *dst, uint32_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = U32BETOS16(src);
			dst++;
			src++;
		}
	} else {
		uint32_t *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = U32BETOS16(src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = U32BETOS16(src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_write_float(int16_t *dst, float *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = FLOATOS16(*src);
			dst++;
			src++;
		}
	} else {
		float *src2 = 0;
		if (channels == 2) {
			src2 = src + frames;
		}
		count = frames;
		while (frames--) {
			*dst = FLOATOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst = FLOATOS16(*src2);
				dst++;
				src2++;
			}
		}
	}

	return;
}

static void snd_read_s8(int8_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOS8(*src);
			dst++;
			src++;
		}
	} else {
		int8_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		while (count--) {
			*dst = S8TOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst2 = S8TOS16(*src);
				dst2++;
				src++;
			}
		}
	}

	return;
}

static void snd_read_u8(uint8_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
		  *dst = S16TOU8(*src);
		  dst++;
		  src++;
		}
	} else {
		uint8_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		while (count--) {
			*dst = S16TOU8(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst2 = S16TOU8(*src);
				dst2++;
				src++;
			}
		}
	}

	return;
}

static void snd_read_s16(int16_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		memcpy(dst, src, frames * sizeof(int16_t) * channels);
	} else {
		int16_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		while (count--) {
			*dst = S16TOS16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst2 = S16TOS16(*src);
				dst2++;
				src++;
			}
		}
	}

	return;
}

static void snd_read_s16_be(uint16_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOS16BE(src);
			dst++;
			src++;
		}
	} else {
		uint16_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		while (count--) {
			*dst = S16TOS16BE(src);
			dst++;
			src++;
			if (channels == 2) {
				*dst2 = S16TOS16BE(src);
				dst2++;
				src++;
			}
		}
	}

	return;
}

static void snd_read_u16(uint16_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
		  *dst = S16TOU16(*src);
		  dst++;
		  src++;
		}
	} else {
		uint16_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		while (count--) {
			*dst = S16TOU16(*src);
			dst++;
			src++;
			if (channels == 2) {
				*dst2 = S16TOU16(*src);
				dst2++;
				src++;
			}
		}
	}

	return;
}

static void snd_read_u16_be(uint16_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOU16(*src);
			*dst = U16TOU16BE(dst);
			dst++;
			src++;
		}
	} else {
		uint16_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		while (count--) {
			*dst = S16TOU16(*src);
			*dst = U16TOU16BE(dst);
			dst++;
			src++;
			if (channels == 2) {
				*dst2 = S16TOU16(*src);
				*dst2 = U16TOU16BE(dst2);
				dst2++;
				src++;
			}
		}
	}

	return;
}

static void snd_read_s24(int32_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access, int right_align)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		if (right_align) {
			while (count--) {
				*dst = S16TOS24R(*src);
				dst++;
				src++;
			}
		} else {
			while (count--) {
				*dst = S16TOS24L(*src);
				dst++;
				src++;
			}
		}
	} else {
		int32_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		if (right_align) {
			while (count--) {
				*dst = S16TOS24R(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2 = S16TOS24R(*src);
					dst2++;
					src++;
				}
			}
		} else {
			while (count--) {
				*dst = S16TOS24L(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2 = S16TOS24L(*src);
					dst2++;
					src++;
				}
			}
		}
	}

	return;
}

static void snd_read_u24(uint32_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access, int right_align)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		if (right_align) {
			while (count--) {
				*dst = S16TOU24R(*src);
				dst++;
				src++;
			}
		} else {
			while (count--) {
				*dst = S16TOU24L(*src);
				dst++;
				src++;
			}
		}
	} else {
		uint32_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		if (right_align) {
			while (count--) {
				*dst = S16TOU24R(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2 = S16TOU24R(*src);
					dst2++;
					src++;
				}
			}
		} else {
			while (count--) {
				*dst = S16TOU24L(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2 = S16TOU24L(*src);
					dst2++;
					src++;
				}
			}
		}
	}

	return;
}

static void snd_read_s32(int32_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOS32(*src);
			dst++;
			src++;
		}
	} else {
		int32_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
			while (count--) {
				*dst = S16TOS32(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2 = S16TOS32(*src);
					dst2++;
					src++;
				}
			}
	}

	return;
}

static void snd_read_s32_be(uint32_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOS32BE(src);
			dst++;
			src++;
		}
	} else {
		uint32_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
			count = frames;
			while (count--) {
				*dst = S16TOS32BE(src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2 = S16TOS32BE(src);
					dst2++;
					src++;
				}
			}
	}

	return;
}

static void snd_read_u32(uint32_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOU32(*src);
			dst++;
			src++;
		}
	} else {
		uint32_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
			count = frames;
			while (count--) {
				*dst = S16TOU32(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2 = S16TOU32(*src);
					dst2++;
					src++;
				}
			}
	}

	return;
}

static void snd_read_u32_be(uint32_t *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOU32(*src);
			*dst = U32TOU32BE_H16BITSVALID(dst);
			dst++;
			src++;
		}
	} else {
		uint32_t *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
		while (count--) {
			*dst = S16TOU32(*src);
			*dst = U32TOU32BE_H16BITSVALID(dst);
			dst++;
			src++;
			if (channels == 2) {
				*dst2 = S16TOU32(*src);
				*dst2 = U32TOU32BE_H16BITSVALID(dst2);
				dst2++;
				src++;
			}
		}
	}

	return;
}

static void snd_read_float(float *dst, int16_t *src,
	snd_pcm_uframes_t frames, int channels, snd_pcm_access_t access)
{
	int count;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		count = frames * channels;
		while (count--) {
			*dst = S16TOFLOAT(*src);
			dst++;
			src++;
		}
	} else {
		float *dst2 = 0;
		if (channels == 2)
			dst2 = dst + frames;
		count = frames;
			while (count--) {
				*dst = S16TOFLOAT(*src);
				dst++;
				src++;
				if (channels == 2) {
					*dst2++ = S16TOFLOAT(*src++);
					dst2++;
					src++;
				}
			}
	}

	return;
}

void snd_write_samples (int16_t *dst, void *src, snd_pcm_uframes_t frames,
	snd_pcm_format_t format, int channels, snd_pcm_access_t access, int right_align)
{
	if (format == SND_PCM_FORMAT_S8) {
		snd_write_s8(dst, (int8_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U8) {
		snd_write_u8(dst, (uint8_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S16) {
		snd_write_s16(dst, (int16_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S16_BE) {
		snd_write_s16_be(dst, (uint16_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U16) {
		snd_write_u16(dst, (uint16_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U16_BE) {
		snd_write_u16_be(dst, (uint16_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S24) {
		snd_write_s24(dst, (int32_t *)src, frames, channels, access, right_align);
	} else if (format == SND_PCM_FORMAT_U24) {
		snd_write_u24(dst, (uint32_t *)src, frames, channels, access, right_align);
	} else if (format == SND_PCM_FORMAT_S32) {
		snd_write_s32(dst, (int32_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S32_BE) {
		snd_write_s32_be(dst, (uint32_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U32) {
		snd_write_u32(dst, (uint32_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U32_BE) {
		snd_write_u32_be(dst, (uint32_t *)src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_FLOAT) {
		snd_write_float(dst, (float *)src, frames, channels, access);
	} else {
		printf("format not support\n");
	} 
}

void snd_read_samples (void *dst, int16_t *src, snd_pcm_uframes_t frames,
	snd_pcm_format_t format, int channels, snd_pcm_access_t access, int right_align)
{
	if (format == SND_PCM_FORMAT_S8) {
		snd_read_s8((uint8_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U8) {
		snd_read_u8((uint8_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S16) {
		snd_read_s16((int16_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S16_BE) {
		snd_read_s16_be((uint16_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U16) {
		snd_read_u16((uint16_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U16_BE) {
		snd_read_u16_be((uint16_t *)dst,src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S24) {
		snd_read_s24((int32_t *)dst, src, frames, channels, access, right_align);
	} else if (format == SND_PCM_FORMAT_U24) {
		snd_read_u24((uint32_t *)dst, src, frames, channels, access, right_align);
	} else if (format == SND_PCM_FORMAT_S32) {
		snd_read_s32((int32_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_S32_BE) {
		snd_read_s32_be((uint32_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U32) {
		snd_read_u32((uint32_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_U32_BE) {
		snd_read_u32_be((uint32_t *)dst, src, frames, channels, access);
	} else if (format == SND_PCM_FORMAT_FLOAT) {
		snd_read_float((float *)dst, src, frames, channels, access);
	} else {
		printf("format not support\n");
	} 
}
