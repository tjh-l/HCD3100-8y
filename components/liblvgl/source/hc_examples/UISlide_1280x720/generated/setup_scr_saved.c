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


void setup_scr_saved(lv_ui *ui)
{
	//Write codes saved
	ui->saved = lv_obj_create(NULL);
	ui->g_kb_saved = lv_keyboard_create(ui->saved);
	lv_obj_add_event_cb(ui->g_kb_saved, kb_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_add_flag(ui->g_kb_saved, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_text_font(ui->g_kb_saved, &lv_font_simsun_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_size(ui->saved, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->saved, LV_SCROLLBAR_MODE_OFF);

	//Write style for saved, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->saved, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes saved_cont0
	ui->saved_cont0 = lv_obj_create(ui->saved);
	lv_obj_set_pos(ui->saved_cont0, 0, 0);
	lv_obj_set_size(ui->saved_cont0, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->saved_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for saved_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->saved_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->saved_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->saved_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->saved_cont0, lv_color_hex(0x4ab243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->saved_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->saved_cont0, lv_color_hex(0x4ab243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->saved_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->saved_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->saved_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->saved_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->saved_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes saved_btnsavecontinue
	ui->saved_btnsavecontinue = lv_btn_create(ui->saved);
	ui->saved_btnsavecontinue_label = lv_label_create(ui->saved_btnsavecontinue);
	lv_label_set_text(ui->saved_btnsavecontinue_label, "CONTINUE");
	lv_label_set_long_mode(ui->saved_btnsavecontinue_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->saved_btnsavecontinue_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->saved_btnsavecontinue, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->saved_btnsavecontinue, 447, 516);
	lv_obj_set_size(ui->saved_btnsavecontinue, 372, 105);
	lv_obj_set_scrollbar_mode(ui->saved_btnsavecontinue, LV_SCROLLBAR_MODE_OFF);

	//Write style for saved_btnsavecontinue, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->saved_btnsavecontinue, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->saved_btnsavecontinue, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->saved_btnsavecontinue, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->saved_btnsavecontinue, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->saved_btnsavecontinue, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->saved_btnsavecontinue, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->saved_btnsavecontinue, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->saved_btnsavecontinue, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->saved_btnsavecontinue, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->saved_btnsavecontinue, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->saved_btnsavecontinue, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->saved_btnsavecontinue, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes saved_label2
	ui->saved_label2 = lv_label_create(ui->saved);
	lv_label_set_text(ui->saved_label2, "File saved");
	lv_label_set_long_mode(ui->saved_label2, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->saved_label2, 400, 422);
	lv_obj_set_size(ui->saved_label2, 480, 52);
	lv_obj_set_scrollbar_mode(ui->saved_label2, LV_SCROLLBAR_MODE_OFF);

	//Write style for saved_label2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->saved_label2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->saved_label2, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->saved_label2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->saved_label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->saved_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes saved_img1
	ui->saved_img1 = lv_img_create(ui->saved);
	lv_obj_add_flag(ui->saved_img1, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->saved_img1, &_ready_alpha_255x255);
	lv_img_set_pivot(ui->saved_img1, 0,0);
	lv_img_set_angle(ui->saved_img1, 0);
	lv_obj_set_pos(ui->saved_img1, 492, 105);
	lv_obj_set_size(ui->saved_img1, 255, 255);
	lv_obj_set_scrollbar_mode(ui->saved_img1, LV_SCROLLBAR_MODE_OFF);

	//Write style for saved_img1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->saved_img1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->saved);

	
}
