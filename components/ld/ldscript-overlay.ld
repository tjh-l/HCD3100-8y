__overlay_audio_start = .;
OVERLAY : NOCROSSREFS {
	.overlay_mp3 {
#if defined(__OVERLAY_MP3__)
		*/libmp3nddec.a:*(.text*)
		*/libmp3nddec.a:*(.rodata*)
		*/libmp3.a:*(.text*)
		*/libmp3.a:*(.rodata*)
#endif
	}

	.overlay_aac {
#if defined(__OVERLAY_AAC__)
		*/libaacdec.a:*(.text*)
		*/libaacdec.a:*(.rodata*)
		*/libaacmips74dec.a:*(.text*)
		*/libaacmips74dec.a:*(.rodata*)
#endif
	}

	.overlay_aaceld {
#if defined(__OVERLAY_AACELD__)
		*/libaaceld.a:*(.text*)
		*/libaaceld.a:*(.rodata*)
		*/libaaceldec.a:*(.text*)
		*/libaaceldec.a:*(.rodata*)
#endif
	}

	.overlay_ac3 {
#if defined(__OVERLAY_AC3__)
		*/libac3dec.a:*(.text*)
		*/libac3dec.a:*(.rodata*)
#endif
	}

	.overlay_eac3 {
#if defined(__OVERLAY_EAC3__)
		*/libeac3dec.a:*(.text*)
		*/libeac3dec.a:*(.rodata*)
#endif
	}

	.overlay_pcm {
#if defined(__OVERLAY_PCM__)
		*/libpcmdec.a:*(.text*)
		*/libpcmdec.a:*(.rodata*)
#endif
	}

	.overlay_flac {
#if defined(__OVERLAY_FLAC__)
		*/libflacdec.a:*(.text*)
		*/libflacdec.a:*(.rodata*)
		*/libflac.a:*(.text*)
		*/libflac.a:*(.rodata*)
#endif
	}

	.overlay_vorbis {
#if defined(__OVERLAY_VORBIS__)
		*/libvorbisdec.a:*(.text*)
		*/libvorbisdec.a:*(.rodata*)
		*/libtremor.a:*(.text*)
		*/libtremor.a:*(.rodata*)
#endif
	}

	.overlay_wma {
#if defined(__OVERLAY_WMA__)
		*/libwmadec.a:*(.text*)
		*/libwmadec.a:*(.rodata*)
#endif
	}

	.overlay_wmapro {
#if defined(__OVERLAY_WMAPRO__)
		*/libwmapro.a:*(.text*)
		*/libwmapro.a:*(.rodata*)
		*/libwmaprodec.a:*(.text*)
		*/libwmaprodec.a:*(.rodata*)
#endif
	}

	.overlay_opus {
#if defined(__OVERLAY_OPUS__)
		*/libopusdec.a:*(.text*)
		*/libopusdec.a:*(.rodata*)
		*/libopus.a:*(.text*)
		*/libopus.a:*(.rodata*)
#endif
	}

	.overlay_ra {
#if defined(__OVERLAY_RA__)
		*/libradec.a:*(.text*)
		*/libradec.a:*(.rodata*)
		*/libra.a:*(.text*)
		*/libra.a:*(.rodata*)
#endif
	}

	.overlay_ape {
#if defined(__OVERLAY_APE__)
		*/libapedec.a:*(.text*)
		*/libapedec.a:*(.rodata*)
#endif
	}

	.overlay_amr {
#if defined(__OVERLAY_AMR__)
		*/libamrdec.a:*(.text*)
		*/libamrdec.a:*(.rodata*)
		*/libopencore-amrnb.a:*(.text*)
		*/libopencore-amrnb.a:*(.rodata*)
		*/libopencore-amrwb.a:*(.text*)
		*/libopencore-amrwb.a:*(.rodata*)
#endif
	}

	.overlay_pcmdvd {
#if defined(__OVERLAY_PCMDVD__)
		*/libpcmdvddec.a:*(.text*)
		*/libpcmdvddec.a:*(.rodata*)
#endif
	}

	.overlay_alac {
#if defined(__OVERLAY_ALAC__)
		*/libalacdec.a:*(.text*)
		*/libalacdec.a:*(.rodata*)
		*/libalac.a:*(.text*)
		*/libalac.a:*(.rodata*)
#endif
	}
}

__overlay_video_start = .;
OVERLAY : NOCROSSREFS {
	.overlay_mpeg2 {
#if defined(__OVERLAY_MPEG2__)
		*/libviddrv_mpeg2dec.a:*(.text*)
		*/libviddrv_mpeg2dec.a:*(.rodata*)
#endif
	}

	.overlay_mpeg4 {
#if defined(__OVERLAY_MPEG4__)
		*/libviddrv_mpeg4dec.a:*(.text*)
		*/libviddrv_mpeg4dec.a:*(.rodata*)
#endif
	}

	.overlay_image {
#if defined(__OVERLAY_IMAGE__)
		*/libviddrv_imagedec.a:*(.text*)
		*/libviddrv_imagedec.a:*(.rodata*)
#endif
	}

	.overlay_h264 {
#if defined(__OVERLAY_H264__)
		*/libviddrv_h264dec.a:*(.text*)
		*/libviddrv_h264dec.a:*(.rodata*)
#endif
	}

	.overlay_rv {
#if defined(__OVERLAY_RV__)
		*/libviddrv_rvdec.a:*(.text*)
		*/libviddrv_rvdec.a:*(.rodata*)
#endif
	}

	.overlay_vc1 {
#if defined(__OVERLAY_VC1__)
		*/libviddrv_vc1dec.a:*(.text*)
		*/libviddrv_vc1dec.a:*(.rodata*)
#endif
	}

	.overlay_vp8 {
#if defined(__OVERLAY_VP8__)
		*/libviddrv_vp8dec.a:*(.text*)
		*/libviddrv_vp8dec.a:*(.rodata*)
#endif
	}
}

__overlay_wifi_start = .;
OVERLAY : NOCROSSREFS {
	.overlay_rtl8188fu {
#if defined(__OVERLAY_RTL8188FU__)
		*/librtl8188fu.a:*(.text*)
		*/librtl8188fu.a:*(.rodata*)
#endif
	}

	.overlay_rtl8188eu {
#if defined(__OVERLAY_RTL8188EU__)
		*/librtl8188eu.a:*(.text*)
		*/librtl8188eu.a:*(.rodata*)
#endif
	}

	.overlay_rtl8723as {
#if defined(__OVERLAY_RTL8723AS__)
		*/librtl8723as.a:*(.text*)
		*/librtl8723as.a:*(.rodata*)
#endif
	}

	.overlay_rtl8723bs {
#if defined(__OVERLAY_RTL8723BS__)
		*/librtl8723bs.a:*(.text*)
		*/librtl8723bs.a:*(.rodata*)
#endif
	}

	.overlay_rtl8733bu {
#if defined(__OVERLAY_RTL8733BU__)
		*/librtl8733bu.a:*(.text*)
		*/librtl8733bu.a:*(.rodata*)
#endif
	}

	.overlay_rtl8811cu {
#if defined(__OVERLAY_RTL8811CU__)
		*/librtl8811cu.a:*(.text*)
		*/librtl8811cu.a:*(.rodata*)
#endif
	}

	.overlay_rtl8822cs {
#if defined(__OVERLAY_RTL8822CS__)
		*/librtl8822cs.a:*(.text*)
		*/librtl8822cs.a:*(.rodata*)
#endif
	}

	.overlay_rtl8733bs {
#if defined(__OVERLAY_RTL8733BS__)
		*/librtl8733bs.a:*(.text*)
		*/librtl8733bs.a:*(.rodata*)
#endif
	}
}

.dynload_mediaplayer : {
PROVIDE (__dynload_mediaplayer_start = .);
#if defined(__DYNLOAD_MEDIAPLAYER__)
	*/libffplayer.a:*(.text*)
	*/libffplayer.a:*(.rodata*)
	*/libavcodec.a:*(.text*)
	*/libavcodec.a:*(.rodata*)
	*/libavformat.a:*(.text*)
	*/libavformat.a:*(.rodata*)
	*/libswscale.a:*(.text*)
	*/libswscale.a:*(.rodata*)
	*/libavutil.a:*(.text*)
	*/libavutil.a:*(.rodata*)
#endif
PROVIDE (__load_start_dynload_mediaplayer = LOADADDR (.dynload_mediaplayer));
PROVIDE (__load_stop_dynload_mediaplayer = (LOADADDR (.dynload_mediaplayer) + SIZEOF (.dynload_mediaplayer)));
}
