/*
* Copyright 2023 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"


void setup_scr_setup(lv_ui *ui)
{
	//Write codes setup
	ui->setup = lv_obj_create(NULL);
	ui->g_kb_setup = lv_keyboard_create(ui->setup);
	lv_obj_add_event_cb(ui->g_kb_setup, kb_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_add_flag(ui->g_kb_setup, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_text_font(ui->g_kb_setup, &lv_font_simsun_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_size(ui->setup, 1024, 600);
	lv_obj_set_scrollbar_mode(ui->setup, LV_SCROLLBAR_MODE_OFF);

	//Write style for setup, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->setup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes setup_cont0
	ui->setup_cont0 = lv_obj_create(ui->setup);
	lv_obj_set_pos(ui->setup_cont0, 0, 0);
	lv_obj_set_size(ui->setup_cont0, 1024, 600);
	lv_obj_set_scrollbar_mode(ui->setup_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for setup_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->setup_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->setup_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->setup_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->setup_cont0, lv_color_hex(0xd20000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->setup_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->setup_cont0, lv_color_hex(0xd20000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->setup_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->setup_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->setup_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->setup_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->setup_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes setup_btnsetback
	ui->setup_btnsetback = lv_btn_create(ui->setup);
	ui->setup_btnsetback_label = lv_label_create(ui->setup_btnsetback);
	lv_label_set_text(ui->setup_btnsetback_label, "BACK");
	lv_label_set_long_mode(ui->setup_btnsetback_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->setup_btnsetback_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->setup_btnsetback, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->setup_btnsetback, 343, 432);
	lv_obj_set_size(ui->setup_btnsetback, 285, 86);
	lv_obj_set_scrollbar_mode(ui->setup_btnsetback, LV_SCROLLBAR_MODE_OFF);

	//Write style for setup_btnsetback, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->setup_btnsetback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->setup_btnsetback, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->setup_btnsetback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->setup_btnsetback, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->setup_btnsetback, 110, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->setup_btnsetback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->setup_btnsetback, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->setup_btnsetback, &lv_font_simsun_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->setup_btnsetback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes setup_label2
	ui->setup_label2 = lv_label_create(ui->setup);
	lv_label_set_text(ui->setup_label2, "You have no permission to change the settings");
	lv_label_set_long_mode(ui->setup_label2, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->setup_label2, 21, 322);
	lv_obj_set_size(ui->setup_label2, 981, 66);
	lv_obj_set_scrollbar_mode(ui->setup_label2, LV_SCROLLBAR_MODE_OFF);

	//Write style for setup_label2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->setup_label2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->setup_label2, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->setup_label2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->setup_label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->setup_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes setup_printer
	ui->setup_printer = lv_img_create(ui->setup);
	lv_obj_add_flag(ui->setup_printer, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->setup_printer, &_printer2_alpha_128x121);
	lv_img_set_pivot(ui->setup_printer, 0,0);
	lv_img_set_angle(ui->setup_printer, 0);
	lv_obj_set_pos(ui->setup_printer, 328, 154);
	lv_obj_set_size(ui->setup_printer, 128, 121);
	lv_obj_set_scrollbar_mode(ui->setup_printer, LV_SCROLLBAR_MODE_OFF);

	//Write style for setup_printer, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->setup_printer, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes setup_img
	ui->setup_img = lv_img_create(ui->setup);
	lv_obj_add_flag(ui->setup_img, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->setup_img, &_no_internet_alpha_53x53);
	lv_img_set_pivot(ui->setup_img, 0,0);
	lv_img_set_angle(ui->setup_img, 0);
	lv_obj_set_pos(ui->setup_img, 462, 136);
	lv_obj_set_size(ui->setup_img, 53, 53);
	lv_obj_set_scrollbar_mode(ui->setup_img, LV_SCROLLBAR_MODE_OFF);

	//Write style for setup_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->setup_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes setup_cloud
	ui->setup_cloud = lv_img_create(ui->setup);
	lv_obj_add_flag(ui->setup_cloud, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->setup_cloud, &_cloud_alpha_117x88);
	lv_img_set_pivot(ui->setup_cloud, 0,0);
	lv_img_set_angle(ui->setup_cloud, 0);
	lv_obj_set_pos(ui->setup_cloud, 550, 66);
	lv_obj_set_size(ui->setup_cloud, 117, 88);
	lv_obj_set_scrollbar_mode(ui->setup_cloud, LV_SCROLLBAR_MODE_OFF);

	//Write style for setup_cloud, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->setup_cloud, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->setup);

	
}
