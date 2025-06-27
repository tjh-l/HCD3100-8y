#ifndef _BOOTLOADER_COMMON_H
#define _BOOTLOADER_COMMON_H

int mtdloadraw(int argc, char **argv);
int mtdloaduImage(int argc, char **argv);

int showlogo(int argc, char *argv[]);
int osdlogo(int argc, char *argv[]);
void wait_show_logo(void);
void wait_show_logo_finish_feed(void);
void stop_show_logo(void);

int open_lcd_backlight(int argc, char *argv[]);

void open_boot_lcd_init(int argc, char *argv[]);

int open_pq_start(int argc, char *argv[]);

int boot_enter_standby(int argc, char *argv[]);

int is_low_battery(int quick_check);
const char *get_low_battery_popup_fpath(void);

#endif

