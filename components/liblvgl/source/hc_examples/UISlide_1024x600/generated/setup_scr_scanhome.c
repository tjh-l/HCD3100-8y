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


void setup_scr_scanhome(lv_ui *ui)
{
	//Write codes scanhome
	ui->scanhome = lv_obj_create(NULL);
	lv_obj_set_size(ui->scanhome, 1024, 600);
	lv_obj_set_scrollbar_mode(ui->scanhome, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_cont0
	ui->scanhome_cont0 = lv_obj_create(ui->scanhome);
	lv_obj_set_pos(ui->scanhome_cont0, 0, 0);
	lv_obj_set_size(ui->scanhome_cont0, 1024, 220);
	lv_obj_set_scrollbar_mode(ui->scanhome_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scanhome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scanhome_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scanhome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scanhome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scanhome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scanhome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_label1
	ui->scanhome_label1 = lv_label_create(ui->scanhome);
	lv_label_set_text(ui->scanhome_label1, "ADJUST IMAGE");
	lv_label_set_long_mode(ui->scanhome_label1, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->scanhome_label1, 290, 66);
	lv_obj_set_size(ui->scanhome_label1, 480, 44);
	lv_obj_set_scrollbar_mode(ui->scanhome_label1, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_label1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scanhome_label1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scanhome_label1, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scanhome_label1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scanhome_label1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_cont2
	ui->scanhome_cont2 = lv_obj_create(ui->scanhome);
	lv_obj_set_pos(ui->scanhome_cont2, 0, 220);
	lv_obj_set_size(ui->scanhome_cont2, 1024, 379);
	lv_obj_set_scrollbar_mode(ui->scanhome_cont2, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_cont2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scanhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scanhome_cont2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_cont2, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scanhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scanhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scanhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scanhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_img3
	ui->scanhome_img3 = lv_img_create(ui->scanhome);
	lv_obj_add_flag(ui->scanhome_img3, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scanhome_img3, &_example_alpha_640x379);
	lv_img_set_pivot(ui->scanhome_img3, 0,0);
	lv_img_set_angle(ui->scanhome_img3, 0);
	lv_obj_set_pos(ui->scanhome_img3, 57, 165);
	lv_obj_set_size(ui->scanhome_img3, 640, 379);
	lv_obj_set_scrollbar_mode(ui->scanhome_img3, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_img3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scanhome_img3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_cont4
	ui->scanhome_cont4 = lv_obj_create(ui->scanhome);
	lv_obj_set_pos(ui->scanhome_cont4, 785, 176);
	lv_obj_set_size(ui->scanhome_cont4, 170, 286);
	lv_obj_set_scrollbar_mode(ui->scanhome_cont4, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_cont4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scanhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scanhome_cont4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_cont4, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scanhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scanhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scanhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scanhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_btnscansave
	ui->scanhome_btnscansave = lv_btn_create(ui->scanhome);
	ui->scanhome_btnscansave_label = lv_label_create(ui->scanhome_btnscansave);
	lv_label_set_text(ui->scanhome_btnscansave_label, "SAVE");
	lv_label_set_long_mode(ui->scanhome_btnscansave_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scanhome_btnscansave_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scanhome_btnscansave, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->scanhome_btnscansave, 785, 487);
	lv_obj_set_size(ui->scanhome_btnscansave, 170, 88);
	lv_obj_set_scrollbar_mode(ui->scanhome_btnscansave, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_btnscansave, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_btnscansave, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_btnscansave, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_btnscansave, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_btnscansave, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scanhome_btnscansave, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_btnscansave, 110, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_btnscansave, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scanhome_btnscansave, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scanhome_btnscansave, &lv_font_simsun_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scanhome_btnscansave, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_sliderhue
	ui->scanhome_sliderhue = lv_slider_create(ui->scanhome);
	lv_slider_set_range(ui->scanhome_sliderhue, 0,100);
	lv_slider_set_value(ui->scanhome_sliderhue, 50, false);
	lv_obj_set_pos(ui->scanhome_sliderhue, 896, 253);
	lv_obj_set_size(ui->scanhome_sliderhue, 17, 176);
	lv_obj_set_scrollbar_mode(ui->scanhome_sliderhue, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_sliderhue, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_sliderhue, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_sliderhue, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_sliderhue, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_sliderhue, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_sliderhue, 110, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ui->scanhome_sliderhue, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_sliderhue, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scanhome_sliderhue, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_sliderhue, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_sliderhue, lv_color_hex(0xd4d7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_sliderhue, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_sliderhue, lv_color_hex(0xddd7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_sliderhue, 110, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for scanhome_sliderhue, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_sliderhue, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_sliderhue, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_sliderhue, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_sliderhue, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_sliderhue, 110, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes scanhome_sliderbright
	ui->scanhome_sliderbright = lv_slider_create(ui->scanhome);
	lv_slider_set_range(ui->scanhome_sliderbright, 0,100);
	lv_slider_set_value(ui->scanhome_sliderbright, 50, false);
	lv_obj_set_pos(ui->scanhome_sliderbright, 810, 253);
	lv_obj_set_size(ui->scanhome_sliderbright, 17, 176);
	lv_obj_set_scrollbar_mode(ui->scanhome_sliderbright, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_sliderbright, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_sliderbright, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_sliderbright, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_sliderbright, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_sliderbright, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_sliderbright, 110, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ui->scanhome_sliderbright, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_sliderbright, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scanhome_sliderbright, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_sliderbright, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_sliderbright, lv_color_hex(0xd4d7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_sliderbright, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_sliderbright, lv_color_hex(0xddd7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_sliderbright, 110, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for scanhome_sliderbright, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_sliderbright, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_sliderbright, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_sliderbright, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_sliderbright, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_sliderbright, 110, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes scanhome_bright
	ui->scanhome_bright = lv_img_create(ui->scanhome);
	lv_obj_add_flag(ui->scanhome_bright, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scanhome_bright, &_bright_alpha_51x51);
	lv_img_set_pivot(ui->scanhome_bright, 0,0);
	lv_img_set_angle(ui->scanhome_bright, 0);
	lv_obj_set_pos(ui->scanhome_bright, 793, 180);
	lv_obj_set_size(ui->scanhome_bright, 51, 51);
	lv_obj_set_scrollbar_mode(ui->scanhome_bright, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_bright, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scanhome_bright, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_hue
	ui->scanhome_hue = lv_img_create(ui->scanhome);
	lv_obj_add_flag(ui->scanhome_hue, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scanhome_hue, &_hue_alpha_44x44);
	lv_img_set_pivot(ui->scanhome_hue, 0,0);
	lv_img_set_angle(ui->scanhome_hue, 0);
	lv_obj_set_pos(ui->scanhome_hue, 881, 183);
	lv_obj_set_size(ui->scanhome_hue, 44, 44);
	lv_obj_set_scrollbar_mode(ui->scanhome_hue, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_hue, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scanhome_hue, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scanhome_btnscanback
	ui->scanhome_btnscanback = lv_btn_create(ui->scanhome);
	ui->scanhome_btnscanback_label = lv_label_create(ui->scanhome_btnscanback);
	lv_label_set_text(ui->scanhome_btnscanback_label, "<");
	lv_label_set_long_mode(ui->scanhome_btnscanback_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scanhome_btnscanback_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scanhome_btnscanback, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->scanhome_btnscanback, 106, 55);
	lv_obj_set_size(ui->scanhome_btnscanback, 64, 64);
	lv_obj_set_scrollbar_mode(ui->scanhome_btnscanback, LV_SCROLLBAR_MODE_OFF);

	//Write style for scanhome_btnscanback, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scanhome_btnscanback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scanhome_btnscanback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scanhome_btnscanback, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scanhome_btnscanback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scanhome_btnscanback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scanhome_btnscanback, 110, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scanhome_btnscanback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scanhome_btnscanback, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scanhome_btnscanback, &lv_font_simsun_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scanhome_btnscanback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->scanhome);

	
}
