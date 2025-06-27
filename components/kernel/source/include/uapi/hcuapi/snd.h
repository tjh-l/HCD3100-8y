#ifndef _HCUAPI_SND_H_
#define _HCUAPI_SND_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/codec_id.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

#define SND_IOCTL_GETCAP			_IOR (SND_IOCBASE, 0, struct snd_capability)
#define SND_IOCTL_HW_PARAMS			_IOW (SND_IOCBASE, 1, struct snd_pcm_params)
#define SND_IOCTL_HW_FREE			_IO  (SND_IOCBASE, 2)
#define SND_IOCTL_START				_IO  (SND_IOCBASE, 3)
#define SND_IOCTL_DROP				_IO  (SND_IOCBASE, 4)
#define SND_IOCTL_DELAY				_IOR (SND_IOCBASE, 5, snd_pcm_uframes_t)
#define SND_IOCTL_DRAIN				_IO  (SND_IOCBASE, 6)
#define SND_IOCTL_PAUSE				_IO  (SND_IOCBASE, 7)
#define SND_IOCTL_RESUME			_IO  (SND_IOCBASE, 8)
#define SND_IOCTL_XFER				_IOWR(SND_IOCBASE, 9, struct snd_xfer)
#define SND_IOCTL_AVAIL_MIN			_IOW (SND_IOCBASE, 10, snd_pcm_uframes_t)
#define SND_IOCTL_SET_VOLUME			_IOW (SND_IOCBASE, 11, uint8_t)
#define SND_IOCTL_GET_VOLUME			_IOR (SND_IOCBASE, 12, uint8_t)
#define SND_IOCTL_SRC_SEL			_IOW (SND_IOCBASE, 13, snd_pcm_source_t)
#define SND_IOCTL_SRC_SEL_CLEAR			_IOW (SND_IOCBASE, 14, snd_pcm_source_t)
#define SND_IOCTL_SET_MUTE			_IO  (SND_IOCBASE, 15) //<! param: 0 un-mute; 1 mute
#define SND_IOCTL_GET_HW_INFO			_IOR (SND_IOCBASE, 16, struct snd_hw_info)
#define SND_IOCTL_SET_RECORD			_IO  (SND_IOCBASE, 17)
#define SND_IOCTL_FREE_RECORD			_IO  (SND_IOCBASE, 18)
#define SND_IOCTL_SET_FREE_RECORD		_IO  (SND_IOCBASE, 18) /* Deprecated */
#define SND_IOCTL_SET_TWOTONE			_IOW (SND_IOCBASE, 19, struct snd_twotone)
#define SND_IOCTL_SET_LR_BALANCE		_IOW (SND_IOCBASE, 20, struct snd_lr_balance)
#define SND_IOCTL_SET_EQ6			_IOW (SND_IOCBASE, 21, struct snd_audio_eq6)
#define SND_IOCTL_SET_FLUSH_TIME		_IO (SND_IOCBASE, 22)

#define SND_IOCTL_SET_EQ_ONOFF			_IO (SND_IOCBASE, 24) //<! param: 0 disable eq; 1 enable eq.
#define SND_IOCTL_SET_EQ_BAND			_IOW (SND_IOCBASE, 25, struct snd_eq_band_setting)

#define SND_IOCTL_SET_PBE			_IO (SND_IOCBASE, 26) //<! param: strength 0 ~ 100; 0 means disable. Perceptual bass enhancement.
#define SND_IOCTL_SET_PBE_PRECUT		_IO (SND_IOCBASE, 27) //<! param: -45 ~ 0; unit dB
#define SND_IOCTL_SET_AUTO_MUTE_ONOFF		_IO (SND_IOCBASE, 28) //<! param: 0 disable auto-mute; 1 enable auto-mute
#define SND_IOCTL_SET_EXTRA_DATA_PATH		_IO (SND_IOCBASE, 29) //<! param: 0 no extra data path; or loopthrough to AUDSINK_SND_DEVBIT_<I2SO | PCMO | SPO | DDP_SPO>, refer to audsink.h
#define SND_IOCTL_SET_EXTRA_DATA_PATH_ONOFF	_IO (SND_IOCBASE, 30) //<! param: 0 turn-off extra data path; 1 turn-on extra data path

#define SND_IOCTL_SET_DRC_PARAM			_IOW (SND_IOCBASE, 31, struct snd_drc_setting)
#define SND_IOCTL_GET_DRC_PARAM			_IOR (SND_IOCBASE, 31, struct snd_drc_setting)
#define SND_IOCTL_SET_EOF			_IO (SND_IOCBASE, 33) //<! param: 1 eof; 0 not eof.
#define SND_IOCTL_SET_HDMI_MUTE			_IO (SND_IOCBASE, 34) //<! param: 0 un-mute; 1 mute

#define SND_IOCTL_SET_VOLUME_MAP		_IOW (SND_IOCBASE, 35, struct snd_volume_map)
#define SND_IOCTL_GET_VOLUME_MAP		_IOR (SND_IOCBASE, 35, struct snd_volume_map)

#define SND_IOCTL_SET_PINMUX_DATA_INACTIVE	 _IO (SND_IOCBASE, 36) //<! param: 0 active; 1 inactive
#define SND_IOCTL_SET_LIMITER_PARAM		_IOW (SND_IOCBASE, 38, struct snd_limiter_setting)

#define SND_EVENT_AUDIO_INFO			_IOW (SND_IOCBASE, 50, struct snd_pcm_params)
#define SND_EVENT_UNDERRUN			_IO  (SND_IOCBASE, 51)

#define SND_IOCTL_SET_CJC8988_INPUT		_IOW  (SND_IOCBASE, 100, struct snd_cjc8988_input)

typedef unsigned long snd_pcm_uframes_t;

typedef int snd_pcm_access_t;
#define SND_PCM_ACCESS_RW_INTERLEAVED		((snd_pcm_access_t) 0) /* readi/writei */
#define SND_PCM_ACCESS_RW_NONINTERLEAVED	((snd_pcm_access_t) 1) /* readn/writen */
#define SND_PCM_ACCESS_LAST			SND_PCM_ACCESS_RW_NONINTERLEAVED

typedef int snd_pcm_format_t;
#define	SND_PCM_FORMAT_S8			((snd_pcm_format_t) 0)
#define	SND_PCM_FORMAT_U8			((snd_pcm_format_t) 1)
#define	SND_PCM_FORMAT_S16_LE			((snd_pcm_format_t) 2)
#define	SND_PCM_FORMAT_S16_BE			((snd_pcm_format_t) 3)
#define	SND_PCM_FORMAT_U16_LE			((snd_pcm_format_t) 4)
#define	SND_PCM_FORMAT_U16_BE			((snd_pcm_format_t) 5)
#define	SND_PCM_FORMAT_S24_LE			((snd_pcm_format_t) 6) /* low three bytes */
#define	SND_PCM_FORMAT_S24_BE			((snd_pcm_format_t) 7) /* low three bytes */
#define	SND_PCM_FORMAT_U24_LE			((snd_pcm_format_t) 8) /* low three bytes */
#define	SND_PCM_FORMAT_U24_BE			((snd_pcm_format_t) 9) /* low three bytes */
#define	SND_PCM_FORMAT_S32_LE			((snd_pcm_format_t) 10)
#define	SND_PCM_FORMAT_S32_BE			((snd_pcm_format_t) 11)
#define	SND_PCM_FORMAT_U32_LE			((snd_pcm_format_t) 12)
#define	SND_PCM_FORMAT_U32_BE			((snd_pcm_format_t) 13)
#define	SND_PCM_FORMAT_FLOAT_LE			((snd_pcm_format_t) 14) /* 4-byte float, IEEE-754 32-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_FLOAT_BE			((snd_pcm_format_t) 15) /* 4-byte float, IEEE-754 32-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_FLOAT64_LE		((snd_pcm_format_t) 16) /* 8-byte float, IEEE-754 64-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_FLOAT64_BE		((snd_pcm_format_t) 17) /* 8-byte float, IEEE-754 64-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_IEC958_SUBFRAME_LE	((snd_pcm_format_t) 18) /* IEC-958 subframe, Little Endian */
#define	SND_PCM_FORMAT_IEC958_SUBFRAME_BE	((snd_pcm_format_t) 19) /* IEC-958 subframe, Big Endian */
#define	SND_PCM_FORMAT_MU_LAW			((snd_pcm_format_t) 20)
#define	SND_PCM_FORMAT_A_LAW			((snd_pcm_format_t) 21)
#define	SND_PCM_FORMAT_IMA_ADPCM		((snd_pcm_format_t) 22)
#define	SND_PCM_FORMAT_MPEG			((snd_pcm_format_t) 23)
#define	SND_PCM_FORMAT_GSM			((snd_pcm_format_t) 24)
#define	SND_PCM_FORMAT_SPECIAL			((snd_pcm_format_t) 31)
#define	SND_PCM_FORMAT_S24_3LE			((snd_pcm_format_t) 32)	/* in three bytes */
#define	SND_PCM_FORMAT_S24_3BE			((snd_pcm_format_t) 33)	/* in three bytes */
#define	SND_PCM_FORMAT_U24_3LE			((snd_pcm_format_t) 34)	/* in three bytes */
#define	SND_PCM_FORMAT_U24_3BE			((snd_pcm_format_t) 35)	/* in three bytes */
#define	SND_PCM_FORMAT_S20_3LE			((snd_pcm_format_t) 36)	/* in three bytes */
#define	SND_PCM_FORMAT_S20_3BE			((snd_pcm_format_t) 37)	/* in three bytes */
#define	SND_PCM_FORMAT_U20_3LE			((snd_pcm_format_t) 38)	/* in three bytes */
#define	SND_PCM_FORMAT_U20_3BE			((snd_pcm_format_t) 39)	/* in three bytes */
#define	SND_PCM_FORMAT_S18_3LE			((snd_pcm_format_t) 40)	/* in three bytes */
#define	SND_PCM_FORMAT_S18_3BE			((snd_pcm_format_t) 41)	/* in three bytes */
#define	SND_PCM_FORMAT_U18_3LE			((snd_pcm_format_t) 42)	/* in three bytes */
#define	SND_PCM_FORMAT_U18_3BE			((snd_pcm_format_t) 43)	/* in three bytes */
#define	SND_PCM_FORMAT_G723_24			((snd_pcm_format_t) 44) /* 8 samples in 3 bytes */
#define	SND_PCM_FORMAT_G723_24_1B		((snd_pcm_format_t) 45) /* 1 sample in 1 byte */
#define	SND_PCM_FORMAT_G723_40			((snd_pcm_format_t) 46) /* 8 Samples in 5 bytes */
#define	SND_PCM_FORMAT_G723_40_1B		((snd_pcm_format_t) 47) /* 1 sample in 1 byte */
#define	SND_PCM_FORMAT_DSD_U8			((snd_pcm_format_t) 48) /* DSD, 1-byte samples DSD (x8) */
#define	SND_PCM_FORMAT_DSD_U16_LE		((snd_pcm_format_t) 49) /* DSD, 2-byte samples DSD (x16), little endian */
#define	SND_PCM_FORMAT_RAW			((snd_pcm_format_t) 50) /* bitstream raw data */
#define	SND_PCM_FORMAT_LAST			SND_PCM_FORMAT_RAW

#define	SND_PCM_FORMAT_S16			SND_PCM_FORMAT_S16_LE
#define	SND_PCM_FORMAT_U16			SND_PCM_FORMAT_U16_LE
#define	SND_PCM_FORMAT_S24			SND_PCM_FORMAT_S24_LE
#define	SND_PCM_FORMAT_U24			SND_PCM_FORMAT_U24_LE
#define	SND_PCM_FORMAT_S32			SND_PCM_FORMAT_S32_LE
#define	SND_PCM_FORMAT_U32			SND_PCM_FORMAT_U32_LE
#define	SND_PCM_FORMAT_FLOAT			SND_PCM_FORMAT_FLOAT_LE
#define	SND_PCM_FORMAT_FLOAT64			SND_PCM_FORMAT_FLOAT64_LE
#define	SND_PCM_FORMAT_IEC958_SUBFRAME		SND_PCM_FORMAT_IEC958_SUBFRAME_LE

#define SND_PCM_RATE_5512			(1<<0)		/* 5512Hz */
#define SND_PCM_RATE_8000			(1<<1)		/* 8000Hz */
#define SND_PCM_RATE_11025			(1<<2)		/* 11025Hz */
#define SND_PCM_RATE_12000			(1<<3)		/* 11025Hz */
#define SND_PCM_RATE_16000			(1<<4)		/* 16000Hz */
#define SND_PCM_RATE_22050			(1<<5)		/* 22050Hz */
#define SND_PCM_RATE_24000			(1<<6)		/* 22050Hz */
#define SND_PCM_RATE_32000			(1<<7)		/* 32000Hz */
#define SND_PCM_RATE_44100			(1<<8)		/* 44100Hz */
#define SND_PCM_RATE_48000			(1<<9)		/* 48000Hz */
#define SND_PCM_RATE_64000			(1<<10)		/* 64000Hz */
#define SND_PCM_RATE_88200			(1<<11)		/* 88200Hz */
#define SND_PCM_RATE_96000			(1<<12)		/* 96000Hz */
#define SND_PCM_RATE_128000			(1<<13)		/* 96000Hz */
#define SND_PCM_RATE_176400			(1<<14)		/* 176400Hz */
#define SND_PCM_RATE_192000			(1<<15)		/* 192000Hz */

#define SND_PCM_RATE_8000_44100                                                \
	(SND_PCM_RATE_8000 | SND_PCM_RATE_11025 | SND_PCM_RATE_12000 |         \
	 SND_PCM_RATE_16000 | SND_PCM_RATE_22050 | SND_PCM_RATE_24000 |        \
	 SND_PCM_RATE_32000 | SND_PCM_RATE_44100)
#define SND_PCM_RATE_8000_48000 (SND_PCM_RATE_8000_44100 | SND_PCM_RATE_48000)
#define SND_PCM_RATE_8000_96000                                                \
	(SND_PCM_RATE_8000_48000 | SND_PCM_RATE_64000 | SND_PCM_RATE_88200 |   \
	 SND_PCM_RATE_96000)
#define SND_PCM_RATE_8000_192000                                               \
	(SND_PCM_RATE_8000_96000 | SND_PCM_RATE_176400 | SND_PCM_RATE_192000)

#define _SND_PCM_FMTBIT(fmt)			(1ULL << (int)SND_PCM_FORMAT_##fmt)

#define SND_PCM_FMTBIT_S8			_SND_PCM_FMTBIT(S8)
#define SND_PCM_FMTBIT_U8			_SND_PCM_FMTBIT(U8)
#define SND_PCM_FMTBIT_S16_LE			_SND_PCM_FMTBIT(S16_LE)
#define SND_PCM_FMTBIT_S16_BE			_SND_PCM_FMTBIT(S16_BE)
#define SND_PCM_FMTBIT_U16_LE			_SND_PCM_FMTBIT(U16_LE)
#define SND_PCM_FMTBIT_U16_BE			_SND_PCM_FMTBIT(U16_BE)
#define SND_PCM_FMTBIT_S24_LE			_SND_PCM_FMTBIT(S24_LE)
#define SND_PCM_FMTBIT_S24_BE			_SND_PCM_FMTBIT(S24_BE)
#define SND_PCM_FMTBIT_U24_LE			_SND_PCM_FMTBIT(U24_LE)
#define SND_PCM_FMTBIT_U24_BE			_SND_PCM_FMTBIT(U24_BE)
#define SND_PCM_FMTBIT_S32_LE			_SND_PCM_FMTBIT(S32_LE)
#define SND_PCM_FMTBIT_S32_BE			_SND_PCM_FMTBIT(S32_BE)
#define SND_PCM_FMTBIT_U32_LE			_SND_PCM_FMTBIT(U32_LE)
#define SND_PCM_FMTBIT_U32_BE			_SND_PCM_FMTBIT(U32_BE)
#define SND_PCM_FMTBIT_FLOAT_LE			_SND_PCM_FMTBIT(FLOAT_LE)
#define SND_PCM_FMTBIT_FLOAT_BE			_SND_PCM_FMTBIT(FLOAT_BE)
#define SND_PCM_FMTBIT_FLOAT64_LE		_SND_PCM_FMTBIT(FLOAT64_LE)
#define SND_PCM_FMTBIT_FLOAT64_BE		_SND_PCM_FMTBIT(FLOAT64_BE)
#define SND_PCM_FMTBIT_IEC958_SUBFRAME_LE	_SND_PCM_FMTBIT(IEC958_SUBFRAME_LE)
#define SND_PCM_FMTBIT_IEC958_SUBFRAME_BE	_SND_PCM_FMTBIT(IEC958_SUBFRAME_BE)
#define SND_PCM_FMTBIT_MU_LAW			_SND_PCM_FMTBIT(MU_LAW)
#define SND_PCM_FMTBIT_A_LAW			_SND_PCM_FMTBIT(A_LAW)
#define SND_PCM_FMTBIT_IMA_ADPCM		_SND_PCM_FMTBIT(IMA_ADPCM)
#define SND_PCM_FMTBIT_MPEG			_SND_PCM_FMTBIT(MPEG)
#define SND_PCM_FMTBIT_GSM			_SND_PCM_FMTBIT(GSM)
#define SND_PCM_FMTBIT_SPECIAL			_SND_PCM_FMTBIT(SPECIAL)
#define SND_PCM_FMTBIT_S24_3LE			_SND_PCM_FMTBIT(S24_3LE)
#define SND_PCM_FMTBIT_U24_3LE			_SND_PCM_FMTBIT(U24_3LE)
#define SND_PCM_FMTBIT_S24_3BE			_SND_PCM_FMTBIT(S24_3BE)
#define SND_PCM_FMTBIT_U24_3BE			_SND_PCM_FMTBIT(U24_3BE)
#define SND_PCM_FMTBIT_S20_3LE			_SND_PCM_FMTBIT(S20_3LE)
#define SND_PCM_FMTBIT_U20_3LE			_SND_PCM_FMTBIT(U20_3LE)
#define SND_PCM_FMTBIT_S20_3BE			_SND_PCM_FMTBIT(S20_3BE)
#define SND_PCM_FMTBIT_U20_3BE			_SND_PCM_FMTBIT(U20_3BE)
#define SND_PCM_FMTBIT_S18_3LE			_SND_PCM_FMTBIT(S18_3LE)
#define SND_PCM_FMTBIT_U18_3LE			_SND_PCM_FMTBIT(U18_3LE)
#define SND_PCM_FMTBIT_S18_3BE			_SND_PCM_FMTBIT(S18_3BE)
#define SND_PCM_FMTBIT_U18_3BE			_SND_PCM_FMTBIT(U18_3BE)
#define SND_PCM_FMTBIT_G723_24			_SND_PCM_FMTBIT(G723_24)
#define SND_PCM_FMTBIT_G723_24_1B		_SND_PCM_FMTBIT(G723_24_1B)
#define SND_PCM_FMTBIT_G723_40			_SND_PCM_FMTBIT(G723_40)
#define SND_PCM_FMTBIT_G723_40_1B		_SND_PCM_FMTBIT(G723_40_1B)
#define SND_PCM_FMTBIT_DSD_U8			_SND_PCM_FMTBIT(DSD_U8)
#define SND_PCM_FMTBIT_DSD_U16_LE		_SND_PCM_FMTBIT(DSD_U16_LE)
#define SND_PCM_FMTBIT_RAW			_SND_PCM_FMTBIT(RAW)

#define SND_PCM_FMTBIT_S16			SND_PCM_FMTBIT_S16_LE
#define SND_PCM_FMTBIT_U16			SND_PCM_FMTBIT_U16_LE
#define SND_PCM_FMTBIT_S24			SND_PCM_FMTBIT_S24_LE
#define SND_PCM_FMTBIT_U24			SND_PCM_FMTBIT_U24_LE
#define SND_PCM_FMTBIT_S32			SND_PCM_FMTBIT_S32_LE
#define SND_PCM_FMTBIT_U32			SND_PCM_FMTBIT_U32_LE
#define SND_PCM_FMTBIT_FLOAT			SND_PCM_FMTBIT_FLOAT_LE
#define SND_PCM_FMTBIT_FLOAT64			SND_PCM_FMTBIT_FLOAT64_LE
#define SND_PCM_FMTBIT_IEC958_SUBFRAME		SND_PCM_FMTBIT_IEC958_SUBFRAME_LE

typedef uint32_t snd_pcm_source_t;
/* i2s sources */
#define SND_PCM_SOURCE_AUDPAD			0
#define SND_PCM_SOURCE_HDMIRX			1
/* spo sources */
#define SND_SPO_SOURCE_I2SODMA			0
#define SND_SPO_SOURCE_SPODMA			1
#define SND_SPO_SOURCE_HDMI_RX			2

typedef uint32_t snd_pcm_dest_t;
#define SND_PCM_DEST_DMA			0
#define SND_PCM_DEST_BYPASS			1

#define SND_PCM_SOURCEBIT_AUDPAD		(1 << SND_PCM_SOURCE_AUDPAD)
#define SND_PCM_SOURCEBIT_HDMIRX		(1 << SND_PCM_SOURCE_HDMIRX)

#define SPDIF_BYPASS_OFF		(0)	//!< spdif output from i2so-dma via /dev/spo or multi snd dev output, refer to AUDSINK_SND_DEVBIT_<xxx>
#define SPDIF_BYPASS_RAW		(1)	//!< spdif output from spdif-dma in raw data format, spdif output only (no i2so output)
#define SPDIF_BYPASS_PCM		(2)	//!< spdif output from spdif-dma in pcm data format, spdif output only (no i2so output)
#define SPDIF_BYPASS_RAW_USE_PREAMBLE	(SPDIF_BYPASS_RAW)	//!< IEC61937 frame, contains header and encapsuled frame, contains 16-bit words named Pa, Pb, Pc and Pd
#define SPDIF_BYPASS_RAW_NO_PREAMBLE	(3)	//!< spdif output from spdif-dma in raw data format without header, for wav-dts to use pcm mode, spdif output only (no i2so output)

enum {
	SND_PCM_ALIGN_LEFT = 0,
	SND_PCM_ALIGN_RIGHT,
};

struct snd_capability {
	/* SND_PCM_RATE_* */
	uint32_t rates;

	/* SND_PCM_FMTBIT_* */
	uint64_t formats;
};

struct snd_pcm_params {
	/* SND_PCM_ACCESS_* */
	snd_pcm_access_t access;

	/* SND_PCM_FORMAT_* */
	snd_pcm_format_t format;

	/* HC_AVCODEC_ID_* */
	enum HCAVCodecID codec_id;

	/* rate in Hz, playback rate in Hz */
	unsigned int rate;

	/* SND_PCM_ALIGN_* */
	uint8_t align;

	/* SND_SYNC_MODE_* */
	uint8_t sync_mode;

	/*
	 * dmabuf size(bytes) = channels * period_size * periods
	 */

	uint8_t channels;

	/*
	 * 8/16/32
	 * Do not support SND_PCM_FMTBIT_*_3LE
	 */
	uint8_t bitdepth;

	/*
	 * Number of periods to auto-trigger start
	 */
	uint16_t start_threshold;

	/*
	 * pcm data source switch
	 */
	snd_pcm_source_t pcm_source;
	snd_pcm_dest_t pcm_dest;

	/*
	 * byte size of one period, must be 32 bytes aligned
	 */
	uint32_t period_size;
	uint32_t periods;

	uint32_t audio_flush_thres;
	/*
	 * data_burst info, refer to IEC 60958
	 * must be set when format is SND_PCM_FORMAT_RAW
	 *  - AC3  : 0x01
	 *  - DTS1 : 0x0B
	 *  - DTS2 : 0x0C
	 *  - DTS3 : 0x0D
	 */
	uint16_t spo_iec_pc;
	 /*
	 * use preamble or not: for wav-dts need to use pcm mode on some amplifier;
	 *  - bs mode (use preamble):     0x00
	 *  - pcm mode(not use preamble): 0x01
	 */
	uint16_t no_preamble;

	/*
	 * when set SPDIF_PCM_BYPASS mode, hdmi_tx source is from spo_dma
	 */
	uint32_t use_spo_dma;
	/*
	 * 0:master_mode; 1:slave_mode; default is master_mode
	*/
	int slave_mode;

	/* rate in Hz, original rate of source in Hz, 0 (or src_rate == rate): invalid */
	unsigned int src_rate;

	/* auto change to audio master if video delay has been setted */
	int auto_audio_master;
};

struct snd_xfer {
	void *data;
	snd_pcm_uframes_t frames;
	uint32_t tstamp_ms;
};

struct snd_hw_info {
	uint32_t dma_addr;
	uint32_t dma_size;
	struct snd_pcm_params pcm_params;
	uint32_t i2si_wr_idx_register;
	uint32_t i2si_rd_idx_register;
};

typedef enum SND_TWOTONE_MODE {
	SND_TWOTONE_MODE_STANDARD = 0,
	SND_TWOTONE_MODE_MUSIC,
	SND_TWOTONE_MODE_MOVIE,
	SND_TWOTONE_MODE_SPORT,
	SND_TWOTONE_MODE_USER,
} snd_twotone_mode_e;

typedef enum SND_EQ6_MODE {
	SND_EQ6_MODE_OFF = 0,
	SND_EQ6_MODE_NORMAL,
	SND_EQ6_MODE_CLASSIC,
	SND_EQ6_MODE_JAZZ,
	SND_EQ6_MODE_ROCK,
	SND_EQ6_MODE_POP,
} snd_eq6_mode_e;

struct snd_twotone {
	int onoff;		/* 0: off; 1: on */

	snd_twotone_mode_e tt_mode;

	/*
	 * user specify param is valid only when tt_mode is SND_TWOTONE_MODE_USER
	 */
	int bass_index;		/* range: [ -10, 10 ] */
	int treble_index;	/* range: [ -10, 10 ] */
};

struct snd_lr_balance {
	int onoff;		/* 0: off; 1: on */
	int lr_balance_index;	/* range: [ -100, 100 ] */
};

struct snd_audio_eq6 {
	int onoff;		/* 0: off; 1: on */
	snd_eq6_mode_e mode;
};

struct snd_cjc8988_input {
	char input_name[20];	/* the input name specified in DTS node of "/hcrtos/cjc8988" */
};

struct snd_eq_band_setting {
	int band;
	int cutoff; /* Hz */
	int q;
	int gain; /* +/- dB */
};

struct snd_drc_setting {
	float peak_dBFS;
	float gain_dBFS;
};

struct snd_limiter_setting {
	float threshold_dBFS;
	float target_dBFS;
};

/*
default volume map:
const uint8_t linear_vol_map[101] = {
0,   3,   5,   8,  10,  13,  16,  18,  21,  23,  26,  28,  31,
33,  36,  38,  41,  44,  46,  49,  51,  54,  56,  59,  61,  64,
66,  69,  72,  74,  77,  79,  82,  84,  87,  89,  92,  95,  97,
100, 102, 105, 107, 110, 112, 115, 117, 120, 123, 125, 128, 130,
133, 135, 138, 140, 143, 145, 148, 151, 153, 156, 158, 161, 163,
166, 168, 171, 173, 176, 179, 181, 184, 186, 189, 191, 194, 196,
199, 202, 204, 207, 209, 212, 214, 217, 219, 222, 224, 227, 230,
232, 235, 237, 240, 242, 245, 247, 250, 252, 255
};
 */
struct snd_volume_map {
	uint8_t volume[101];
};

#endif	/* _HCUAPI_SND_H_ */
