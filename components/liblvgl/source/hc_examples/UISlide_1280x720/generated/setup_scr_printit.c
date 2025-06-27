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


void setup_scr_printit(lv_ui *ui)
{
	//Write codes printit
	ui->printit = lv_obj_create(NULL);
	ui->g_kb_printit = lv_keyboard_create(ui->printit);
	lv_obj_add_event_cb(ui->g_kb_printit, kb_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_add_flag(ui->g_kb_printit, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_text_font(ui->g_kb_printit, &lv_font_simsun_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_size(ui->printit, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->printit, LV_SCROLLBAR_MODE_OFF);

	//Write style for printit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->printit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes printit_cont0
	ui->printit_cont0 = lv_obj_create(ui->printit);
	lv_obj_set_pos(ui->printit_cont0, 0, 0);
	lv_obj_set_size(ui->printit_cont0, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->printit_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for printit_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->printit_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->printit_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->printit_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->printit_cont0, lv_color_hex(0xd20000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->printit_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->printit_cont0, lv_color_hex(0xd20000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->printit_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->printit_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->printit_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->printit_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->printit_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes printit_btnprtitback
	ui->printit_btnprtitback = lv_btn_create(ui->printit);
	ui->printit_btnprtitback_label = lv_label_create(ui->printit_btnprtitback);
	lv_label_set_text(ui->printit_btnprtitback_label, "BACK");
	lv_label_set_long_mode(ui->printit_btnprtitback_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->printit_btnprtitback_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->printit_btnprtitback, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->printit_btnprtitback, 433, 502);
	lv_obj_set_size(ui->printit_btnprtitback, 356, 103);
	lv_obj_set_scrollbar_mode(ui->printit_btnprtitback, LV_SCROLLBAR_MODE_OFF);

	//Write style for printit_btnprtitback, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->printit_btnprtitback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->printit_btnprtitback, lv_color_hex(0xd20000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->printit_btnprtitback, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->printit_btnprtitback, lv_color_hex(0xd20000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->printit_btnprtitback, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->printit_btnprtitback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->printit_btnprtitback, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->printit_btnprtitback, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->printit_btnprtitback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->printit_btnprtitback, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->printit_btnprtitback, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->printit_btnprtitback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes printit_label2
	ui->printit_label2 = lv_label_create(ui->printit);
	lv_label_set_text(ui->printit_label2, "No internet connection");
	lv_label_set_long_mode(ui->printit_label2, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->printit_label2, 26, 386);
	lv_obj_set_size(ui->printit_label2, 1226, 79);
	lv_obj_set_scrollbar_mode(ui->printit_label2, LV_SCROLLBAR_MODE_OFF);

	//Write style for printit_label2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->printit_label2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->printit_label2, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->printit_label2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->printit_label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->printit_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes printit_printer
	ui->printit_printer = lv_img_create(ui->printit);
	lv_obj_add_flag(ui->printit_printer, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->printit_printer, &_printer2_alpha_160x145);
	lv_img_set_pivot(ui->printit_printer, 0,0);
	lv_img_set_angle(ui->printit_printer, 0);
	lv_obj_set_pos(ui->printit_printer, 410, 184);
	lv_obj_set_size(ui->printit_printer, 160, 145);
	lv_obj_set_scrollbar_mode(ui->printit_printer, LV_SCROLLBAR_MODE_OFF);

	//Write style for printit_printer, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->printit_printer, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes printit_imgnotit
	ui->printit_imgnotit = lv_img_create(ui->printit);
	lv_obj_add_flag(ui->printit_imgnotit, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->printit_imgnotit, &_no_internet_alpha_63x63);
	lv_img_set_pivot(ui->printit_imgnotit, 0,0);
	lv_img_set_angle(ui->printit_imgnotit, 0);
	lv_obj_set_pos(ui->printit_imgnotit, 577, 163);
	lv_obj_set_size(ui->printit_imgnotit, 63, 63);
	lv_obj_set_scrollbar_mode(ui->printit_imgnotit, LV_SCROLLBAR_MODE_OFF);

	//Write style for printit_imgnotit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->printit_imgnotit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes printit_cloud
	ui->printit_cloud = lv_img_create(ui->printit);
	lv_obj_add_flag(ui->printit_cloud, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->printit_cloud, &_cloud_alpha_146x105);
	lv_img_set_pivot(ui->printit_cloud, 0,0);
	lv_img_set_angle(ui->printit_cloud, 0);
	lv_obj_set_pos(ui->printit_cloud, 687, 79);
	lv_obj_set_size(ui->printit_cloud, 146, 105);
	lv_obj_set_scrollbar_mode(ui->printit_cloud, LV_SCROLLBAR_MODE_OFF);

	//Write style for printit_cloud, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->printit_cloud, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->printit);

	
}
