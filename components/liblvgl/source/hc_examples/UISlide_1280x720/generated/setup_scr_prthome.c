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


void setup_scr_prthome(lv_ui *ui)
{
	//Write codes prthome
	ui->prthome = lv_obj_create(NULL);
	lv_obj_set_size(ui->prthome, 1280, 720);
	lv_obj_set_scrollbar_mode(ui->prthome, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prthome, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_cont0
	ui->prthome_cont0 = lv_obj_create(ui->prthome);
	lv_obj_set_pos(ui->prthome_cont0, 0, 0);
	lv_obj_set_size(ui->prthome_cont0, 1280, 264);
	lv_obj_set_scrollbar_mode(ui->prthome_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prthome_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prthome_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prthome_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_cont3
	ui->prthome_cont3 = lv_obj_create(ui->prthome);
	lv_obj_set_pos(ui->prthome_cont3, 0, 264);
	lv_obj_set_size(ui->prthome_cont3, 1280, 454);
	lv_obj_set_scrollbar_mode(ui->prthome_cont3, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_cont3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_cont3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_cont3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_cont3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prthome_cont3, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prthome_cont3, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prthome_cont3, lv_color_hex(0xdedede), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_cont3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_cont3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_cont3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_cont3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_cont3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_cont1
	ui->prthome_cont1 = lv_obj_create(ui->prthome);
	lv_obj_set_pos(ui->prthome_cont1, 106, 158);
	lv_obj_set_size(ui->prthome_cont1, 1066, 369);
	lv_obj_set_scrollbar_mode(ui->prthome_cont1, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_cont1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_cont1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prthome_cont1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prthome_cont1, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prthome_cont1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_cont1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_label4
	ui->prthome_label4 = lv_label_create(ui->prthome);
	lv_label_set_text(ui->prthome_label4, "PRINT MENU");
	lv_label_set_long_mode(ui->prthome_label4, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prthome_label4, 450, 42);
	lv_obj_set_size(ui->prthome_label4, 365, 79);
	lv_obj_set_scrollbar_mode(ui->prthome_label4, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_label4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_label4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_label4, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prthome_label4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_label4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_label4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_imgbtnit
	ui->prthome_imgbtnit = lv_imgbtn_create(ui->prthome);
	lv_imgbtn_set_src(ui->prthome_imgbtnit, LV_IMGBTN_STATE_RELEASED, NULL, &_btn4_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnit, LV_IMGBTN_STATE_PRESSED, NULL, &_btn4_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnit, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn4_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnit, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn4_alpha_306x369, NULL);
	lv_obj_add_flag(ui->prthome_imgbtnit, LV_OBJ_FLAG_CHECKABLE);
	ui->prthome_imgbtnit_label = lv_label_create(ui->prthome_imgbtnit);
	lv_label_set_text(ui->prthome_imgbtnit_label, "");
	lv_label_set_long_mode(ui->prthome_imgbtnit_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prthome_imgbtnit_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prthome_imgbtnit, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prthome_imgbtnit, 866, 158);
	lv_obj_set_size(ui->prthome_imgbtnit, 306, 369);
	lv_obj_set_scrollbar_mode(ui->prthome_imgbtnit, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_imgbtnit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_imgbtnit, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_imgbtnit, &lv_font_arial_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_imgbtnit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for prthome_imgbtnit, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnit, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->prthome_imgbtnit, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->prthome_imgbtnit, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnit, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for prthome_imgbtnit, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnit, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->prthome_imgbtnit, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->prthome_imgbtnit, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnit, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes prthome_imgbtnusb
	ui->prthome_imgbtnusb = lv_imgbtn_create(ui->prthome);
	lv_imgbtn_set_src(ui->prthome_imgbtnusb, LV_IMGBTN_STATE_RELEASED, NULL, &_btn2_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnusb, LV_IMGBTN_STATE_PRESSED, NULL, &_btn2_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnusb, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn2_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnusb, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn2_alpha_306x369, NULL);
	lv_obj_add_flag(ui->prthome_imgbtnusb, LV_OBJ_FLAG_CHECKABLE);
	ui->prthome_imgbtnusb_label = lv_label_create(ui->prthome_imgbtnusb);
	lv_label_set_text(ui->prthome_imgbtnusb_label, "");
	lv_label_set_long_mode(ui->prthome_imgbtnusb_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prthome_imgbtnusb_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prthome_imgbtnusb, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prthome_imgbtnusb, 106, 158);
	lv_obj_set_size(ui->prthome_imgbtnusb, 306, 369);
	lv_obj_set_scrollbar_mode(ui->prthome_imgbtnusb, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_imgbtnusb, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnusb, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_imgbtnusb, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_imgbtnusb, &lv_font_arial_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_imgbtnusb, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for prthome_imgbtnusb, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnusb, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->prthome_imgbtnusb, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->prthome_imgbtnusb, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnusb, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for prthome_imgbtnusb, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnusb, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->prthome_imgbtnusb, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->prthome_imgbtnusb, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnusb, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes prthome_imgbtnmobile
	ui->prthome_imgbtnmobile = lv_imgbtn_create(ui->prthome);
	lv_imgbtn_set_src(ui->prthome_imgbtnmobile, LV_IMGBTN_STATE_RELEASED, NULL, &_btn3_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnmobile, LV_IMGBTN_STATE_PRESSED, NULL, &_btn3_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnmobile, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &_btn3_alpha_306x369, NULL);
	lv_imgbtn_set_src(ui->prthome_imgbtnmobile, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &_btn3_alpha_306x369, NULL);
	lv_obj_add_flag(ui->prthome_imgbtnmobile, LV_OBJ_FLAG_CHECKABLE);
	ui->prthome_imgbtnmobile_label = lv_label_create(ui->prthome_imgbtnmobile);
	lv_label_set_text(ui->prthome_imgbtnmobile_label, "");
	lv_label_set_long_mode(ui->prthome_imgbtnmobile_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prthome_imgbtnmobile_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prthome_imgbtnmobile, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prthome_imgbtnmobile, 487, 158);
	lv_obj_set_size(ui->prthome_imgbtnmobile, 306, 369);
	lv_obj_set_scrollbar_mode(ui->prthome_imgbtnmobile, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_imgbtnmobile, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnmobile, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_imgbtnmobile, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_imgbtnmobile, &lv_font_arial_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_imgbtnmobile, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for prthome_imgbtnmobile, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnmobile, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->prthome_imgbtnmobile, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui->prthome_imgbtnmobile, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnmobile, 0, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write style for prthome_imgbtnmobile, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_img_opa(ui->prthome_imgbtnmobile, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_color(ui->prthome_imgbtnmobile, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->prthome_imgbtnmobile, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->prthome_imgbtnmobile, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	//Write codes prthome_labelusb
	ui->prthome_labelusb = lv_label_create(ui->prthome);
	lv_label_set_text(ui->prthome_labelusb, "USB");
	lv_label_set_long_mode(ui->prthome_labelusb, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prthome_labelusb, 153, 422);
	lv_obj_set_size(ui->prthome_labelusb, 196, 52);
	lv_obj_set_scrollbar_mode(ui->prthome_labelusb, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_labelusb, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_labelusb, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_labelusb, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prthome_labelusb, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_labelusb, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_labelusb, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_labelmobile
	ui->prthome_labelmobile = lv_label_create(ui->prthome);
	lv_label_set_text(ui->prthome_labelmobile, "MOBILE");
	lv_label_set_long_mode(ui->prthome_labelmobile, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prthome_labelmobile, 527, 422);
	lv_obj_set_size(ui->prthome_labelmobile, 196, 52);
	lv_obj_set_scrollbar_mode(ui->prthome_labelmobile, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_labelmobile, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_labelmobile, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_labelmobile, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prthome_labelmobile, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_labelmobile, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_labelmobile, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_labelit
	ui->prthome_labelit = lv_label_create(ui->prthome);
	lv_label_set_text(ui->prthome_labelit, "INTERNET");
	lv_label_set_long_mode(ui->prthome_labelit, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prthome_labelit, 906, 422);
	lv_obj_set_size(ui->prthome_labelit, 226, 52);
	lv_obj_set_scrollbar_mode(ui->prthome_labelit, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_labelit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_labelit, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_labelit, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prthome_labelit, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_labelit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_labelit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_label2
	ui->prthome_label2 = lv_label_create(ui->prthome);
	lv_label_set_text(ui->prthome_label2, "From where do you want to print ?");
	lv_label_set_long_mode(ui->prthome_label2, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->prthome_label2, 42, 576);
	lv_obj_set_size(ui->prthome_label2, 1172, 79);
	lv_obj_set_scrollbar_mode(ui->prthome_label2, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_label2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_label2, lv_color_hex(0x251d1d), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_label2, &lv_font_arial_40, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->prthome_label2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_label2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_usb
	ui->prthome_usb = lv_img_create(ui->prthome);
	lv_obj_add_flag(ui->prthome_usb, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->prthome_usb, &_usb_alpha_76x76);
	lv_img_set_pivot(ui->prthome_usb, 0,0);
	lv_img_set_angle(ui->prthome_usb, 0);
	lv_obj_set_pos(ui->prthome_usb, 266, 224);
	lv_obj_set_size(ui->prthome_usb, 76, 76);
	lv_obj_set_scrollbar_mode(ui->prthome_usb, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_usb, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prthome_usb, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_mobile
	ui->prthome_mobile = lv_img_create(ui->prthome);
	lv_obj_add_flag(ui->prthome_mobile, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->prthome_mobile, &_mobile_alpha_76x76);
	lv_img_set_pivot(ui->prthome_mobile, 0,0);
	lv_img_set_angle(ui->prthome_mobile, 0);
	lv_obj_set_pos(ui->prthome_mobile, 645, 224);
	lv_obj_set_size(ui->prthome_mobile, 76, 76);
	lv_obj_set_scrollbar_mode(ui->prthome_mobile, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_mobile, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prthome_mobile, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_internet
	ui->prthome_internet = lv_img_create(ui->prthome);
	lv_obj_add_flag(ui->prthome_internet, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->prthome_internet, &_internet_alpha_76x76);
	lv_img_set_pivot(ui->prthome_internet, 0,0);
	lv_img_set_angle(ui->prthome_internet, 0);
	lv_obj_set_pos(ui->prthome_internet, 1021, 224);
	lv_obj_set_size(ui->prthome_internet, 76, 76);
	lv_obj_set_scrollbar_mode(ui->prthome_internet, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_internet, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->prthome_internet, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes prthome_btnprintback
	ui->prthome_btnprintback = lv_btn_create(ui->prthome);
	ui->prthome_btnprintback_label = lv_label_create(ui->prthome_btnprintback);
	lv_label_set_text(ui->prthome_btnprintback_label, "<");
	lv_label_set_long_mode(ui->prthome_btnprintback_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->prthome_btnprintback_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->prthome_btnprintback, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->prthome_btnprintback, 132, 33);
	lv_obj_set_size(ui->prthome_btnprintback, 76, 76);
	lv_obj_set_scrollbar_mode(ui->prthome_btnprintback, LV_SCROLLBAR_MODE_OFF);

	//Write style for prthome_btnprintback, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->prthome_btnprintback, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->prthome_btnprintback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->prthome_btnprintback, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->prthome_btnprintback, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->prthome_btnprintback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->prthome_btnprintback, 137, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->prthome_btnprintback, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->prthome_btnprintback, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->prthome_btnprintback, &lv_font_simsun_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->prthome_btnprintback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->prthome);

	
}
