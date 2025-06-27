#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bluetooth.h>

int __attribute__((weak)) bluetooth_init(const char *uart_path, bluetooth_callback_t callback)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_deinit(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_poweron(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_poweroff(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_scan(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_stop_scan(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_connect(unsigned char *mac)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_is_connected(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_disconnect(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_set_music_vol(unsigned char val)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_memory_connection(unsigned char value)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_set_gpio_backlight(unsigned char value)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_set_gpio_mutu(unsigned char value)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_set_cvbs_aux_mode(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_set_cvbs_fiber_mode(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_del_all_device(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_del_list_device(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_set_connection_cvbs_aux_mode(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_set_connection_cvbs_fiber_mode(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_ir_key_init(bluetooth_ir_control_t control)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_power_on_to_rx(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_factory_reset(void)
{
    printf("[weak] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) bluetooth_ioctl(int cmd, ...)
{
    printf("[weak] %s\n", __func__);
    return 0;
}
