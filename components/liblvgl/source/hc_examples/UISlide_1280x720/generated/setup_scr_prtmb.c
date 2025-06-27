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


void setup_scr_prtmb(lv_ui *ui)
{
	//Write codes prtmb
	ui->prtmb = lv_obj_create(NULL);
	lv_obj_set_size(ui->prtmb, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->prtmb, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtmb, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtmb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtmb_cont0
	ui->prtmb_cont0 = lv_obj_create(ui->prtmb);
	lv_obj_set_pos(ui->prtmb_cont0, 0, 0);
	lv_obj_set_size(ui->prtmb_cont0, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->prtmb_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtmb_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtmb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtmb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtmb_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtmb_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtmb_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtmb_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtmb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtmb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtmb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtmb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtmb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtmb_btnback
	ui->prtmb_btnback = lv_btn_create(ui->prtmb);
	ui->prtmb_btnback_label = lv_label_create(ui->prtmb_btnback);
	lv_label_set_text(ui->prtmb_btnback_label, "BACK");
	lv_label_set_long_mode(ui->prtmb_btnback_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prtmb_btnback_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prtmb_btnback, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prtmb_btnback, 455, 518);
	lv_obj_set_size(ui->prtmb_btnback, 356, 103);
	lv_obj_set_scrollbar_mode(ui->prtmb_btnback, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtmb_btnback, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtmb_btnback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtmb_btnback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtmb_btnback, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtmb_btnback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtmb_btnback, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->prtmb_btnback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->prtmb_btnback, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtmb_btnback, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtmb_btnback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtmb_btnback, lv_color_hex(0xf4ecec), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtmb_btnback, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtmb_btnback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtmb_label2
	ui->prtmb_label2 = lv_label_create(ui->prtmb);
	lv_label_set_text(ui->prtmb_label2, "Put your phone near to the printer");
	lv_label_set_long_mode(ui->prtmb_label2, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prtmb_label2, 26, 386);
	lv_obj_set_size(ui->prtmb_label2, 1226, 79);
	lv_obj_set_scrollbar_mode(ui->prtmb_label2, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtmb_label2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtmb_label2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtmb_label2, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prtmb_label2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtmb_label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtmb_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtmb_printer
	ui->prtmb_printer = lv_img_create(ui->prtmb);
	lv_obj_add_flag(ui->prtmb_printer, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->prtmb_printer, &_printer2_alpha_160x145);
	lv_img_set_pivot(ui->prtmb_printer, 0,0);
	lv_img_set_angle(ui->prtmb_printer, 0);
	lv_obj_set_pos(ui->prtmb_printer, 410, 184);
	lv_obj_set_size(ui->prtmb_printer, 160, 145);
	lv_obj_set_scrollbar_mode(ui->prtmb_printer, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtmb_printer, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prtmb_printer, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtmb_img
	ui->prtmb_img = lv_img_create(ui->prtmb);
	lv_obj_add_flag(ui->prtmb_img, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->prtmb_img, &_wave_alpha_63x63);
	lv_img_set_pivot(ui->prtmb_img, 0,0);
	lv_img_set_angle(ui->prtmb_img, 0);
	lv_obj_set_pos(ui->prtmb_img, 626, 219);
	lv_obj_set_size(ui->prtmb_img, 63, 63);
	lv_obj_set_scrollbar_mode(ui->prtmb_img, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtmb_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prtmb_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtmb_cloud
	ui->prtmb_cloud = lv_img_create(ui->prtmb);
	lv_obj_add_flag(ui->prtmb_cloud, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->prtmb_cloud, &_phone_alpha_120x145);
	lv_img_set_pivot(ui->prtmb_cloud, 0,0);
	lv_img_set_angle(ui->prtmb_cloud, 0);
	lv_obj_set_pos(ui->prtmb_cloud, 746, 189);
	lv_obj_set_size(ui->prtmb_cloud, 120, 145);
	lv_obj_set_scrollbar_mode(ui->prtmb_cloud, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtmb_cloud, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prtmb_cloud, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->prtmb);

	
}
