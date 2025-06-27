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


void setup_scr_loader(lv_ui *ui)
{
	//Write codes loader
	ui->loader = lv_obj_create(NULL);
	ui->g_kb_loader = lv_keyboard_create(ui->loader);
	lv_obj_add_event_cb(ui->g_kb_loader, kb_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_add_flag(ui->g_kb_loader, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_text_font(ui->g_kb_loader, &lv_font_simsun_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_size(ui->loader, 1024, 600);
	lv_obj_set_scrollbar_mode(ui->loader, LV_SCROLLBAR_MODE_OFF);

	//Write style for loader, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->loader, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes loader_cont0
	ui->loader_cont0 = lv_obj_create(ui->loader);
	lv_obj_set_pos(ui->loader_cont0, 0, 0);
	lv_obj_set_size(ui->loader_cont0, 1024, 600);
	lv_obj_set_scrollbar_mode(ui->loader_cont0, LV_SCROLLBAR_MODE_OFF);

	//Write style for loader_cont0, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->loader_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->loader_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->loader_cont0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->loader_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->loader_cont0, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->loader_cont0, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->loader_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->loader_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->loader_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->loader_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->loader_cont0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes loader_loadarc
	ui->loader_loadarc = lv_arc_create(ui->loader);
	lv_arc_set_mode(ui->loader_loadarc, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui->loader_loadarc, 0, 100);
	lv_arc_set_bg_angles(ui->loader_loadarc, 0, 360);
	lv_arc_set_angles(ui->loader_loadarc, 271, 271);
	lv_arc_set_rotation(ui->loader_loadarc, 0);
	lv_obj_set_style_arc_rounded(ui->loader_loadarc, 0,  LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_rounded(ui->loader_loadarc, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->loader_loadarc, 384, 176);
	lv_obj_set_size(ui->loader_loadarc, 234, 234);
	lv_obj_set_scrollbar_mode(ui->loader_loadarc, LV_SCROLLBAR_MODE_OFF);

	//Write style for loader_loadarc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->loader_loadarc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->loader_loadarc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_width(ui->loader_loadarc, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->loader_loadarc, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->loader_loadarc, 13, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->loader_loadarc, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->loader_loadarc, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->loader_loadarc, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->loader_loadarc, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->loader_loadarc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for loader_loadarc, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_arc_width(ui->loader_loadarc, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->loader_loadarc, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for loader_loadarc, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->loader_loadarc, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->loader_loadarc, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->loader_loadarc, LV_GRAD_DIR_VER, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->loader_loadarc, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->loader_loadarc, 5, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes loader_loadlabel
	ui->loader_loadlabel = lv_label_create(ui->loader);
	lv_label_set_text(ui->loader_loadlabel, "0 %");
	lv_label_set_long_mode(ui->loader_loadlabel, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->loader_loadlabel, 428, 275);
	lv_obj_set_size(ui->loader_loadlabel, 170, 44);
	lv_obj_set_scrollbar_mode(ui->loader_loadlabel, LV_SCROLLBAR_MODE_OFF);

	//Write style for loader_loadlabel, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->loader_loadlabel, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->loader_loadlabel, &lv_font_arial_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->loader_loadlabel, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->loader_loadlabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->loader_loadlabel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->loader);

	
}
