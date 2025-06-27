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


void setup_scr_copynext(lv_ui *ui)
{
	//Write codes copynext
	ui->copynext = lv_obj_create(NULL);
	lv_obj_set_size(ui->copynext, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->copynext, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_cont1
	ui->copynext_cont1 = lv_obj_create(ui->copynext);
	lv_obj_set_pos(ui->copynext_cont1, 0, 0);
	lv_obj_set_size(ui->copynext_cont1, 1280, 264);
	lv_obj_set_scrollbar_mode(ui->copynext_cont1, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_cont1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_cont1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_cont1, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_cont1, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_cont1, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_cont2
	ui->copynext_cont2 = lv_obj_create(ui->copynext);
	lv_obj_set_pos(ui->copynext_cont2, 0, 264);
	lv_obj_set_size(ui->copynext_cont2, 1280, 454);
	lv_obj_set_scrollbar_mode(ui->copynext_cont2, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_cont2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_cont2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_cont2, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_cont2, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_cont2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_label1
	ui->copynext_label1 = lv_label_create(ui->copynext);
	lv_label_set_text(ui->copynext_label1, "ADJUST IMAGE");
	lv_label_set_long_mode(ui->copynext_label1, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->copynext_label1, 362, 79);
	lv_obj_set_size(ui->copynext_label1, 600, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_label1, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_label1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_label1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_label1, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->copynext_label1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_label1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_label1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_img3
	ui->copynext_img3 = lv_img_create(ui->copynext);
	lv_obj_add_flag(ui->copynext_img3, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->copynext_img3, &_example_alpha_666x362);
	lv_img_set_pivot(ui->copynext_img3, 0,0);
	lv_img_set_angle(ui->copynext_img3, 0);
	lv_obj_set_pos(ui->copynext_img3, 71, 198);
	lv_obj_set_size(ui->copynext_img3, 666, 362);
	lv_obj_set_scrollbar_mode(ui->copynext_img3, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_img3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->copynext_img3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_cont4
	ui->copynext_cont4 = lv_obj_create(ui->copynext);
	lv_obj_set_pos(ui->copynext_cont4, 812, 211);
	lv_obj_set_size(ui->copynext_cont4, 400, 343);
	lv_obj_set_scrollbar_mode(ui->copynext_cont4, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_cont4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_cont4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_cont4, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_cont4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_cont4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_ddlist2
	ui->copynext_ddlist2 = lv_dropdown_create(ui->copynext);
	lv_dropdown_set_options(ui->copynext_ddlist2, "72 DPI\n96 DPI\n150 DPI\n300 DPI\n600 DPI\n900 DPI\n1200 DPI");
	lv_obj_set_pos(ui->copynext_ddlist2, 442, 602);
	lv_obj_set_size(ui->copynext_ddlist2, 266, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_ddlist2, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_ddlist2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_text_color(ui->copynext_ddlist2, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_ddlist2, &lv_font_arial_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_ddlist2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->copynext_ddlist2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->copynext_ddlist2, lv_color_hex(0xe1e6ee), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_ddlist2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_ddlist2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_ddlist2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_ddlist2, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_ddlist2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_ddlist2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_ddlist2, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_ddlist2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_ddlist2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_CHECKED for &style_copynext_ddlist2_extra_list_selected_checked
	static lv_style_t style_copynext_ddlist2_extra_list_selected_checked;
	ui_init_style(&style_copynext_ddlist2_extra_list_selected_checked);
	
	lv_style_set_text_color(&style_copynext_ddlist2_extra_list_selected_checked, lv_color_hex(0xffffff));
	lv_style_set_text_font(&style_copynext_ddlist2_extra_list_selected_checked, &lv_font_montserratMedium_30);
	lv_style_set_border_width(&style_copynext_ddlist2_extra_list_selected_checked, 1);
	lv_style_set_border_opa(&style_copynext_ddlist2_extra_list_selected_checked, 255);
	lv_style_set_border_color(&style_copynext_ddlist2_extra_list_selected_checked, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_copynext_ddlist2_extra_list_selected_checked, 7);
	lv_style_set_bg_opa(&style_copynext_ddlist2_extra_list_selected_checked, 255);
	lv_style_set_bg_color(&style_copynext_ddlist2_extra_list_selected_checked, lv_color_hex(0x00a1b5));
	lv_obj_add_style(lv_dropdown_get_list(ui->copynext_ddlist2), &style_copynext_ddlist2_extra_list_selected_checked, LV_PART_SELECTED|LV_STATE_CHECKED);

	//Write style state: LV_STATE_DEFAULT for &style_copynext_ddlist2_extra_list_main_default
	static lv_style_t style_copynext_ddlist2_extra_list_main_default;
	ui_init_style(&style_copynext_ddlist2_extra_list_main_default);
	
	lv_style_set_max_height(&style_copynext_ddlist2_extra_list_main_default, 90);
	lv_style_set_text_color(&style_copynext_ddlist2_extra_list_main_default, lv_color_hex(0x0D3055));
	lv_style_set_text_font(&style_copynext_ddlist2_extra_list_main_default, &lv_font_arial_30);
	lv_style_set_border_width(&style_copynext_ddlist2_extra_list_main_default, 1);
	lv_style_set_border_opa(&style_copynext_ddlist2_extra_list_main_default, 255);
	lv_style_set_border_color(&style_copynext_ddlist2_extra_list_main_default, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_copynext_ddlist2_extra_list_main_default, 7);
	lv_style_set_bg_opa(&style_copynext_ddlist2_extra_list_main_default, 255);
	lv_style_set_bg_color(&style_copynext_ddlist2_extra_list_main_default, lv_color_hex(0xffffff));
	lv_style_set_bg_grad_dir(&style_copynext_ddlist2_extra_list_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_grad_color(&style_copynext_ddlist2_extra_list_main_default, lv_color_hex(0xffffff));
	lv_obj_add_style(lv_dropdown_get_list(ui->copynext_ddlist2), &style_copynext_ddlist2_extra_list_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_copynext_ddlist2_extra_list_scrollbar_default
	static lv_style_t style_copynext_ddlist2_extra_list_scrollbar_default;
	ui_init_style(&style_copynext_ddlist2_extra_list_scrollbar_default);
	
	lv_style_set_radius(&style_copynext_ddlist2_extra_list_scrollbar_default, 7);
	lv_style_set_bg_opa(&style_copynext_ddlist2_extra_list_scrollbar_default, 255);
	lv_style_set_bg_color(&style_copynext_ddlist2_extra_list_scrollbar_default, lv_color_hex(0x00ff00));
	lv_obj_add_style(lv_dropdown_get_list(ui->copynext_ddlist2), &style_copynext_ddlist2_extra_list_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Write codes copynext_btncopyback
	ui->copynext_btncopyback = lv_btn_create(ui->copynext);
	ui->copynext_btncopyback_label = lv_label_create(ui->copynext_btncopyback);
	lv_label_set_text(ui->copynext_btncopyback_label, "<");
	lv_label_set_long_mode(ui->copynext_btncopyback_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->copynext_btncopyback_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->copynext_btncopyback, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->copynext_btncopyback, 132, 66);
	lv_obj_set_size(ui->copynext_btncopyback, 76, 76);
	lv_obj_set_scrollbar_mode(ui->copynext_btncopyback, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_btncopyback, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_btncopyback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_btncopyback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_btncopyback, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_btncopyback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_btncopyback, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_btncopyback, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_btncopyback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_swcolor
	ui->copynext_swcolor = lv_switch_create(ui->copynext);
	lv_obj_set_pos(ui->copynext_swcolor, 861, 463);
	lv_obj_set_size(ui->copynext_swcolor, 106, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_swcolor, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_swcolor, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_swcolor, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_swcolor, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_swcolor, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_swcolor, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_swcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_swcolor, 275, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_swcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for copynext_swcolor, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->copynext_swcolor, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->copynext_swcolor, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_dir(ui->copynext_swcolor, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_color(ui->copynext_swcolor, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->copynext_swcolor, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for copynext_swcolor, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_swcolor, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_swcolor, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_swcolor, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_swcolor, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_swcolor, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_swcolor, 275, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes copynext_labelcopy
	ui->copynext_labelcopy = lv_label_create(ui->copynext);
	lv_label_set_text(ui->copynext_labelcopy, "Copies");
	lv_label_set_long_mode(ui->copynext_labelcopy, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->copynext_labelcopy, 922, 226);
	lv_obj_set_size(ui->copynext_labelcopy, 170, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_labelcopy, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_labelcopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_labelcopy, lv_color_hex(0x201818), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_labelcopy, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->copynext_labelcopy, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_labelcopy, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_up
	ui->copynext_up = lv_btn_create(ui->copynext);
	ui->copynext_up_label = lv_label_create(ui->copynext_up);
	lv_label_set_text(ui->copynext_up_label, "+");
	lv_label_set_long_mode(ui->copynext_up_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->copynext_up_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->copynext_up, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->copynext_up, 1111, 290);
	lv_obj_set_size(ui->copynext_up, 50, 50);
	lv_obj_set_scrollbar_mode(ui->copynext_up, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_up, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_up, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_up, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_up, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_up, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_up, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_up, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_up, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_up, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_up, &lv_font_simsun_45, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_up, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_down
	ui->copynext_down = lv_btn_create(ui->copynext);
	ui->copynext_down_label = lv_label_create(ui->copynext_down);
	lv_label_set_text(ui->copynext_down_label, "-");
	lv_label_set_long_mode(ui->copynext_down_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->copynext_down_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->copynext_down, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->copynext_down, 857, 290);
	lv_obj_set_size(ui->copynext_down, 50, 50);
	lv_obj_set_scrollbar_mode(ui->copynext_down, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_down, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_down, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_down, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_down, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_down, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_down, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_down, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_down, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_down, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_down, &lv_font_simsun_45, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_down, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_labelcnt
	ui->copynext_labelcnt = lv_label_create(ui->copynext);
	lv_label_set_text(ui->copynext_labelcnt, "1");
	lv_label_set_long_mode(ui->copynext_labelcnt, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->copynext_labelcnt, 927, 298);
	lv_obj_set_size(ui->copynext_labelcnt, 148, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_labelcnt, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_labelcnt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_labelcnt, lv_color_hex(0x0a0606), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_labelcnt, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->copynext_labelcnt, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_labelcnt, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_labelcnt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_labelcolor
	ui->copynext_labelcolor = lv_label_create(ui->copynext);
	lv_label_set_text(ui->copynext_labelcolor, "Color");
	lv_label_set_long_mode(ui->copynext_labelcolor, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->copynext_labelcolor, 836, 386);
	lv_obj_set_size(ui->copynext_labelcolor, 132, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_labelcolor, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_labelcolor, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_labelcolor, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_labelcolor, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->copynext_labelcolor, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_labelcolor, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_labelcolor, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_labelvert
	ui->copynext_labelvert = lv_label_create(ui->copynext);
	lv_label_set_text(ui->copynext_labelvert, "Vertical");
	lv_label_set_long_mode(ui->copynext_labelvert, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->copynext_labelvert, 1012, 386);
	lv_obj_set_size(ui->copynext_labelvert, 186, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_labelvert, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_labelvert, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_labelvert, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_labelvert, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->copynext_labelvert, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_labelvert, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_labelvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes copynext_swvert
	ui->copynext_swvert = lv_switch_create(ui->copynext);
	lv_obj_set_pos(ui->copynext_swvert, 1040, 463);
	lv_obj_set_size(ui->copynext_swvert, 106, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_swvert, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_swvert, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_swvert, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_swvert, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_swvert, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_swvert, lv_color_hex(0xd4d7d9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_swvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_swvert, 275, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_swvert, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for copynext_swvert, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->copynext_swvert, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->copynext_swvert, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_dir(ui->copynext_swvert, LV_GRAD_DIR_VER, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_color(ui->copynext_swvert, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->copynext_swvert, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for copynext_swvert, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_swvert, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_swvert, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_swvert, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_swvert, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_swvert, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_swvert, 275, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes copynext_ddlist1
	ui->copynext_ddlist1 = lv_dropdown_create(ui->copynext);
	lv_dropdown_set_options(ui->copynext_ddlist1, "Best\nNormal\nDraft");
	lv_obj_set_pos(ui->copynext_ddlist1, 82, 600);
	lv_obj_set_size(ui->copynext_ddlist1, 266, 52);
	lv_obj_set_scrollbar_mode(ui->copynext_ddlist1, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_ddlist1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_text_color(ui->copynext_ddlist1, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_ddlist1, &lv_font_arial_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_ddlist1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->copynext_ddlist1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->copynext_ddlist1, lv_color_hex(0xe1e6ee), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->copynext_ddlist1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->copynext_ddlist1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->copynext_ddlist1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_ddlist1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->copynext_ddlist1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_ddlist1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_ddlist1, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_ddlist1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_ddlist1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_CHECKED for &style_copynext_ddlist1_extra_list_selected_checked
	static lv_style_t style_copynext_ddlist1_extra_list_selected_checked;
	ui_init_style(&style_copynext_ddlist1_extra_list_selected_checked);
	
	lv_style_set_text_color(&style_copynext_ddlist1_extra_list_selected_checked, lv_color_hex(0xffffff));
	lv_style_set_text_font(&style_copynext_ddlist1_extra_list_selected_checked, &lv_font_montserratMedium_30);
	lv_style_set_border_width(&style_copynext_ddlist1_extra_list_selected_checked, 1);
	lv_style_set_border_opa(&style_copynext_ddlist1_extra_list_selected_checked, 255);
	lv_style_set_border_color(&style_copynext_ddlist1_extra_list_selected_checked, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_copynext_ddlist1_extra_list_selected_checked, 7);
	lv_style_set_bg_opa(&style_copynext_ddlist1_extra_list_selected_checked, 255);
	lv_style_set_bg_color(&style_copynext_ddlist1_extra_list_selected_checked, lv_color_hex(0x00a1b5));
	lv_obj_add_style(lv_dropdown_get_list(ui->copynext_ddlist1), &style_copynext_ddlist1_extra_list_selected_checked, LV_PART_SELECTED|LV_STATE_CHECKED);

	//Write style state: LV_STATE_DEFAULT for &style_copynext_ddlist1_extra_list_main_default
	static lv_style_t style_copynext_ddlist1_extra_list_main_default;
	ui_init_style(&style_copynext_ddlist1_extra_list_main_default);
	
	lv_style_set_max_height(&style_copynext_ddlist1_extra_list_main_default, 90);
	lv_style_set_text_color(&style_copynext_ddlist1_extra_list_main_default, lv_color_hex(0x0D3055));
	lv_style_set_text_font(&style_copynext_ddlist1_extra_list_main_default, &lv_font_arial_30);
	lv_style_set_border_width(&style_copynext_ddlist1_extra_list_main_default, 1);
	lv_style_set_border_opa(&style_copynext_ddlist1_extra_list_main_default, 255);
	lv_style_set_border_color(&style_copynext_ddlist1_extra_list_main_default, lv_color_hex(0xe1e6ee));
	lv_style_set_radius(&style_copynext_ddlist1_extra_list_main_default, 7);
	lv_style_set_bg_opa(&style_copynext_ddlist1_extra_list_main_default, 255);
	lv_style_set_bg_color(&style_copynext_ddlist1_extra_list_main_default, lv_color_hex(0xffffff));
	lv_style_set_bg_grad_dir(&style_copynext_ddlist1_extra_list_main_default, LV_GRAD_DIR_VER);
	lv_style_set_bg_grad_color(&style_copynext_ddlist1_extra_list_main_default, lv_color_hex(0xffffff));
	lv_obj_add_style(lv_dropdown_get_list(ui->copynext_ddlist1), &style_copynext_ddlist1_extra_list_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_copynext_ddlist1_extra_list_scrollbar_default
	static lv_style_t style_copynext_ddlist1_extra_list_scrollbar_default;
	ui_init_style(&style_copynext_ddlist1_extra_list_scrollbar_default);
	
	lv_style_set_radius(&style_copynext_ddlist1_extra_list_scrollbar_default, 7);
	lv_style_set_bg_opa(&style_copynext_ddlist1_extra_list_scrollbar_default, 255);
	lv_style_set_bg_color(&style_copynext_ddlist1_extra_list_scrollbar_default, lv_color_hex(0x00ff00));
	lv_obj_add_style(lv_dropdown_get_list(ui->copynext_ddlist1), &style_copynext_ddlist1_extra_list_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Write codes copynext_print
	ui->copynext_print = lv_btn_create(ui->copynext);
	ui->copynext_print_label = lv_label_create(ui->copynext_print);
	lv_label_set_text(ui->copynext_print_label, "PRINT");
	lv_label_set_long_mode(ui->copynext_print_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->copynext_print_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->copynext_print, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->copynext_print, 852, 589);
	lv_obj_set_size(ui->copynext_print, 313, 105);
	lv_obj_set_scrollbar_mode(ui->copynext_print, LV_SCROLLBAR_MODE_OFF);

	//Write style for copynext_print, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->copynext_print, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->copynext_print, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->copynext_print, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->copynext_print, lv_color_hex(0x4ab241), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->copynext_print, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->copynext_print, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->copynext_print, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->copynext_print, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->copynext_print, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->copynext_print, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->copynext);

	
}
