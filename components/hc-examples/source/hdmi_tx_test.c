#include "dis_test.h"
#include <hcuapi/hdmi_tx.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "../../cmds/source/cec/hdmi_cec.h"

int hdmi_get_edid_all_video_res(int argc , char *argv[])
{
    int hdmi_fd = -1;
    struct hdmi_edidinfo edidinfo = { 0 };
    int i = 0;
    int ret = -1;

    (void)argc;
    (void)argv;

    hdmi_fd = open("/dev/hdmi" , O_RDWR);
    if(hdmi_fd < 0)
    {
        printf("open hdmi error\n");
        return -1;
    }
    else
    {
        ret = ioctl(hdmi_fd , HDMI_TX_GET_EDIDINFO , &edidinfo);
        if(ret < 0)
        {
            printf("HDMI_TX_GET_EDID_TVSYS error\n");
            close(hdmi_fd);
            return -1;
        }
        for(i = 0; i < edidinfo.num_tvsys; i++)
        {
            printf("i:%d\n" , edidinfo.tvsys[i]);
        }
        printf("num_tvsys:%d\n" , edidinfo.num_tvsys);
        printf("best res:%d\n" , edidinfo.best_tvsys);
		printf("manufacturer_name:%s\n" , edidinfo.prod_info.manufacturer_name);
		printf("monitor_name:%s\n" , edidinfo.prod_info.monitor_name);
    }

    close(hdmi_fd);

    return 0;
}

void hdmi_tx_process_cec_cmd(enum HDMI_CEC_CMD cec_cmd,unsigned char logical_address , struct hdmi_cec_info *cec_info)
{
    unsigned char *data = cec_info->data;
    printf("cec_cmd = %d logical_address=%d\n" , cec_cmd, logical_address);
    switch(cec_cmd)
    {
        case HDMI_CEC_CMD_SYSTEM_STANDBY:
        {
            data[0] = (logical_address << 4) | CEC_LA_TV;
            data[1] = OPCODE_SYSTEM_STANDBY;
            cec_info->data_len = 2;
            printf("CEC_CMD_SYSTEM_STANDBY\n");
            break;
        }
        case HDMI_CEC_CMD_IMAGE_VIEW_ON:
        {
            data[0] = (logical_address << 4) | CEC_LA_TV;
            data[1] = OPCODE_IMAGE_VIEW_ON;
            cec_info->data_len = 2;
            printf("OPCODE_IMAGE_VIEW_ON\n");
            break;
        }
        default:
        {
            printf("UN support CEC CMD:0x%x\n" , cec_cmd);
            break;
        }
    }
}

int hdmi_send_cec_cmd(int argc , char *argv[])
{
    int hdmi_fd = -1;
    int ret = -1;
    int opt;
    opterr = 0;
    optind = 0;
    enum HDMI_CEC_CMD cec_cmd = 0;
    unsigned char logical_address = 0;
    struct hdmi_cec_info cec_info = { 0 };

    while((opt = getopt(argc , argv , "c:l:")) != EOF)
    {
        switch(opt)
        {
            case 'c':
                cec_cmd = atoi(optarg);
                break;
            case 'l':
                logical_address = atoi(optarg);
                break;
            default:
                break;
        }
    }

    hdmi_tx_process_cec_cmd(cec_cmd, logical_address, &cec_info);
    hdmi_fd = open("/dev/hdmi" , O_RDWR);
    if(hdmi_fd < 0)
    {
        printf("open hdmi error\n");
        return -1;
    }
    else
    {
        ret = ioctl(hdmi_fd , HDMI_CEC_SET_LOGIC_ADDR , logical_address);
        ret = ioctl(hdmi_fd , HDMI_CEC_SEND_CMD , &cec_info);
        if(ret < 0)
        {
            printf("HDMI_TX_SEND_CEC_CMD error\n");
            close(hdmi_fd);
            return -1;
        }
    }

    close(hdmi_fd);

    return 0;
}



int hdmi_set_cec_onoff(int argc , char *argv[])
{
    int hdmi_fd = -1;
    int ret = -1;
    int onoff = 0;
    int opt;
    opterr = 0;
    optind = 0;

    while((opt = getopt(argc , argv , "e:")) != EOF)
    {
        switch(opt)
        {
            case 'e':
            {
                onoff = atoi(optarg);
                printf("onoff = %d\n" , onoff);
                break;
            }
            default:
                break;
        }
    }

    hdmi_fd = open("/dev/hdmi" , O_RDWR);
    if(hdmi_fd < 0)
    {
        printf("open hdmi error\n");
        return -1;
    }
    else
    {
        ret = ioctl(hdmi_fd , HDMI_CEC_SET_ONOFF , onoff);
        if(ret < 0)
        {
            printf("HDMI_TX_SET_CEC_ONOFF error\n");
            close(hdmi_fd);
            return -1;
        }
    }

    close(hdmi_fd);

    return 0;
}



int hdmi_get_cec_onoff(int argc , char *argv[])
{
    int hdmi_fd = -1;
    int ret = -1;
    uint32_t onoff = 0;
    int opt;
    opterr = 0;
    optind = 0;

    while((opt = getopt(argc , argv , "e:")) != EOF)
    {
        switch(opt)
        {
            case 'e':
                onoff = atoi(optarg);
                break;
            default:
                break;
        }
    }

    hdmi_fd = open("/dev/hdmi" , O_RDWR);
    if(hdmi_fd < 0)
    {
        printf("open hdmi error\n");
        return -1;
    }
    else
    {
        ret = ioctl(hdmi_fd , HDMI_CEC_GET_ONOFF , &onoff);
        if(ret < 0)
        {
            printf("HDMI_TX_SET_CEC_ONOFF error\n");
            close(hdmi_fd);
            return -1;
        }
        printf("onoff = %ld\n" , onoff);
    }

    close(hdmi_fd);

    return 0;
}



int hdmi_send_logical_addr(int argc , char *argv[])
{
    int hdmi_fd = -1;
    int ret = -1;
    int opt;
    opterr = 0;
    optind = 0;
    unsigned char logical_address = 0;

    while((opt = getopt(argc , argv , "l:")) != EOF)
    {
        switch(opt)
        {
            case 'l':
                logical_address = atoi(optarg);
                break;
            default:
                break;
        }
    }

    hdmi_fd = open("/dev/hdmi" , O_RDWR);
    if(hdmi_fd < 0)
    {
        printf("open hdmi error\n");
        return -1;
    }
    else
    {
        ret = ioctl(hdmi_fd , HDMI_CEC_SET_LOGIC_ADDR , logical_address);
        if(ret < 0)
        {
            printf("HDMI_TX_SEND_CEC_CMD error\n");
            close(hdmi_fd);
            return -1;
        }
    }

    close(hdmi_fd);

    return 0;
}
