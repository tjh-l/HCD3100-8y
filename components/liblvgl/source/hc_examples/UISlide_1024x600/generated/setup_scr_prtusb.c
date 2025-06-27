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


void setup_scr_prtusb(lv_ui *ui)
{
	//Write codes prtusb
	ui->prtusb = lv_obj_create(NULL);
	lv_obj_set_size(ui->prtusb, 1024, 600);
	lv_obj_set_scrollbar_mode(ui->prtusb, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_cont0
	ui->prtusb_cont0 = lv_obj_create(ui->prtusb);
	lv_obj_set_pos(ui->prtusb_cont0, 0, 0);
	lv_obj_set_size(ui->prtusb_cont0, 1024, 220);
	lv_obj_set_scrollbar_mode(ui->prtusb_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_cont2
	ui->prtusb_cont2 = lv_obj_create(ui->prtusb);
	lv_obj_set_pos(ui->prtusb_cont2, 0, 220);
	lv_obj_set_size(ui->prtusb_cont2, 1024, 379);
	lv_obj_set_scrollbar_mode(ui->prtusb_cont2, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_cont2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_cont2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_cont2, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_labeltitle
	ui->prtusb_labeltitle = lv_label_create(ui->prtusb);
	lv_label_set_text(ui->prtusb_labeltitle, "PRINTING FROM USB");
	lv_label_set_long_mode(ui->prtusb_labeltitle, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prtusb_labeltitle, 290, 66);
	lv_obj_set_size(ui->prtusb_labeltitle, 480, 66);
	lv_obj_set_scrollbar_mode(ui->prtusb_labeltitle, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_labeltitle, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_labeltitle, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_labeltitle, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prtusb_labeltitle, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_labeltitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_labeltitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_cont4
	ui->prtusb_cont4 = lv_obj_create(ui->prtusb);
	lv_obj_set_pos(ui->prtusb_cont4, 650, 176);
	lv_obj_set_size(ui->prtusb_cont4, 320, 286);
	lv_obj_set_scrollbar_mode(ui->prtusb_cont4, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_cont4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_cont4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_cont4, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_btnprint
	ui->prtusb_btnprint = lv_btn_create(ui->prtusb);
	ui->prtusb_btnprint_label = lv_label_create(ui->prtusb_btnprint);
	lv_label_set_text(ui->prtusb_btnprint_label, "PRINT");
	lv_label_set_long_mode(ui->prtusb_btnprint_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prtusb_btnprint_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prtusb_btnprint, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prtusb_btnprint, 682, 491);
	lv_obj_set_size(ui->prtusb_btnprint, 251, 88);
	lv_obj_set_scrollbar_mode(ui->prtusb_btnprint, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_btnprint, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_btnprint, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_btnprint, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_btnprint, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_btnprint, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_btnprint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_btnprint, 110, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_btnprint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_btnprint, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_btnprint, &lv_font_simsun_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_btnprint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_back
	ui->prtusb_back = lv_btn_create(ui->prtusb);
	ui->prtusb_back_label = lv_label_create(ui->prtusb_back);
	lv_label_set_text(ui->prtusb_back_label, "<");
	lv_label_set_long_mode(ui->prtusb_back_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prtusb_back_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prtusb_back, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prtusb_back, 106, 55);
	lv_obj_set_size(ui->prtusb_back, 64, 64);
	lv_obj_set_scrollbar_mode(ui->prtusb_back, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_back, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_back, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_back, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_back, 110, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_back, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_back, &lv_font_simsun_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_swcolor
	ui->prtusb_swcolor = lv_switch_create(ui->prtusb);
	lv_obj_set_pos(ui->prtusb_swcolor, 689, 386);
	lv_obj_set_size(ui->prtusb_swcolor, 85, 44);
	lv_obj_set_scrollbar_mode(ui->prtusb_swcolor, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_swcolor, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_swcolor, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_swcolor, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_swcolor, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_swcolor, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_swcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_swcolor, 220, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_swcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for prtusb_swcolor, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->prtusb_swcolor, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->prtusb_swcolor, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_swcolor, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_color(ui->prtusb_swcolor, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->prtusb_swcolor, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for prtusb_swcolor, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_swcolor, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_swcolor, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_swcolor, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_swcolor, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_swcolor, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_swcolor, 220, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes prtusb_labelcopy
	ui->prtusb_labelcopy = lv_label_create(ui->prtusb);
	lv_label_set_text(ui->prtusb_labelcopy, "Copies");
	lv_label_set_long_mode(ui->prtusb_labelcopy, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prtusb_labelcopy, 736, 191);
	lv_obj_set_size(ui->prtusb_labelcopy, 136, 44);
	lv_obj_set_scrollbar_mode(ui->prtusb_labelcopy, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_labelcopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_labelcopy, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_labelcopy, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prtusb_labelcopy, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_labelcopy, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_up
	ui->prtusb_up = lv_btn_create(ui->prtusb);
	ui->prtusb_up_label = lv_label_create(ui->prtusb_up);
	lv_label_set_text(ui->prtusb_up_label, "+");
	lv_label_set_long_mode(ui->prtusb_up_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prtusb_up_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prtusb_up, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prtusb_up, 874, 242);
	lv_obj_set_size(ui->prtusb_up, 42, 42);
	lv_obj_set_scrollbar_mode(ui->prtusb_up, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_up, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_up, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_up, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_up, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_up, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_up, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_up, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_up, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_up, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_up, &lv_font_simsun_38, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_up, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_down
	ui->prtusb_down = lv_btn_create(ui->prtusb);
	ui->prtusb_down_label = lv_label_create(ui->prtusb_down);
	lv_label_set_text(ui->prtusb_down_label, "-");
	lv_label_set_long_mode(ui->prtusb_down_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prtusb_down_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prtusb_down, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prtusb_down, 686, 242);
	lv_obj_set_size(ui->prtusb_down, 42, 42);
	lv_obj_set_scrollbar_mode(ui->prtusb_down, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_down, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_down, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_down, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_down, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_down, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_down, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_down, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_down, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_down, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_down, &lv_font_simsun_38, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_down, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_labelcnt
	ui->prtusb_labelcnt = lv_label_create(ui->prtusb);
	lv_label_set_text(ui->prtusb_labelcnt, "1");
	lv_label_set_long_mode(ui->prtusb_labelcnt, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prtusb_labelcnt, 708, 242);
	lv_obj_set_size(ui->prtusb_labelcnt, 192, 66);
	lv_obj_set_scrollbar_mode(ui->prtusb_labelcnt, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_labelcnt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_labelcnt, lv_color_hex(0x141010), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_labelcnt, &lv_font_arial_44, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prtusb_labelcnt, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_labelcnt, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_labelcolor
	ui->prtusb_labelcolor = lv_label_create(ui->prtusb);
	lv_label_set_text(ui->prtusb_labelcolor, "Color");
	lv_label_set_long_mode(ui->prtusb_labelcolor, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prtusb_labelcolor, 669, 322);
	lv_obj_set_size(ui->prtusb_labelcolor, 106, 44);
	lv_obj_set_scrollbar_mode(ui->prtusb_labelcolor, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_labelcolor, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_labelcolor, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_labelcolor, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prtusb_labelcolor, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_labelcolor, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_labelvert
	ui->prtusb_labelvert = lv_label_create(ui->prtusb);
	lv_label_set_text(ui->prtusb_labelvert, "Vertical");
	lv_label_set_long_mode(ui->prtusb_labelvert, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prtusb_labelvert, 810, 322);
	lv_obj_set_size(ui->prtusb_labelvert, 149, 44);
	lv_obj_set_scrollbar_mode(ui->prtusb_labelvert, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_labelvert, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prtusb_labelvert, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_labelvert, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prtusb_labelvert, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prtusb_labelvert, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prtusb_swvert
	ui->prtusb_swvert = lv_switch_create(ui->prtusb);
	lv_obj_set_pos(ui->prtusb_swvert, 832, 386);
	lv_obj_set_size(ui->prtusb_swvert, 85, 44);
	lv_obj_set_scrollbar_mode(ui->prtusb_swvert, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_swvert, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_swvert, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_swvert, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_swvert, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_swvert, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_swvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_swvert, 220, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_swvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for prtusb_swvert, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->prtusb_swvert, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->prtusb_swvert, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_swvert, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_color(ui->prtusb_swvert, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->prtusb_swvert, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for prtusb_swvert, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prtusb_swvert, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_swvert, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_swvert, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_swvert, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_swvert, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_swvert, 220, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes prtusb_list16
	ui->prtusb_list16 = lv_list_create(ui->prtusb);
	ui->prtusb_list16_item0 = lv_list_add_btn(ui->prtusb_list16, LV_SYMBOL_FILE, "Contract 12.pdf");
	ui->prtusb_list16_item1 = lv_list_add_btn(ui->prtusb_list16, LV_SYMBOL_FILE, "Scanned_05_21.pdf");
	ui->prtusb_list16_item2 = lv_list_add_btn(ui->prtusb_list16, LV_SYMBOL_FILE, "Photo_2.jpg");
	ui->prtusb_list16_item3 = lv_list_add_btn(ui->prtusb_list16, LV_SYMBOL_FILE, "Photo_3.jpg");
	lv_obj_set_pos(ui->prtusb_list16, 66, 183);
	lv_obj_set_size(ui->prtusb_list16, 512, 273);
	lv_obj_set_scrollbar_mode(ui->prtusb_list16, LV_SCROLLBAR_MODE_OFF);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_list16_main_main_default
	static lv_style_t style_prtusb_list16_main_main_default;
	ui_init_style(&style_prtusb_list16_main_main_default);
	
	lv_style_set_pad_top(&style_prtusb_list16_main_main_default, 5);
	lv_style_set_pad_left(&style_prtusb_list16_main_main_default, 5);
	lv_style_set_pad_right(&style_prtusb_list16_main_main_default, 5);
	lv_style_set_pad_bottom(&style_prtusb_list16_main_main_default, 5);
	lv_style_set_bg_opa(&style_prtusb_list16_main_main_default, 255);
	lv_style_set_bg_color(&style_prtusb_list16_main_main_default, lv_color_hex(0xffffff));
	lv_style_set_bg_grad_dir(&style_prtusb_list16_main_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_grad_color(&style_prtusb_list16_main_main_default, lv_color_hex(0xffffff));
	lv_style_set_border_width(&style_prtusb_list16_main_main_default, 1);
	lv_style_set_border_opa(&style_prtusb_list16_main_main_default, 255);
	lv_style_set_border_color(&style_prtusb_list16_main_main_default, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_prtusb_list16_main_main_default, 6);
	lv_style_set_shadow_width(&style_prtusb_list16_main_main_default, 0);
	lv_obj_add_style(ui->prtusb_list16, &style_prtusb_list16_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_list16_main_scrollbar_default
	static lv_style_t style_prtusb_list16_main_scrollbar_default;
	ui_init_style(&style_prtusb_list16_main_scrollbar_default);
	
	lv_style_set_radius(&style_prtusb_list16_main_scrollbar_default, 6);
	lv_style_set_bg_opa(&style_prtusb_list16_main_scrollbar_default, 255);
	lv_style_set_bg_color(&style_prtusb_list16_main_scrollbar_default, lv_color_hex(0xffffff));
	lv_obj_add_style(ui->prtusb_list16, &style_prtusb_list16_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_list16_extra_btns_main_default
	static lv_style_t style_prtusb_list16_extra_btns_main_default;
	ui_init_style(&style_prtusb_list16_extra_btns_main_default);
	
	lv_style_set_pad_top(&style_prtusb_list16_extra_btns_main_default, 5);
	lv_style_set_pad_left(&style_prtusb_list16_extra_btns_main_default, 5);
	lv_style_set_pad_right(&style_prtusb_list16_extra_btns_main_default, 5);
	lv_style_set_pad_bottom(&style_prtusb_list16_extra_btns_main_default, 5);
	lv_style_set_border_width(&style_prtusb_list16_extra_btns_main_default, 0);
	lv_style_set_text_color(&style_prtusb_list16_extra_btns_main_default, lv_color_hex(0x0D3055));
	lv_style_set_text_font(&style_prtusb_list16_extra_btns_main_default, &lv_font_arial_25);
	lv_style_set_radius(&style_prtusb_list16_extra_btns_main_default, 6);
	lv_style_set_bg_opa(&style_prtusb_list16_extra_btns_main_default, 255);
	lv_style_set_bg_color(&style_prtusb_list16_extra_btns_main_default, lv_color_hex(0xffffff));
	lv_style_set_bg_grad_dir(&style_prtusb_list16_extra_btns_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_grad_color(&style_prtusb_list16_extra_btns_main_default, lv_color_hex(0xffffff));
	lv_obj_add_style(ui->prtusb_list16_item3, &style_prtusb_list16_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_style(ui->prtusb_list16_item2, &style_prtusb_list16_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_style(ui->prtusb_list16_item1, &style_prtusb_list16_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_style(ui->prtusb_list16_item0, &style_prtusb_list16_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_list16_extra_texts_main_default
	static lv_style_t style_prtusb_list16_extra_texts_main_default;
	ui_init_style(&style_prtusb_list16_extra_texts_main_default);
	
	lv_style_set_pad_top(&style_prtusb_list16_extra_texts_main_default, 5);
	lv_style_set_pad_left(&style_prtusb_list16_extra_texts_main_default, 5);
	lv_style_set_pad_right(&style_prtusb_list16_extra_texts_main_default, 5);
	lv_style_set_pad_bottom(&style_prtusb_list16_extra_texts_main_default, 5);
	lv_style_set_border_width(&style_prtusb_list16_extra_texts_main_default, 0);
	lv_style_set_text_color(&style_prtusb_list16_extra_texts_main_default, lv_color_hex(0x0D3055));
	lv_style_set_text_font(&style_prtusb_list16_extra_texts_main_default, &lv_font_montserratMedium_25);
	lv_style_set_radius(&style_prtusb_list16_extra_texts_main_default, 6);
	lv_style_set_bg_opa(&style_prtusb_list16_extra_texts_main_default, 255);
	lv_style_set_bg_color(&style_prtusb_list16_extra_texts_main_default, lv_color_hex(0xffffff));

	//Write codes prtusb_ddlist1
	ui->prtusb_ddlist1 = lv_dropdown_create(ui->prtusb);
	lv_dropdown_set_options(ui->prtusb_ddlist1, "Best\nNormal\nDraft");
	lv_obj_set_pos(ui->prtusb_ddlist1, 59, 485);
	lv_obj_set_size(ui->prtusb_ddlist1, 213, 46);
	lv_obj_set_scrollbar_mode(ui->prtusb_ddlist1, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_ddlist1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_text_color(ui->prtusb_ddlist1, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_ddlist1, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_ddlist1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->prtusb_ddlist1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->prtusb_ddlist1, lv_color_hex(0xe1e6ee), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_ddlist1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_ddlist1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_ddlist1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_ddlist1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_ddlist1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_ddlist1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_ddlist1, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_ddlist1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_ddlist1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_CHECKED for &style_prtusb_ddlist1_extra_list_selected_checked
	static lv_style_t style_prtusb_ddlist1_extra_list_selected_checked;
	ui_init_style(&style_prtusb_ddlist1_extra_list_selected_checked);
	
	lv_style_set_text_color(&style_prtusb_ddlist1_extra_list_selected_checked, lv_color_hex(0xffffff));
	lv_style_set_text_font(&style_prtusb_ddlist1_extra_list_selected_checked, &lv_font_montserratMedium_25);
	lv_style_set_border_width(&style_prtusb_ddlist1_extra_list_selected_checked, 1);
	lv_style_set_border_opa(&style_prtusb_ddlist1_extra_list_selected_checked, 255);
	lv_style_set_border_color(&style_prtusb_ddlist1_extra_list_selected_checked, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_prtusb_ddlist1_extra_list_selected_checked, 6);
	lv_style_set_bg_opa(&style_prtusb_ddlist1_extra_list_selected_checked, 255);
	lv_style_set_bg_color(&style_prtusb_ddlist1_extra_list_selected_checked, lv_color_hex(0x00a1b5));
	lv_obj_add_style(lv_dropdown_get_list(ui->prtusb_ddlist1), &style_prtusb_ddlist1_extra_list_selected_checked, LV_PART_SELECTED|LV_STATE_CHECKED);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_ddlist1_extra_list_main_default
	static lv_style_t style_prtusb_ddlist1_extra_list_main_default;
	ui_init_style(&style_prtusb_ddlist1_extra_list_main_default);
	
	lv_style_set_max_height(&style_prtusb_ddlist1_extra_list_main_default, 90);
	lv_style_set_text_color(&style_prtusb_ddlist1_extra_list_main_default, lv_color_hex(0x0D3055));
	lv_style_set_text_font(&style_prtusb_ddlist1_extra_list_main_default, &lv_font_arial_25);
	lv_style_set_border_width(&style_prtusb_ddlist1_extra_list_main_default, 1);
	lv_style_set_border_opa(&style_prtusb_ddlist1_extra_list_main_default, 255);
	lv_style_set_border_color(&style_prtusb_ddlist1_extra_list_main_default, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_prtusb_ddlist1_extra_list_main_default, 6);
	lv_style_set_bg_opa(&style_prtusb_ddlist1_extra_list_main_default, 255);
	lv_style_set_bg_color(&style_prtusb_ddlist1_extra_list_main_default, lv_color_hex(0xffffff));
	lv_style_set_bg_grad_dir(&style_prtusb_ddlist1_extra_list_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_grad_color(&style_prtusb_ddlist1_extra_list_main_default, lv_color_hex(0xffffff));
	lv_obj_add_style(lv_dropdown_get_list(ui->prtusb_ddlist1), &style_prtusb_ddlist1_extra_list_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_ddlist1_extra_list_scrollbar_default
	static lv_style_t style_prtusb_ddlist1_extra_list_scrollbar_default;
	ui_init_style(&style_prtusb_ddlist1_extra_list_scrollbar_default);
	
	lv_style_set_radius(&style_prtusb_ddlist1_extra_list_scrollbar_default, 6);
	lv_style_set_bg_opa(&style_prtusb_ddlist1_extra_list_scrollbar_default, 255);
	lv_style_set_bg_color(&style_prtusb_ddlist1_extra_list_scrollbar_default, lv_color_hex(0x00ff00));
	lv_obj_add_style(lv_dropdown_get_list(ui->prtusb_ddlist1), &style_prtusb_ddlist1_extra_list_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Write codes prtusb_ddlist2
	ui->prtusb_ddlist2 = lv_dropdown_create(ui->prtusb);
	lv_dropdown_set_options(ui->prtusb_ddlist2, "72 DPI\n96 DPI\n150 DPI\n300 DPI\n600 DPI\n900 DPI\n1200 DPI");
	lv_obj_set_pos(ui->prtusb_ddlist2, 354, 485);
	lv_obj_set_size(ui->prtusb_ddlist2, 213, 50);
	lv_obj_set_scrollbar_mode(ui->prtusb_ddlist2, LV_SCROLLBAR_MODE_OFF);

	//Write style for prtusb_ddlist2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_text_color(ui->prtusb_ddlist2, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prtusb_ddlist2, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prtusb_ddlist2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->prtusb_ddlist2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->prtusb_ddlist2, lv_color_hex(0xe1e6ee), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prtusb_ddlist2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prtusb_ddlist2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prtusb_ddlist2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prtusb_ddlist2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prtusb_ddlist2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prtusb_ddlist2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prtusb_ddlist2, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prtusb_ddlist2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prtusb_ddlist2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_CHECKED for &style_prtusb_ddlist2_extra_list_selected_checked
	static lv_style_t style_prtusb_ddlist2_extra_list_selected_checked;
	ui_init_style(&style_prtusb_ddlist2_extra_list_selected_checked);
	
	lv_style_set_text_color(&style_prtusb_ddlist2_extra_list_selected_checked, lv_color_hex(0xffffff));
	lv_style_set_text_font(&style_prtusb_ddlist2_extra_list_selected_checked, &lv_font_montserratMedium_25);
	lv_style_set_border_width(&style_prtusb_ddlist2_extra_list_selected_checked, 1);
	lv_style_set_border_opa(&style_prtusb_ddlist2_extra_list_selected_checked, 255);
	lv_style_set_border_color(&style_prtusb_ddlist2_extra_list_selected_checked, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_prtusb_ddlist2_extra_list_selected_checked, 6);
	lv_style_set_bg_opa(&style_prtusb_ddlist2_extra_list_selected_checked, 255);
	lv_style_set_bg_color(&style_prtusb_ddlist2_extra_list_selected_checked, lv_color_hex(0x00a1b5));
	lv_obj_add_style(lv_dropdown_get_list(ui->prtusb_ddlist2), &style_prtusb_ddlist2_extra_list_selected_checked, LV_PART_SELECTED|LV_STATE_CHECKED);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_ddlist2_extra_list_main_default
	static lv_style_t style_prtusb_ddlist2_extra_list_main_default;
	ui_init_style(&style_prtusb_ddlist2_extra_list_main_default);
	
	lv_style_set_max_height(&style_prtusb_ddlist2_extra_list_main_default, 90);
	lv_style_set_text_color(&style_prtusb_ddlist2_extra_list_main_default, lv_color_hex(0x0D3055));
	lv_style_set_text_font(&style_prtusb_ddlist2_extra_list_main_default, &lv_font_arial_25);
	lv_style_set_border_width(&style_prtusb_ddlist2_extra_list_main_default, 1);
	lv_style_set_border_opa(&style_prtusb_ddlist2_extra_list_main_default, 255);
	lv_style_set_border_color(&style_prtusb_ddlist2_extra_list_main_default, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_prtusb_ddlist2_extra_list_main_default, 6);
	lv_style_set_bg_opa(&style_prtusb_ddlist2_extra_list_main_default, 255);
	lv_style_set_bg_color(&style_prtusb_ddlist2_extra_list_main_default, lv_color_hex(0xffffff));
	lv_style_set_bg_grad_dir(&style_prtusb_ddlist2_extra_list_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_grad_color(&style_prtusb_ddlist2_extra_list_main_default, lv_color_hex(0xffffff));
	lv_obj_add_style(lv_dropdown_get_list(ui->prtusb_ddlist2), &style_prtusb_ddlist2_extra_list_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_prtusb_ddlist2_extra_list_scrollbar_default
	static lv_style_t style_prtusb_ddlist2_extra_list_scrollbar_default;
	ui_init_style(&style_prtusb_ddlist2_extra_list_scrollbar_default);
	
	lv_style_set_radius(&style_prtusb_ddlist2_extra_list_scrollbar_default, 6);
	lv_style_set_bg_opa(&style_prtusb_ddlist2_extra_list_scrollbar_default, 255);
	lv_style_set_bg_color(&style_prtusb_ddlist2_extra_list_scrollbar_default, lv_color_hex(0x00ff00));
	lv_obj_add_style(lv_dropdown_get_list(ui->prtusb_ddlist2), &style_prtusb_ddlist2_extra_list_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->prtusb);

	
}
