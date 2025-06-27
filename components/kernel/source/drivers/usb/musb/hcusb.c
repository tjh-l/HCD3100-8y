#define LOG_TAG "hcusb"
#define ELOG_OUTPUT_LVL ELOG_LVL_INFO

#include <stdio.h>
#include <kernel/elog.h>
#include <kernel/drivers/hcusb.h>

#ifdef CONFIG_USB_PHY_SETTING
void hcusb_phy_setting(uint8_t usb_port)
{   
    if(USB_PORT_0 == usb_port) {
        /* 配置usb physetting */
        *((volatile uint8_t*)0xB8845100) = 0x1f; // [2:0] always enable pre-emphasis
        *((volatile uint8_t*)0xB8845102) = 0x56; // [6:4] HS eye tuning
        *((volatile uint8_t*)0xB8845103) = 0xe0; // [6:2] odt, default:0xd4,0xfc:fastest rise time; 0xc0:slowest rise time. bit7 default is 1
        *((volatile uint8_t*)0xB8845105) = 0xbc; // [4:2] TX HS pre_emphasize stength
        log_i("usb#0 phy setting\n");
    }
    else if(USB_PORT_1 == usb_port){
        /* 配置usb physetting */
        *((volatile uint8_t*)0xB8845000) = 0x1f; // [2:0] always enable pre-emphasis
        *((volatile uint8_t*)0xB8845002) = 0x56; // [6:4] HS eye tuning
        *((volatile uint8_t*)0xB8845003) = 0xe0; // [6:2] odt, default:0xd4,0xfc:fastest rise time; 0xc0:slowest rise time. bit7 default is 1
        *((volatile uint8_t*)0xB8845005) = 0xbc; // [4:2] TX HS pre_emphasize stength
        log_i("usb#1 phy setting\n");
    }
}
#endif


#ifdef CONFIG_USB_GADGET_VBUS_DETECT
#include <hcuapi/sys-blocking-notify.h>
#include <kernel/notify.h>
#include <linux/timer.h>

#define  USB_GADGET_PERIOD  300

static struct timer_list g_detect_timer;
static bool is_usbg_connect_status = false;

static bool is_usbg_connecting(void)
{
    // !note: 硬件上需要将VBUS pin连接到SOC,
    //   这里的检测原理是通过读取USB phy寄存器内部的PHY状态,
    //   进而获知是否有 usb host 插入
    if((*(uint8_t *)0xB8844381 & 0x18) == 0x18)
        return true;
    else 
        return false; 
}

static void __usb_detect_handler(unsigned long __data)
{
    if(!is_usbg_connect_status & is_usbg_connecting() ) {
        printf("==> usb#0 is connected now\n");
        is_usbg_connect_status = true;
        sys_notify_event(USB_GADGET_NOTIFY_CONNECT, NULL);
    } else if (is_usbg_connect_status & !is_usbg_connecting()) {
        printf("==> usb#0 has been disconnected\n");
        is_usbg_connect_status = false;
        sys_notify_event(USB_GADGET_NOTIFY_DISCONNECT, NULL);
    }

    mod_timer(&g_detect_timer, 
            jiffies + msecs_to_jiffies(USB_GADGET_PERIOD));
}

void hcusb_gadget_detect_handler(uint8_t mode)
{
    if(mode == MUSB_PERIPHERAL){

        *(uint16_t *)(0xb8845020) = 0x8000; //configure phy vbus/avalid/bvalid =1

        setup_timer(&g_detect_timer, 
                __usb_detect_handler, mode);
        mod_timer(&g_detect_timer, 
                jiffies + msecs_to_jiffies(USB_GADGET_PERIOD));
    }else if(mode == MUSB_HOST) {
        if(g_detect_timer.function) {
            del_timer(&g_detect_timer);
            init_timer(&g_detect_timer);
        }
    }
}

#endif


