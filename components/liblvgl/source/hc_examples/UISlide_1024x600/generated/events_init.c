/*
* Copyright 2023 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"


#include "custom.h"
#include "custom.h"
static void home_imgbtnscan_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_RELEASED:
	{
		
		break;
	}
	case LV_EVENT_CLICKED:
	{
		guider_load_screen(SCR_LOADER);
	add_loader(load_scan);
		break;
	}
	default:
		break;
	}
}
static void home_imgbtnset_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		guider_load_screen(SCR_LOADER);
	add_loader(load_setup);
	//guider_load_screen(SCR_LOADER);
	//add_loader(load_print);
		break;
	}
	default:
		break;
	}
}
static void home_imgbtncopy_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_RELEASED:
	{
		
		break;
	}
	case LV_EVENT_CLICKED:
	{
		guider_load_screen(SCR_LOADER);
	add_loader(load_copy);
		break;
	}
	default:
		break;
	}
}
static void home_imgbtnprt_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		guider_load_screen(SCR_LOADER);
	add_loader(load_print);
		break;
	}
	default:
		break;
	}
}
static void home_imgcopy_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_PRESSED:
	{
		guider_load_screen(SCR_LOADER);
	add_loader(load_copy);
		break;
	}
	default:
		break;
	}
}
static void home_imgscan_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_PRESSED:
	{
		guider_load_screen(SCR_LOADER);
	add_loader(load_scan);
		break;
	}
	default:
		break;
	}
}
static void home_imgprt_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_PRESSED:
	{
		guider_load_screen(SCR_LOADER);
	add_loader(load_print);
		break;
	}
	default:
		break;
	}
}
void events_init_home(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->home_imgbtnscan, home_imgbtnscan_event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui->home_imgbtnset, home_imgbtnset_event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui->home_imgbtncopy, home_imgbtncopy_event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui->home_imgbtnprt, home_imgbtnprt_event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui->home_imgcopy, home_imgcopy_event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui->home_imgscan, home_imgscan_event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui->home_imgprt, home_imgprt_event_handler, LV_EVENT_ALL, NULL);
}

void events_init(lv_ui *ui)
{

}
