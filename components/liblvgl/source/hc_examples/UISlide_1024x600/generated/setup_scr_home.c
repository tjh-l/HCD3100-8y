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


void setup_scr_home(lv_ui *ui)
{
	//Write codes home
	ui->home = lv_obj_create(NULL);
	lv_obj_set_size(ui->home, 1024, 600);
	lv_obj_set_scrollbar_mode(ui->home, LV_SCROLLBAR_MODE_OFF);

	//Write style for home, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_img_bg1
	ui->home_img_bg1 = lv_img_create(ui->home);
	lv_obj_add_flag(ui->home_img_bg1, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_bg1, &_blue_bg_alpha_1024x268);
	lv_img_set_pivot(ui->home_img_bg1, 50,50);
	lv_img_set_angle(ui->home_img_bg1, 0);
	lv_obj_set_pos(ui->home_img_bg1, 0, 0);
	lv_obj_set_size(ui->home_img_bg1, 1024, 268);
	lv_obj_set_scrollbar_mode(ui->home_img_bg1, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_bg1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_bg1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_img_bg2
	ui->home_img_bg2 = lv_img_create(ui->home);
	lv_obj_add_flag(ui->home_img_bg2, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_bg2, &_blue_bg_alpha_1024x260);
	lv_img_set_pivot(ui->home_img_bg2, 50,50);
	lv_img_set_angle(ui->home_img_bg2, 0);
	lv_obj_set_pos(ui->home_img_bg2, 0, 268);
	lv_obj_set_size(ui->home_img_bg2, 1024, 260);
	lv_obj_set_scrollbar_mode(ui->home_img_bg2, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_bg2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_bg2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_img_bg3
	ui->home_img_bg3 = lv_img_create(ui->home);
	lv_obj_add_flag(ui->home_img_bg3, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_bg3, &_blue_bg_alpha_1024x73);
	lv_img_set_pivot(ui->home_img_bg3, 50,50);
	lv_img_set_angle(ui->home_img_bg3, 0);
	lv_obj_set_pos(ui->home_img_bg3, 0, 528);
	lv_obj_set_size(ui->home_img_bg3, 1024, 73);
	lv_obj_set_scrollbar_mode(ui->home_img_bg3, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_bg3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_bg3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_slide_cont
	ui->home_slide_cont = lv_obj_create(ui->home);
	lv_obj_set_pos(ui->home_slide_cont, 36, 268);
	lv_obj_set_size(ui->home_slide_cont, 960, 260);
	lv_obj_set_scrollbar_mode(ui->home_slide_cont, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_slide_cont, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_slide_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgbtn_2
	ui->home_imgbtn_2 = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtn_2, LV_IMGBTN_STATE_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_2, LV_IMGBTN_STATE_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_2, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_2, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtn_2, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtn_2_label = lv_label_create(ui->home_imgbtn_2);
	lv_label_set_text(ui->home_imgbtn_2_label, "");
	lv_label_set_long_mode(ui->home_imgbtn_2_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtn_2_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtn_2, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtn_2, 979, 21);
	lv_obj_set_size(ui->home_imgbtn_2, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtn_2, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtn_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtn_2, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtn_2, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtn_2, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtn_2, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_2, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtn_2, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtn_2, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_2, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_label_2
	ui->home_label_2 = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_label_2, "SETUP");
	lv_label_set_long_mode(ui->home_label_2, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_label_2, 993, 161);
	lv_obj_set_size(ui->home_label_2, 160, 44);
	lv_obj_set_scrollbar_mode(ui->home_label_2, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_label_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_label_2, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_label_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_label_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_img_2
	ui->home_img_2 = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_img_2, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_2, &_setup_alpha_61x61);
	lv_img_set_pivot(ui->home_img_2, 0,0);
	lv_img_set_angle(ui->home_img_2, 0);
	lv_obj_set_pos(ui->home_img_2, 1064, 67);
	lv_obj_set_size(ui->home_img_2, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_img_2, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgbtn_6
	ui->home_imgbtn_6 = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtn_6, LV_IMGBTN_STATE_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_6, LV_IMGBTN_STATE_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_6, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_6, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtn_6, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtn_6_label = lv_label_create(ui->home_imgbtn_6);
	lv_label_set_text(ui->home_imgbtn_6_label, "");
	lv_label_set_long_mode(ui->home_imgbtn_6_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtn_6_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtn_6, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtn_6, 1740, 21);
	lv_obj_set_size(ui->home_imgbtn_6, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtn_6, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtn_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtn_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtn_6, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtn_6, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtn_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtn_6, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_6, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtn_6, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtn_6, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_6, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtn_6, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_6, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtn_6, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtn_6, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_6, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_img_6
	ui->home_img_6 = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_img_6, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_6, &_setup_alpha_61x61);
	lv_img_set_pivot(ui->home_img_6, 0,0);
	lv_img_set_angle(ui->home_img_6, 0);
	lv_obj_set_pos(ui->home_img_6, 1824, 74);
	lv_obj_set_size(ui->home_img_6, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_img_6, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_label_6
	ui->home_label_6 = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_label_6, "SETUP");
	lv_label_set_long_mode(ui->home_label_6, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_label_6, 1763, 161);
	lv_obj_set_size(ui->home_label_6, 160, 44);
	lv_obj_set_scrollbar_mode(ui->home_label_6, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_label_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_label_6, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_label_6, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_label_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_label_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgbtn_5
	ui->home_imgbtn_5 = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtn_5, LV_IMGBTN_STATE_RELEASED, NULL, &_btn3_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_5, LV_IMGBTN_STATE_PRESSED, NULL, &_btn3_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_5, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn3_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_5, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn3_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtn_5, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtn_5_label = lv_label_create(ui->home_imgbtn_5);
	lv_label_set_text(ui->home_imgbtn_5_label, "");
	lv_label_set_long_mode(ui->home_imgbtn_5_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtn_5_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtn_5, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtn_5, 1552, 21);
	lv_obj_set_size(ui->home_imgbtn_5, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtn_5, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtn_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtn_5, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtn_5, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtn_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtn_5, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_5, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtn_5, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtn_5, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_5, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtn_5, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_5, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtn_5, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtn_5, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_5, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_imgbtn_4
	ui->home_imgbtn_4 = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtn_4, LV_IMGBTN_STATE_RELEASED, NULL, &_btn2_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_4, LV_IMGBTN_STATE_PRESSED, NULL, &_btn2_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_4, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn2_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_4, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn2_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtn_4, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtn_4_label = lv_label_create(ui->home_imgbtn_4);
	lv_label_set_text(ui->home_imgbtn_4_label, "");
	lv_label_set_long_mode(ui->home_imgbtn_4_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtn_4_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtn_4, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtn_4, 1361, 21);
	lv_obj_set_size(ui->home_imgbtn_4, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtn_4, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtn_4, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtn_4, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtn_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtn_4, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_4, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtn_4, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtn_4, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_4, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtn_4, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_4, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtn_4, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtn_4, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_4, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_label_5
	ui->home_label_5 = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_label_5, "PRINT");
	lv_label_set_long_mode(ui->home_label_5, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_label_5, 1576, 161);
	lv_obj_set_size(ui->home_label_5, 149, 44);
	lv_obj_set_scrollbar_mode(ui->home_label_5, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_label_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_label_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_label_5, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_label_5, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_label_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_label_4
	ui->home_label_4 = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_label_4, "SCAN");
	lv_label_set_long_mode(ui->home_label_4, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_label_4, 1407, 161);
	lv_obj_set_size(ui->home_label_4, 128, 44);
	lv_obj_set_scrollbar_mode(ui->home_label_4, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_label_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_label_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_label_4, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_label_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_label_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_img_4
	ui->home_img_4 = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_img_4, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_4, &_scan_alpha_61x61);
	lv_img_set_pivot(ui->home_img_4, 0,0);
	lv_img_set_angle(ui->home_img_4, 0);
	lv_obj_set_pos(ui->home_img_4, 1441, 52);
	lv_obj_set_size(ui->home_img_4, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_img_4, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgbtn_3
	ui->home_imgbtn_3 = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtn_3, LV_IMGBTN_STATE_RELEASED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_3, LV_IMGBTN_STATE_PRESSED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_3, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_3, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtn_3, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtn_3_label = lv_label_create(ui->home_imgbtn_3);
	lv_label_set_text(ui->home_imgbtn_3_label, "");
	lv_label_set_long_mode(ui->home_imgbtn_3_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtn_3_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtn_3, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtn_3, 1170, 21);
	lv_obj_set_size(ui->home_imgbtn_3, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtn_3, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtn_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtn_3, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtn_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtn_3, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_3, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtn_3, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtn_3, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_3, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtn_3, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_3, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtn_3, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtn_3, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_3, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_label_3
	ui->home_label_3 = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_label_3, "COPY");
	lv_label_set_long_mode(ui->home_label_3, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_label_3, 1203, 161);
	lv_obj_set_size(ui->home_label_3, 132, 44);
	lv_obj_set_scrollbar_mode(ui->home_label_3, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_label_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_label_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_label_3, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_label_3, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_label_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_img_3
	ui->home_img_3 = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_img_3, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_3, &_copy_alpha_61x61);
	lv_img_set_pivot(ui->home_img_3, 0,0);
	lv_img_set_angle(ui->home_img_3, 0);
	lv_obj_set_pos(ui->home_img_3, 1263, 60);
	lv_obj_set_size(ui->home_img_3, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_img_3, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgbtn_1
	ui->home_imgbtn_1 = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtn_1, LV_IMGBTN_STATE_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_1, LV_IMGBTN_STATE_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_1, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtn_1, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtn_1, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtn_1_label = lv_label_create(ui->home_imgbtn_1);
	lv_label_set_text(ui->home_imgbtn_1_label, "");
	lv_label_set_long_mode(ui->home_imgbtn_1_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtn_1_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtn_1, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtn_1, 788, 21);
	lv_obj_set_size(ui->home_imgbtn_1, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtn_1, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtn_1, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtn_1, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtn_1, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtn_1, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtn_1, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtn_1, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_imgbtnscan
	ui->home_imgbtnscan = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtnscan, LV_IMGBTN_STATE_RELEASED, NULL, &_btn2_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnscan, LV_IMGBTN_STATE_PRESSED, NULL, &_btn2_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnscan, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn2_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnscan, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn2_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtnscan, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtnscan_label = lv_label_create(ui->home_imgbtnscan);
	lv_label_set_text(ui->home_imgbtnscan_label, "");
	lv_label_set_long_mode(ui->home_imgbtnscan_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtnscan_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtnscan, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtnscan, 215, 21);
	lv_obj_set_size(ui->home_imgbtnscan, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtnscan, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtnscan, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtnscan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtnscan, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtnscan, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtnscan, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtnscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtnscan, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtnscan, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtnscan, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtnscan, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtnscan, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtnscan, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtnscan, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtnscan, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtnscan, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtnscan, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_imgbtnset
	ui->home_imgbtnset = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtnset, LV_IMGBTN_STATE_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnset, LV_IMGBTN_STATE_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnset, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn4_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnset, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn4_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtnset, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtnset_label = lv_label_create(ui->home_imgbtnset);
	lv_label_set_text(ui->home_imgbtnset_label, "");
	lv_label_set_long_mode(ui->home_imgbtnset_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtnset_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtnset, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtnset, 597, 21);
	lv_obj_set_size(ui->home_imgbtnset, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtnset, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtnset, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtnset, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtnset, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtnset, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtnset, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtnset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtnset, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtnset, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtnset, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtnset, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtnset, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtnset, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtnset, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtnset, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtnset, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtnset, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_img_1
	ui->home_img_1 = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_img_1, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_1, &_setup_alpha_61x60);
	lv_img_set_pivot(ui->home_img_1, 0,0);
	lv_img_set_angle(ui->home_img_1, 0);
	lv_obj_set_pos(ui->home_img_1, 876, 57);
	lv_obj_set_size(ui->home_img_1, 61, 60);
	lv_obj_set_scrollbar_mode(ui->home_img_1, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_label_1
	ui->home_label_1 = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_label_1, "IMG BG");
	lv_label_set_long_mode(ui->home_label_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_label_1, 813, 161);
	lv_obj_set_size(ui->home_label_1, 160, 44);
	lv_obj_set_scrollbar_mode(ui->home_label_1, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_label_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_label_1, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_label_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgbtncopy
	ui->home_imgbtncopy = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtncopy, LV_IMGBTN_STATE_RELEASED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtncopy, LV_IMGBTN_STATE_PRESSED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtncopy, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtncopy, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn_bg_1_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtncopy, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtncopy_label = lv_label_create(ui->home_imgbtncopy);
	lv_label_set_text(ui->home_imgbtncopy_label, "");
	lv_label_set_long_mode(ui->home_imgbtncopy_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtncopy_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtncopy, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtncopy, 24, 21);
	lv_obj_set_size(ui->home_imgbtncopy, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtncopy, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtncopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtncopy, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtncopy, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtncopy, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtncopy, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtncopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtncopy, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtncopy, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtncopy, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtncopy, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtncopy, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtncopy, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtncopy, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtncopy, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtncopy, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtncopy, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_imgbtnprt
	ui->home_imgbtnprt = lv_imgbtn_create(ui->home_slide_cont);
	lv_imgbtn_set_src(ui->home_imgbtnprt, LV_IMGBTN_STATE_RELEASED, NULL, &_btn3_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnprt, LV_IMGBTN_STATE_PRESSED, NULL, &_btn3_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnprt, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn3_alpha_181x220, NULL);
	lv_imgbtn_set_src(ui->home_imgbtnprt, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn3_alpha_181x220, NULL);
	lv_obj_add_flag(ui->home_imgbtnprt, LV_OBJ_FLAG_CHECKABLE);
	ui->home_imgbtnprt_label = lv_label_create(ui->home_imgbtnprt);
	lv_label_set_text(ui->home_imgbtnprt_label, "");
	lv_label_set_long_mode(ui->home_imgbtnprt_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->home_imgbtnprt_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->home_imgbtnprt, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->home_imgbtnprt, 406, 21);
	lv_obj_set_size(ui->home_imgbtnprt, 181, 220);
	lv_obj_set_scrollbar_mode(ui->home_imgbtnprt, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgbtnprt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgbtnprt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_imgbtnprt, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_imgbtnprt, &lv_font_arial_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_imgbtnprt, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_imgbtnprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_imgbtnprt, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->home_imgbtnprt, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->home_imgbtnprt, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->home_imgbtnprt, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->home_imgbtnprt, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for home_imgbtnprt, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->home_imgbtnprt, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->home_imgbtnprt, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->home_imgbtnprt, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->home_imgbtnprt, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes home_imgset
	ui->home_imgset = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_imgset, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_imgset, &_setup_alpha_61x61);
	lv_img_set_pivot(ui->home_imgset, 0,0);
	lv_img_set_angle(ui->home_imgset, 0);
	lv_obj_set_pos(ui->home_imgset, 695, 51);
	lv_obj_set_size(ui->home_imgset, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_imgset, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgset, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgset, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgcopy
	ui->home_imgcopy = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_imgcopy, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_imgcopy, &_copy_alpha_61x61);
	lv_img_set_pivot(ui->home_imgcopy, 0,0);
	lv_img_set_angle(ui->home_imgcopy, 0);
	lv_obj_set_pos(ui->home_imgcopy, 107, 61);
	lv_obj_set_size(ui->home_imgcopy, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_imgcopy, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgcopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgcopy, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_labelcopy
	ui->home_labelcopy = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_labelcopy, "COPY");
	lv_label_set_long_mode(ui->home_labelcopy, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_labelcopy, 43, 161);
	lv_obj_set_size(ui->home_labelcopy, 132, 44);
	lv_obj_set_scrollbar_mode(ui->home_labelcopy, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_labelcopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_labelcopy, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_labelcopy, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_labelcopy, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_labelcopy, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_labelcopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgscan
	ui->home_imgscan = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_imgscan, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_imgscan, &_scan_alpha_61x61);
	lv_img_set_pivot(ui->home_imgscan, 0,0);
	lv_img_set_angle(ui->home_imgscan, 0);
	lv_obj_set_pos(ui->home_imgscan, 301, 63);
	lv_obj_set_size(ui->home_imgscan, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_imgscan, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgscan, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgscan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_labelscan
	ui->home_labelscan = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_labelscan, "SCAN");
	lv_label_set_long_mode(ui->home_labelscan, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_labelscan, 237, 161);
	lv_obj_set_size(ui->home_labelscan, 128, 44);
	lv_obj_set_scrollbar_mode(ui->home_labelscan, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_labelscan, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_labelscan, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_labelscan, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_labelscan, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_labelscan, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_labelscan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_imgprt
	ui->home_imgprt = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_imgprt, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_imgprt, &_print_alpha_61x61);
	lv_img_set_pivot(ui->home_imgprt, 0,0);
	lv_img_set_angle(ui->home_imgprt, 0);
	lv_obj_set_pos(ui->home_imgprt, 499, 58);
	lv_obj_set_size(ui->home_imgprt, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_imgprt, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_imgprt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_imgprt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_labelprt
	ui->home_labelprt = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_labelprt, "PRINT");
	lv_label_set_long_mode(ui->home_labelprt, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_labelprt, 435, 161);
	lv_obj_set_size(ui->home_labelprt, 149, 44);
	lv_obj_set_scrollbar_mode(ui->home_labelprt, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_labelprt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_labelprt, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_labelprt, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_labelprt, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_labelprt, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_labelprt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_labelset
	ui->home_labelset = lv_label_create(ui->home_slide_cont);
	lv_label_set_text(ui->home_labelset, "SETUP");
	lv_label_set_long_mode(ui->home_labelset, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_labelset, 616, 161);
	lv_obj_set_size(ui->home_labelset, 160, 44);
	lv_obj_set_scrollbar_mode(ui->home_labelset, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_labelset, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_labelset, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_labelset, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_labelset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_labelset, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_labelset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_img_5
	ui->home_img_5 = lv_img_create(ui->home_slide_cont);
	lv_obj_add_flag(ui->home_img_5, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_img_5, &_print_alpha_61x61);
	lv_img_set_pivot(ui->home_img_5, 0,0);
	lv_img_set_angle(ui->home_img_5, 0);
	lv_obj_set_pos(ui->home_img_5, 1646, 59);
	lv_obj_set_size(ui->home_img_5, 61, 61);
	lv_obj_set_scrollbar_mode(ui->home_img_5, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_img_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_img_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_cont1
	ui->home_cont1 = lv_obj_create(ui->home);
	lv_obj_set_pos(ui->home_cont1, 0, 0);
	lv_obj_set_size(ui->home_cont1, 1024, 220);
	lv_obj_set_scrollbar_mode(ui->home_cont1, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_cont1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_cont1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->home_cont1, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->home_cont1, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->home_cont1, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_labeldate
	ui->home_labeldate = lv_label_create(ui->home_cont1);
	lv_label_set_text(ui->home_labeldate, "20 Nov 2020 08:08");
	lv_label_set_long_mode(ui->home_labeldate, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_labeldate, 512, 66);
	lv_obj_set_size(ui->home_labeldate, 480, 40);
	lv_obj_set_scrollbar_mode(ui->home_labeldate, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_labeldate, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_labeldate, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_labeldate, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_labeldate, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_labeldate, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_labeldate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_pc
	ui->home_pc = lv_img_create(ui->home_cont1);
	lv_obj_add_flag(ui->home_pc, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_pc, &_pc_alpha_44x44);
	lv_img_set_pivot(ui->home_pc, 0,0);
	lv_img_set_angle(ui->home_pc, 0);
	lv_obj_set_pos(ui->home_pc, 422, 66);
	lv_obj_set_size(ui->home_pc, 44, 44);
	lv_obj_set_scrollbar_mode(ui->home_pc, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_pc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_pc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_eco
	ui->home_eco = lv_img_create(ui->home_cont1);
	lv_obj_add_flag(ui->home_eco, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_eco, &_eco_alpha_44x44);
	lv_img_set_pivot(ui->home_eco, 0,0);
	lv_img_set_angle(ui->home_eco, 0);
	lv_obj_set_pos(ui->home_eco, 313, 66);
	lv_obj_set_size(ui->home_eco, 44, 44);
	lv_obj_set_scrollbar_mode(ui->home_eco, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_eco, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_eco, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_wifi
	ui->home_wifi = lv_img_create(ui->home_cont1);
	lv_obj_add_flag(ui->home_wifi, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_wifi, &_wifi_alpha_61x41);
	lv_img_set_pivot(ui->home_wifi, 0,0);
	lv_img_set_angle(ui->home_wifi, 0);
	lv_obj_set_pos(ui->home_wifi, 119, 68);
	lv_obj_set_size(ui->home_wifi, 61, 41);
	lv_obj_set_scrollbar_mode(ui->home_wifi, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_wifi, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_wifi, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_tel
	ui->home_tel = lv_img_create(ui->home_cont1);
	lv_obj_add_flag(ui->home_tel, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_tel, &_tel_alpha_44x44);
	lv_img_set_pivot(ui->home_tel, 0,0);
	lv_img_set_angle(ui->home_tel, 0);
	lv_obj_set_pos(ui->home_tel, 224, 66);
	lv_obj_set_size(ui->home_tel, 44, 44);
	lv_obj_set_scrollbar_mode(ui->home_tel, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_tel, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_tel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_sw_img_bg
	ui->home_sw_img_bg = lv_switch_create(ui->home_cont1);
	lv_obj_set_pos(ui->home_sw_img_bg, 924, 143);
	lv_obj_set_size(ui->home_sw_img_bg, 84, 35);
	lv_obj_set_scrollbar_mode(ui->home_sw_img_bg, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_sw_img_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home_sw_img_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->home_sw_img_bg, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->home_sw_img_bg, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->home_sw_img_bg, 153, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->home_sw_img_bg, lv_color_hex(0x00bdff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_sw_img_bg, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_sw_img_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_sw_img_bg, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->home_sw_img_bg, 223, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->home_sw_img_bg, lv_color_hex(0x878787), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->home_sw_img_bg, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for home_sw_img_bg, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home_sw_img_bg, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->home_sw_img_bg, lv_color_hex(0xf78585), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->home_sw_img_bg, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_sw_img_bg, 100, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write style for home_sw_img_bg, Part: LV_PART_KNOB, State: LV_STATE_FOCUSED.
	lv_obj_set_style_bg_opa(ui->home_sw_img_bg, 255, LV_PART_KNOB|LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(ui->home_sw_img_bg, lv_color_hex(0x878787), LV_PART_KNOB|LV_STATE_FOCUSED);
	lv_obj_set_style_border_width(ui->home_sw_img_bg, 0, LV_PART_KNOB|LV_STATE_FOCUSED);
	lv_obj_set_style_radius(ui->home_sw_img_bg, 100, LV_PART_KNOB|LV_STATE_FOCUSED);

	//Write codes home_sw_img_bg_label
	ui->home_sw_img_bg_label = lv_label_create(ui->home_cont1);
	lv_label_set_text(ui->home_sw_img_bg_label, "Wallpaper");
	lv_label_set_long_mode(ui->home_sw_img_bg_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_sw_img_bg_label, 701, 139);
	lv_obj_set_size(ui->home_sw_img_bg_label, 226, 51);
	lv_obj_set_scrollbar_mode(ui->home_sw_img_bg_label, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_sw_img_bg_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_sw_img_bg_label, lv_color_hex(0xfefefe), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_sw_img_bg_label, &lv_font_montserratMedium_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_sw_img_bg_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_sw_img_bg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_sw_img_bg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->home);

	lv_obj_add_flag(guider_ui.home_img_bg1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(guider_ui.home_img_bg2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(guider_ui.home_img_bg3, LV_OBJ_FLAG_HIDDEN);
	//Init events for screen.
	events_init_home(ui);
}
