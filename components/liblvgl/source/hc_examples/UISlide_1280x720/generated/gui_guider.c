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


void ui_init_style(lv_style_t * style)
{
  if (style->prop_cnt > 1)
    lv_style_reset(style);
  else
    lv_style_init(style);
}

void init_scr_del_flag(lv_ui *ui)
{
  
	ui->home_del = true;
	ui->copyhome_del = true;
	ui->copynext_del = true;
	ui->scanhome_del = true;
	ui->prthome_del = true;
	ui->prtusb_del = true;
	ui->prtmb_del = true;
	ui->printit_del = true;
	ui->setup_del = true;
	ui->loader_del = true;
	ui->saved_del = true;
}

void setup_ui(lv_ui *ui)
{
  init_scr_del_flag(ui);
  setup_scr_home(ui);
  lv_scr_load(ui->home);
}
