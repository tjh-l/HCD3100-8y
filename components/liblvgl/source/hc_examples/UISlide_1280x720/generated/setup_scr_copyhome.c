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


void setup_scr_copyhome(lv_ui *ui)
{
	//Write codes copyhome
	ui->copyhome = lv_obj_create(NULL);
	lv_obj_set_size(ui->copyhome, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->copyhome, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_cont1
	ui->copyhome_cont1 = lv_obj_create(ui->copyhome);
	lv_obj_set_pos(ui->copyhome_cont1, 0, 0);
	lv_obj_set_size(ui->copyhome_cont1, 1280, 264);
	lv_obj_set_scrollbar_mode(ui->copyhome_cont1, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_cont1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copyhome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copyhome_cont1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_cont1, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_cont1, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_cont1, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copyhome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copyhome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copyhome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copyhome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_cont2
	ui->copyhome_cont2 = lv_obj_create(ui->copyhome);
	lv_obj_set_pos(ui->copyhome_cont2, 0, 264);
	lv_obj_set_size(ui->copyhome_cont2, 1280, 454);
	lv_obj_set_scrollbar_mode(ui->copyhome_cont2, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_cont2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copyhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copyhome_cont2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_cont2, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copyhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copyhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copyhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copyhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_label1
	ui->copyhome_label1 = lv_label_create(ui->copyhome);
	lv_label_set_text(ui->copyhome_label1, "ADJUST IMAGE");
	lv_label_set_long_mode(ui->copyhome_label1, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->copyhome_label1, 362, 79);
	lv_obj_set_size(ui->copyhome_label1, 600, 52);
	lv_obj_set_scrollbar_mode(ui->copyhome_label1, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_label1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copyhome_label1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copyhome_label1, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->copyhome_label1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copyhome_label1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_img3
	ui->copyhome_img3 = lv_img_create(ui->copyhome);
	lv_obj_add_flag(ui->copyhome_img3, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->copyhome_img3, &_example_alpha_800x454);
	lv_img_set_pivot(ui->copyhome_img3, 0,0);
	lv_img_set_angle(ui->copyhome_img3, 0);
	lv_obj_set_pos(ui->copyhome_img3, 71, 198);
	lv_obj_set_size(ui->copyhome_img3, 800, 454);
	lv_obj_set_scrollbar_mode(ui->copyhome_img3, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_img3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->copyhome_img3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_cont4
	ui->copyhome_cont4 = lv_obj_create(ui->copyhome);
	lv_obj_set_pos(ui->copyhome_cont4, 981, 211);
	lv_obj_set_size(ui->copyhome_cont4, 212, 343);
	lv_obj_set_scrollbar_mode(ui->copyhome_cont4, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_cont4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copyhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copyhome_cont4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_cont4, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copyhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copyhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copyhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copyhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_btncopynext
	ui->copyhome_btncopynext = lv_btn_create(ui->copyhome);
	ui->copyhome_btncopynext_label = lv_label_create(ui->copyhome_btncopynext);
	lv_label_set_text(ui->copyhome_btncopynext_label, "NEXT");
	lv_label_set_long_mode(ui->copyhome_btncopynext_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->copyhome_btncopynext_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->copyhome_btncopynext, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->copyhome_btncopynext, 981, 584);
	lv_obj_set_size(ui->copyhome_btncopynext, 212, 105);
	lv_obj_set_scrollbar_mode(ui->copyhome_btncopynext, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_btncopynext, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_btncopynext, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_btncopynext, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_btncopynext, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_btncopynext, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copyhome_btncopynext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_btncopynext, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_btncopynext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copyhome_btncopynext, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copyhome_btncopynext, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copyhome_btncopynext, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_sliderhue
	ui->copyhome_sliderhue = lv_slider_create(ui->copyhome);
	lv_slider_set_range(ui->copyhome_sliderhue, 0,100);
	lv_slider_set_value(ui->copyhome_sliderhue, 50, false);
	lv_obj_set_pos(ui->copyhome_sliderhue, 1120, 303);
	lv_obj_set_size(ui->copyhome_sliderhue, 21, 211);
	lv_obj_set_scrollbar_mode(ui->copyhome_sliderhue, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_sliderhue, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_sliderhue, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_sliderhue, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_sliderhue, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_sliderhue, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_sliderhue, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ui->copyhome_sliderhue, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_sliderhue, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for copyhome_sliderhue, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_sliderhue, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_sliderhue, lv_color_hex(0xd4d7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_sliderhue, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_sliderhue, lv_color_hex(0xddd7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_sliderhue, 137, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for copyhome_sliderhue, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_sliderhue, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_sliderhue, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_sliderhue, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_sliderhue, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_sliderhue, 137, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes copyhome_sliderbright
	ui->copyhome_sliderbright = lv_slider_create(ui->copyhome);
	lv_slider_set_range(ui->copyhome_sliderbright, 0,100);
	lv_slider_set_value(ui->copyhome_sliderbright, 50, false);
	lv_obj_set_pos(ui->copyhome_sliderbright, 1012, 303);
	lv_obj_set_size(ui->copyhome_sliderbright, 21, 211);
	lv_obj_set_scrollbar_mode(ui->copyhome_sliderbright, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_sliderbright, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_sliderbright, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_sliderbright, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_sliderbright, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_sliderbright, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_sliderbright, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ui->copyhome_sliderbright, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_sliderbright, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for copyhome_sliderbright, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_sliderbright, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_sliderbright, lv_color_hex(0xd4d7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_sliderbright, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_sliderbright, lv_color_hex(0xddd7d9), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_sliderbright, 137, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for copyhome_sliderbright, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_sliderbright, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_sliderbright, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_sliderbright, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_sliderbright, lv_color_hex(0x293041), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_sliderbright, 137, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes copyhome_bright
	ui->copyhome_bright = lv_img_create(ui->copyhome);
	lv_obj_add_flag(ui->copyhome_bright, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->copyhome_bright, &_bright_alpha_61x61);
	lv_img_set_pivot(ui->copyhome_bright, 0,0);
	lv_img_set_angle(ui->copyhome_bright, 0);
	lv_obj_set_pos(ui->copyhome_bright, 991, 216);
	lv_obj_set_size(ui->copyhome_bright, 61, 61);
	lv_obj_set_scrollbar_mode(ui->copyhome_bright, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_bright, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->copyhome_bright, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_hue
	ui->copyhome_hue = lv_img_create(ui->copyhome);
	lv_obj_add_flag(ui->copyhome_hue, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->copyhome_hue, &_hue_alpha_52x52);
	lv_img_set_pivot(ui->copyhome_hue, 0,0);
	lv_img_set_angle(ui->copyhome_hue, 0);
	lv_obj_set_pos(ui->copyhome_hue, 1101, 219);
	lv_obj_set_size(ui->copyhome_hue, 52, 52);
	lv_obj_set_scrollbar_mode(ui->copyhome_hue, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_hue, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->copyhome_hue, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copyhome_btncopyback
	ui->copyhome_btncopyback = lv_btn_create(ui->copyhome);
	ui->copyhome_btncopyback_label = lv_label_create(ui->copyhome_btncopyback);
	lv_label_set_text(ui->copyhome_btncopyback_label, "<");
	lv_label_set_long_mode(ui->copyhome_btncopyback_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->copyhome_btncopyback_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->copyhome_btncopyback, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->copyhome_btncopyback, 132, 66);
	lv_obj_set_size(ui->copyhome_btncopyback, 76, 76);
	lv_obj_set_scrollbar_mode(ui->copyhome_btncopyback, LV_SCROLLBAR_MODE_OFF);

	//Write style for copyhome_btncopyback, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copyhome_btncopyback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copyhome_btncopyback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copyhome_btncopyback, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copyhome_btncopyback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copyhome_btncopyback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copyhome_btncopyback, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copyhome_btncopyback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copyhome_btncopyback, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copyhome_btncopyback, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copyhome_btncopyback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->copyhome);

	
}
