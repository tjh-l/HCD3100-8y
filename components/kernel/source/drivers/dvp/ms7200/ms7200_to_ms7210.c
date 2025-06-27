/**
******************************************************************************
* @readme
*
* this file show an example for how to use ms7200 & ms7210 together on demo board
*
* if you only need to use ms7200,
* replace "ms7210_init()" and "ms7210_media_service()" related code with your own system.
*
* if you only need to use ms7210,
* replace "ms7200_init()" and "ms7200_media_service()" related code with your own system.
******************************************************************************/
//#include "mculib_common.h"
#include "ms7200.h"
#include <stdbool.h>
#include <hcuapi/dis.h>
#include <kernel/vfs.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
//#include "ms7210.h"

static UINT16 g_u16_tmds_clk = 0;
static BOOL g_b_rxphy_status = TRUE;
static BOOL g_b_input_valid = FALSE;
static UINT8 g_u8_rx_stable_timer_count = 0;
#define RX_STABLE_TIMEOUT (2)
static VIDEOTIMING_T g_t_hdmirx_timing;
static HDMI_CONFIG_T g_t_hdmirx_infoframe;
//static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_RGB, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_HSVSDE };
//static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_EMBEDDED };
static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_8_12BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_EMBEDDED };

extern VOID ms7200drv_dvout_data_uv_flip(VOID);
extern VOID ms7200drv_dvout_data_swap_all(VOID);


#if 0
static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_RGB, DVIN_BW_MODE_16_20_24BIT, DVIN_SQ_MODE_NONSEQ, DVIN_DR_MODE_SDR, DVIN_SY_MODE_HSVSDE };
static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_YUV422, DVIN_BW_MODE_16_20_24BIT, DVIN_SQ_MODE_NONSEQ, DVIN_DR_MODE_SDR, DVIN_SY_MODE_EMBEDDED };
static BOOL g_b_output_valid = FALSE;
static VIDEOTIMING_T g_t_hdmitx_timing;
static HDMI_CONFIG_T g_t_hdmitx_infoframe;
static UINT8 g_u8_tx_stable_timer_count = 0;
#endif
#define TX_STABLE_TIMEOUT (3)

static UINT8 u8_sys_edid_default_buf[256] =
{
    // Explore Semiconductor, Inc. EDID Editor V2
    0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x4c,0x2d,0xff,0x0d,0x58,0x4d,0x51,0x30,
    0x1c,0x1c,0x01,0x03,0x80,0x3d,0x23,0x78,0x2a,0x5f,0xb1,0xa2,0x57,0x4f,0xa2,0x28,
    0x0f,0x50,0x54,0x20,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x1d,0x00,0x72,0x51,0xd0,0x1e,0x20,0x6e,0x28,
    0x55,0x00,0x80,0xd8,0x10,0x00,0x00,0x06,0x00,0x00,0x00,0xfd,0x00,0x18,0x4b,0x1e,
    0x5a,0x1e,0x00,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xfc,0x00,0x55,
    0x32,0x38,0x48,0x37,0x35,0x78,0x0a,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xff,
    0x00,0x48,0x54,0x50,0x4b,0x37,0x30,0x30,0x30,0x35,0x31,0x0a,0x20,0x20,0x01,0x51,
    0x02,0x03,0x21,0xf6,0x46,0x84,0x13,0x03,0x12,0x05,0x14,0x23,0x09,0x07,0x07,0x83,
    0x01,0x00,0x00,0x6d,0x03,0x0c,0x00,0x10,0x00,0x80,0x3c,0x20,0x10,0x60,0x01,0x02,
    0x03,0x01,0x1d,0x00,0xbc,0x52,0xd0,0x1e,0x20,0xb8,0x28,0x55,0x40,0x80,0xd8,0x10,
    0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,

};



VOID ms7200_init(VOID)
{
    printf("%s(%d) ms7200 chip connect = %d\n", __func__, __LINE__, ms7200_chip_connect_detect(0x56));

    g_b_input_valid = false;
    ms7200_hdmirx_init();
    //ms7200_hdmirx_hdcp_init(u8_rx_ksv_buf, u8_rx_key_buf);
    ms7200_dvout_init(&g_t_dvout_config, 0);
    //ms7200_dvout_clk_driver_set(3);
    ms7200_hdmirx_hpd_config(FALSE, u8_sys_edid_default_buf);
    //ms7200drv_dvout_data_uv_flip();
   // ms7200drv_dvout_data_swap_yc_channel();
    //ms7200_dvout_video_config(TRUE);
    //ms7200_dvout_audio_config(TRUE);
}

void sys_default_hdmi_video_config(void)
{
    //g_t_hdmi_infoframe.u8_hdmi_flag = TRUE;
    g_t_hdmirx_infoframe.u8_vic = 0;
    //g_t_hdmi_infoframe.u16_video_clk = 7425;
    g_t_hdmirx_infoframe.u8_clk_rpt = HDMI_X1CLK;
    g_t_hdmirx_infoframe.u8_scan_info = HDMI_OVERSCAN;
    g_t_hdmirx_infoframe.u8_aspect_ratio = HDMI_16X9;
    g_t_hdmirx_infoframe.u8_color_space = HDMI_RGB;
    g_t_hdmirx_infoframe.u8_color_depth = HDMI_COLOR_DEPTH_8BIT;
    g_t_hdmirx_infoframe.u8_colorimetry = HDMI_COLORIMETRY_709;
}

void sys_default_hdmi_vendor_specific_config(void)
{
    g_t_hdmirx_infoframe.u8_video_format = HDMI_NO_ADD_FORMAT;
    g_t_hdmirx_infoframe.u8_4Kx2K_vic = HDMI_4Kx2K_30HZ;
    g_t_hdmirx_infoframe.u8_3D_structure = HDMI_FRAME_PACKING;
}

void sys_default_hdmi_audio_config(void)
{
    g_t_hdmirx_infoframe.u8_audio_mode = HDMI_AUD_MODE_AUDIO_SAMPLE;
    g_t_hdmirx_infoframe.u8_audio_rate = HDMI_AUD_RATE_48K;
    g_t_hdmirx_infoframe.u8_audio_bits = HDMI_AUD_LENGTH_16BITS;
    g_t_hdmirx_infoframe.u8_audio_channels = HDMI_AUD_2CH;
    g_t_hdmirx_infoframe.u8_audio_speaker_locations = 0;
}

static int _abs(int i)
{
    /* compute absolute value of int argument */
    return (i < 0 ? -i : i);
}

static void switch_clk(bool b_from_pad)
{
    if (b_from_pad == true)
    {
        *(UINT32*)(0xB8800078) |= (1 << 16);
    }
    else
    {
        *(UINT32 *)(0xB8800078) &= ~(1 << 16);
    }
    return;
}
static int close_video_output(void)
{
    struct dis_win_onoff winon = { 0 };
    int fd;
    printf("CLOSE MP\n");
    winon.layer = DIS_LAYER_MAIN;
    winon.distype = DIS_TYPE_HD;
    winon.on = 0;
    fd = open("/dev/dis" , O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }
    ioctl(fd , DIS_SET_WIN_ONOFF , &winon);
    close(fd);
    return 0;
}
bool ms7200_media_service(VOID)
{
    UINT8 u8_rxphy_status = 0;
    UINT32 u32_int_status = 0;
    HDMI_CONFIG_T t_hdmi_infoframe;

    //detect 5v to pull up/down hpd
    BOOL source_connect_detect = ms7200_hdmirx_source_connect_detect();
    //printf("%s(%d) source_connect_detect=%s\n", __func__, __LINE__, source_connect_detect ? "TRUE" : "FALSE");
    ms7200_hdmirx_hpd_config(source_connect_detect, NULL);
    if (source_connect_detect == false)
    {
        return false;
    }
    //reconfig rxphy when input clk change
    u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_HDMI, FREQ_LOCK_ISTS | FREQ_UNLOCK_ISTS | CLK_CHANGE_ISTS, TRUE);
    //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
    if(u32_int_status)
    {
        printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
        ms7200_hdmirx_input_infoframe_get(&t_hdmi_infoframe);
        //printf("%s(%d) t_hdmi_infoframe.u8_video_format=%x\n", __func__, __LINE__, t_hdmi_infoframe.u8_video_format);
        //printf("%s(%d) t_hdmi_infoframe.u8_vic=%d\n", __func__, __LINE__, t_hdmi_infoframe.u8_vic);
        //printf("%s(%d) t_hdmi_infoframe.u8_4Kx2K_vic=%d\n", __func__, __LINE__, t_hdmi_infoframe.u8_4Kx2K_vic);
        //printf("%s(%d) t_hdmi_infoframe.u8_color_space=%d\n" , __func__ , __LINE__ , t_hdmi_infoframe.u8_color_space);
        if(u32_int_status & CLK_CHANGE_ISTS)
        {
            //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            if(_abs(ms7200_hdmirx_input_clk_get() - g_u16_tmds_clk) > 1000)
            {
                printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
                g_b_rxphy_status = FALSE;
            }
        }
        else if((u32_int_status & (FREQ_LOCK_ISTS | FREQ_UNLOCK_ISTS)) == FREQ_LOCK_ISTS)
        {
            //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            g_b_rxphy_status = TRUE;
        }
        printf("%s(%d) g_b_rxphy_status=%d\n", __func__, __LINE__, g_b_rxphy_status);
        if(!g_b_rxphy_status)
        {
            close_video_output();
            switch_clk(true);
            printf("SET CLK SRC OK\n");
            //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            ms7200_dvout_audio_config(FALSE);
            ms7200_dvout_video_config(FALSE);
            u8_rxphy_status = ms7200_hdmirx_rxphy_config(&g_u16_tmds_clk);
            g_b_rxphy_status = u8_rxphy_status ? (u8_rxphy_status & 0x01) : TRUE;
            g_b_input_valid = FALSE;
            g_u8_rx_stable_timer_count = 0;
            //printf("g_u16_tmds_clk = %d\n", g_u16_tmds_clk);
            //printf("g_b_rxphy_status = %d\n", g_b_rxphy_status);
        }
    }
    if(g_u16_tmds_clk < 500)
    {
        //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
        return false;
    }

    //reset pdec when mdt change
    u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_MDT, MDT_STB_ISTS | MDT_USTB_ISTS, TRUE);
    //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
    if(u32_int_status)
    {
        //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
        if(!(g_b_input_valid && (u32_int_status == MDT_STB_ISTS)))
        {
            printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            ms7200_dvout_audio_config(FALSE);
            ms7200_dvout_video_config(FALSE);
            ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
            ms7200_hdmirx_core_reset(HDMI_RX_CTRL_PDEC | HDMI_RX_CTRL_AUD);
            ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
            g_u8_rx_stable_timer_count = 0;
            if(g_b_input_valid)
            {
                printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
                g_b_input_valid = FALSE;
                printf("timing unstable\n");
            }
        }
    }

    //printf("%s(%d) g_b_input_valid=%d\n", __func__, __LINE__, g_b_input_valid);
    //get input timing status when mdt stable
    if(!g_b_input_valid)
    {
        //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
        if(g_u8_rx_stable_timer_count < RX_STABLE_TIMEOUT)
        {
            printf("%s(%d) g_u8_rx_stable_timer_count=%d\n", __func__, __LINE__, g_u8_rx_stable_timer_count);
            g_u8_rx_stable_timer_count++;
            return false;
        }
        g_b_input_valid = ms7200_hdmirx_input_timing_get(&g_t_hdmirx_timing);
        printf("%s(%d) g_b_input_valid=%d\n", __func__, __LINE__, g_b_input_valid);
        if(g_b_input_valid)
        {
            printf("%s(%d) g_b_input_valid=%d\n", __func__, __LINE__, g_b_input_valid);
            u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
            ms7200_hdmirx_input_infoframe_get(&g_t_hdmirx_infoframe);
            g_t_hdmirx_infoframe.u16_video_clk = g_u16_tmds_clk;
            if(!(u32_int_status & AVI_RCV_ISTS))
            {
                printf("%s(%d) g_b_input_valid=%d\n", __func__, __LINE__, g_b_input_valid);
                sys_default_hdmi_video_config();
            }
            if(!(u32_int_status & AIF_RCV_ISTS))
            {
                printf("%s(%d) g_b_input_valid=%d\n", __func__, __LINE__, g_b_input_valid);
                sys_default_hdmi_vendor_specific_config();
            }
            if(!(u32_int_status & VSI_RCV_ISTS))
            {
                printf("%s(%d) g_b_input_valid=%d\n", __func__, __LINE__, g_b_input_valid);
                sys_default_hdmi_vendor_specific_config();
            }

            ms7200_hdmirx_video_config(&g_t_hdmirx_infoframe);
            ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
            ms7200_dvout_video_config(TRUE);
            ms7200_dvout_audio_config(TRUE);
            printf("timing stable\n");
            return true;
        }
        else
        {
            printf("timing error\n");
            switch_clk(true);
            return false;
        }
    }

    //reconfig system when input packet change
    u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
    //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
    if(u32_int_status & (AVI_CKS_CHG_ISTS | AIF_CKS_CHG_ISTS | VSI_CKS_CHG_ISTS))
    {
        printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
        ms7200_hdmirx_input_infoframe_get(&t_hdmi_infoframe);
        t_hdmi_infoframe.u16_video_clk = g_u16_tmds_clk;
        if(!(u32_int_status & AVI_RCV_ISTS))
        {
            printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            sys_default_hdmi_video_config();
        }
        if(!(u32_int_status & AIF_RCV_ISTS))
        {
            printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            sys_default_hdmi_vendor_specific_config();
        }
        if(!(u32_int_status & VSI_RCV_ISTS))
        {
            printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            sys_default_hdmi_vendor_specific_config();
        }
        if(memcmp(&t_hdmi_infoframe, &g_t_hdmirx_infoframe, sizeof(HDMI_CONFIG_T)) != 0)
        {
            printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            ms7200_dvout_audio_config(FALSE);
            ms7200_dvout_video_config(FALSE);
            ms7200_hdmirx_video_config(&t_hdmi_infoframe);
            ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
            ms7200_dvout_video_config(TRUE);
            ms7200_dvout_audio_config(TRUE);
            g_t_hdmirx_infoframe = t_hdmi_infoframe;
            printf("infoframe change\n");
            switch_clk(true);
            g_b_input_valid = false;
        }
    }

    //reconfig audio when acr change or audio fifo error
    if(u32_int_status & (ACR_CTS_CHG_ISTS | ACR_N_CHG_ISTS))
    {
        printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
        ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
        printf("acr change\n");
    }
    else
    {
        u32_int_status = ms7200_hdmirx_audio_fifo_status_get();
        //printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
        if(u32_int_status & AFIF_THS_PASS_STS)
        {
            printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
            if(u32_int_status & (AFIF_UNDERFL_STS | AFIF_OVERFL_STS))
            {
                printf("%s(%d) u32_int_status=%lu\n", __func__, __LINE__, u32_int_status);
                ms7200_hdmirx_audio_config(FALSE, g_u16_tmds_clk);
                printf("audio fifo reset\n");
            }
        }
    }

    return  g_b_input_valid;
    //printf("%s(%d)\n", __func__, __LINE__);
}
