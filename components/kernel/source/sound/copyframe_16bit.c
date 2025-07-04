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

void audsink_copy_to_s16lep(void *dst, void *src, snd_pcm_uframes_t frames, int ipitch,
    uint8_t channels_in, snd_pcm_format_t format, snd_pcm_access_t access, int align)
{
	int16_t *out;
	uint8_t ch;
	int istep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	switch (format) {
		case SND_PCM_FORMAT_S8:
		{
			char *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (char *)src + ch;
				} else {
					in = (char *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = S8TOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_U8:
		{
			unsigned char *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (unsigned char *)src + ch;
				} else {
					in = (unsigned char *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = U8TOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_S16:
		{
			int16_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (int16_t *)src + ch;
				} else {
					in = (int16_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = S16TOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_S16_BE:
		{
			int16_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (int16_t *)src + ch;
				} else {
					in = (int16_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = S16BETOS16(in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_U16:
		{
			uint16_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (uint16_t *)src + ch;
				} else {
					in = (uint16_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = U16TOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_U16_BE:
		{
			uint16_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (uint16_t *)src + ch;
				} else {
					in = (uint16_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = U16BETOS16(in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_S24:
		{
			int32_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (int32_t *)src + ch;
				} else {
					in = (int32_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					if (SND_PCM_ALIGN_LEFT == align)
						*out = S24LTOS16(*in);
					else
						*out = S24RTOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_U24:
		{
			uint32_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (uint32_t *)src + ch;
				} else {
					in = (uint32_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					if (SND_PCM_ALIGN_LEFT == align)
						*out = U24LTOS16(*in);
					else
						*out = U24RTOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_S32:
		{
			int32_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (int32_t *)src + ch;
				} else {
					in = (int32_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = S32TOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_S32_BE:
		{
			int32_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (int32_t *)src + ch;
				} else {
					in = (int32_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = S32BETOS16(in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_U32:
		{
			uint32_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (uint32_t *)src + ch;
				} else {
					in = (uint32_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = U32TOS16(*in);
					out++;
					in += istep;
				}
			}
			break;
		}

		case SND_PCM_FORMAT_U32_BE:
		{
			uint32_t *in;
			for (ch = 0; ch < channels_in; ch++) {
				if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
					in = (uint32_t *)src + ch;
				} else {
					in = (uint32_t *)src + ch * ipitch;
				}

				out = (int16_t *)dst + ch * frames;

				for (i = 0; i < frames; i++) {
					*out = U32BETOS16(in);
					out++;
					in += istep;
				}
			}
			break;
		}

		default:
			printf("unsupport audio format!!!\n");
			break;
	}

	return;
}

#ifdef CONFIG_AUDSINK_16BIT_OUT
void audsink_copy_raw(void *dst, void *src, snd_pcm_uframes_t frames)
{
	memcpy(dst, src, frames);
}

void audsink_copy_s8(void *dst, void *src, snd_pcm_uframes_t frames,
		     snd_pcm_uframes_t pitch, uint8_t channels_in,
		     snd_pcm_access_t access, int dup)
{
	char *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (char *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (char *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (char *)src;
					in2 = (char *)src + 1;
					break;
				}
			} else {
				in = (char *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (char *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (char *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (char *)src;
					in2 = (char *)src + pitch;
					break;
				}
			} else {
				in = (char *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S8TOS16(*in) + S8TOS16(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S8TOS16(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (char *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S8TOS16(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u8(void *dst, void *src, snd_pcm_uframes_t frames,
		     snd_pcm_uframes_t pitch, uint8_t channels_in,
		     snd_pcm_access_t access, int dup)
{
	unsigned char *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (unsigned char *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (unsigned char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (unsigned char *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (unsigned char *)src;
					in2 = (unsigned char *)src + 1;
					break;
				}
			} else {
				in = (unsigned char *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (unsigned char *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (unsigned char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (unsigned char *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (unsigned char *)src;
					in2 = (unsigned char *)src + pitch;
					break;
				}
			} else {
				in = (unsigned char *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U8TOS16(*in) + U8TOS16(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U8TOS16(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (unsigned char *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U8TOS16(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_s16(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	int16_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + 1;
					break;
				}
			} else {
				in = (int16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (int16_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S16TOS16(*in) + S16TOS16(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S16TOS16(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (int16_t *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S16TOS16(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_s16_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup)
{
	int16_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + 1;
					break;
				}
			} else {
				in = (int16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (int16_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S16BETOS16(in) + S16BETOS16(in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S16BETOS16(in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (int16_t *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S16BETOS16(in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u16(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	uint16_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (uint16_t *)src + 1;
					break;
				}
			} else {
				in = (uint16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (uint16_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U16TOS16(*in) + U16TOS16(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U16TOS16(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint16_t *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U16TOS16(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u16_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup)
{
	uint16_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (uint16_t *)src + 1;
					break;
				}
			} else {
				in = (uint16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (uint16_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U16BETOS16(in) + U16BETOS16(in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U16BETOS16(in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint16_t *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U16BETOS16(in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_s24(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup, uint8_t align)
{
	int32_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + 1;
					break;
				}
			} else {
				in = (int32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + pitch;
					break;
				}
			} else {
				in = (int32_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = (S24LTOS16(*in) + S24LTOS16(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = (S24RTOS16(*in) + S24RTOS16(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			}
			break;
		} else {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = S24LTOS16(*in);
					out += ostep;
					in += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = S24RTOS16(*in);
					out += ostep;
					in += istep;
				}
			}
		}
	}

	if (channels_in == 1) {
		in = (int32_t *)src;
		out = (int16_t *)dst + 1;

		if (align == SND_PCM_ALIGN_LEFT) {
			for (i = 0; i < frames; i++) {
				*out = S24LTOS16(*in);
				out += ostep;
				in++;
			}
		} else {
			for (i = 0; i < frames; i++) {
				*out = S24RTOS16(*in);
				out += ostep;
				in++;
			}
		}
	}

	return;
}

void audsink_copy_u24(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup, uint8_t align)
{
	uint32_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + 1;
					break;
				}
			} else {
				in = (uint32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + pitch;
					break;
				}
			} else {
				in = (uint32_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = (U24LTOS16(*in) + U24LTOS16(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = (U24RTOS16(*in) + U24RTOS16(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			}
			break;
		} else {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = U24LTOS16(*in);
					out += ostep;
					in += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = U24RTOS16(*in);
					out += ostep;
					in += istep;
				}
			}
		}
	}

	if (channels_in == 1) {
		in = (uint32_t *)src;
		out = (int16_t *)dst + 1;

		if (align == SND_PCM_ALIGN_LEFT) {
			for (i = 0; i < frames; i++) {
				*out = U24LTOS16(*in);
				out += ostep;
				in++;
			}
		} else {
			for (i = 0; i < frames; i++) {
				*out = U24RTOS16(*in);
				out += ostep;
				in++;
			}
		}
	}

	return;
}

void audsink_copy_s32(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	int32_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + 1;
					break;
				}
			} else {
				in = (int32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + pitch;
					break;
				}
			} else {
				in = (int32_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S32TOS16(*in) + S32TOS16(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S32TOS16(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (int32_t *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S32TOS16(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

  void audsink_copy_s32_be(void *dst, void *src, snd_pcm_uframes_t frames,
			   snd_pcm_uframes_t pitch, uint8_t channels_in,
			   snd_pcm_access_t access, int dup)
  {
	  int32_t *in, *in2;
	  int16_t *out;
	  uint8_t ch;
	  int istep;
	  int ostep;
	  snd_pcm_uframes_t i;

	  if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		  istep = channels_in;
	  } else {
		  istep = 1;
	  }

	  if (channels_in == 1) {
		  ostep = 2;
	  } else {
		  ostep = channels_in;
	  }

	  for (ch = 0; ch < channels_in; ch++) {
		  in2 = NULL;
		  if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			  if (channels_in == 2) {
				  switch (dup) {
				  case AUDSINK_PCM_DUPLICATE_STEREO:
					  in = (int32_t *)src + ch;
					  break;
				  case AUDSINK_PCM_DUPLICATE_LEFT:
					  in = (int32_t *)src;
					  break;
				  case AUDSINK_PCM_DUPLICATE_RIGHT:
					  in = (int32_t *)src + 1;
					  break;
				  case AUDSINK_PCM_DUPLICATE_MONO:
				  default:
					  in = (int32_t *)src;
					  in2 = (int32_t *)src + 1;
					  break;
				  }
			  } else {
				  in = (int32_t *)src + ch;
			  }
		  } else {
			  if (channels_in == 2) {
				  switch (dup) {
				  case AUDSINK_PCM_DUPLICATE_STEREO:
					  in = (int32_t *)src + ch * pitch;
					  break;
				  case AUDSINK_PCM_DUPLICATE_LEFT:
					  in = (int32_t *)src;
					  break;
				  case AUDSINK_PCM_DUPLICATE_RIGHT:
					  in = (int32_t *)src + pitch;
					  break;
				  case AUDSINK_PCM_DUPLICATE_MONO:
				  default:
					  in = (int32_t *)src;
					  in2 = (int32_t *)src + pitch;
					  break;
				  }
			  } else {
				  in = (int32_t *)src + ch * pitch;
			  }
		  }

		  out = (int16_t *)dst + ch;

		  if (in2) {
			  for (i = 0; i < frames; i++) {
				  *out = (S32BETOS16(in) + S32BETOS16(in2)) >> 1;
				  *(out + 1) = *out;
				  out += ostep;
				  in += istep;
				  in2 += istep;
			  }
			  break;
		  } else {
			  for (i = 0; i < frames; i++) {  
				  *out = S32BETOS16(in);
				  out += ostep;
				  in += istep;  
			  }
		  }
	  }

	  if (channels_in == 1) {
		  in = (int32_t *)src;
		  out = (int16_t *)dst + 1;

		  for (i = 0; i < frames; i++) {
			  *out = S32BETOS16(in);
			  out += ostep;
			  in++;
		  }
	  }

	  return;
  }


void audsink_copy_u32(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	uint32_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + 1;
					break;
				}
			} else {
				in = (uint32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + pitch;
					break;
				}
			} else {
				in = (uint32_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U32TOS16(*in) + U32TOS16(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U32TOS16(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint32_t *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U32TOS16(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u32_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup)
{
	uint32_t *in, *in2;
	int16_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + 1;
					break;
				}
			} else {
				in = (uint32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + pitch;
					break;
				}
			} else {
				in = (uint32_t *)src + ch * pitch;
			}
		}

		out = (int16_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U32BETOS16(in) + U32BETOS16(in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U32BETOS16(in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint32_t *)src;
		out = (int16_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U32BETOS16(in);
			out += ostep;
			in++;
		}
	}

	return;
}
#endif
