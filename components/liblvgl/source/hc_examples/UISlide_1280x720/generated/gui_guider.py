# Copyright 2023 NXP
# NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
# accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
# activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
# comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
# terms, then you may not retain, install, activate or otherwise use the software.

import SDL
import utime as time
import usys as sys
import lvgl as lv
import lodepng as png
import ustruct
import fs_driver

lv.init()
SDL.init(w=1024,h=600)

# Register SDL display driver.
disp_buf1 = lv.disp_draw_buf_t()
buf1_1 = bytearray(1024*10)
disp_buf1.init(buf1_1, None, len(buf1_1)//4)
disp_drv = lv.disp_drv_t()
disp_drv.init()
disp_drv.draw_buf = disp_buf1
disp_drv.flush_cb = SDL.monitor_flush
disp_drv.hor_res = 1024
disp_drv.ver_res = 600
disp_drv.register()

# Regsiter SDL mouse driver
indev_drv = lv.indev_drv_t()
indev_drv.init()
indev_drv.type = lv.INDEV_TYPE.POINTER
indev_drv.read_cb = SDL.mouse_read
indev_drv.register()

fs_drv = lv.fs_drv_t()
fs_driver.fs_register(fs_drv, 'Z')

# Below: Taken from https://github.com/lvgl/lv_binding_micropython/blob/master/driver/js/imagetools.py#L22-L94

COLOR_SIZE = lv.color_t.__SIZE__
COLOR_IS_SWAPPED = hasattr(lv.color_t().ch,'green_h')

class lodepng_error(RuntimeError):
    def __init__(self, err):
        if type(err) is int:
            super().__init__(png.error_text(err))
        else:
            super().__init__(err)

# Parse PNG file header
# Taken from https://github.com/shibukawa/imagesize_py/blob/ffef30c1a4715c5acf90e8945ceb77f4a2ed2d45/imagesize.py#L63-L85

def get_png_info(decoder, src, header):
    # Only handle variable image types

    if lv.img.src_get_type(src) != lv.img.SRC.VARIABLE:
        return lv.RES.INV

    data = lv.img_dsc_t.__cast__(src).data
    if data == None:
        return lv.RES.INV

    png_header = bytes(data.__dereference__(24))

    if png_header.startswith(b'\211PNG\r\n\032\n'):
        if png_header[12:16] == b'IHDR':
            start = 16
        # Maybe this is for an older PNG version.
        else:
            start = 8
        try:
            width, height = ustruct.unpack(">LL", png_header[start:start+8])
        except ustruct.error:
            return lv.RES.INV
    else:
        return lv.RES.INV

    header.always_zero = 0
    header.w = width
    header.h = height
    header.cf = lv.img.CF.TRUE_COLOR_ALPHA

    return lv.RES.OK

def convert_rgba8888_to_bgra8888(img_view):
    for i in range(0, len(img_view), lv.color_t.__SIZE__):
        ch = lv.color_t.__cast__(img_view[i:i]).ch
        ch.red, ch.blue = ch.blue, ch.red

# Read and parse PNG file

def open_png(decoder, dsc):
    img_dsc = lv.img_dsc_t.__cast__(dsc.src)
    png_data = img_dsc.data
    png_size = img_dsc.data_size
    png_decoded = png.C_Pointer()
    png_width = png.C_Pointer()
    png_height = png.C_Pointer()
    error = png.decode32(png_decoded, png_width, png_height, png_data, png_size)
    if error:
        raise lodepng_error(error)
    img_size = png_width.int_val * png_height.int_val * 4
    img_data = png_decoded.ptr_val
    img_view = img_data.__dereference__(img_size)

    if COLOR_SIZE == 4:
        convert_rgba8888_to_bgra8888(img_view)
    else:
        raise lodepng_error("Error: Color mode not supported yet!")

    dsc.img_data = img_data
    return lv.RES.OK

# Above: Taken from https://github.com/lvgl/lv_binding_micropython/blob/master/driver/js/imagetools.py#L22-L94

decoder = lv.img.decoder_create()
decoder.info_cb = get_png_info
decoder.open_cb = open_png

def anim_x_cb(obj, v):
    obj.set_x(v)

def anim_y_cb(obj, v):
    obj.set_y(v)

def anim_width_cb(obj, v):
    obj.set_width(v)

def anim_height_cb(obj, v):
    obj.set_height(v)

def anim_img_zoom_cb(obj, v):
    obj.set_zoom(v)

def anim_img_rotate_cb(obj, v):
    obj.set_angle(v)

global_font_cache = {}
def test_font(font_family, font_size):
    global global_font_cache
    if font_family + str(font_size) in global_font_cache:
        return global_font_cache[font_family + str(font_size)]
    if font_size % 2:
        candidates = [
            (font_family, font_size),
            (font_family, font_size-font_size%2),
            (font_family, font_size+font_size%2),
            ("montserrat", font_size-font_size%2),
            ("montserrat", font_size+font_size%2),
            ("montserrat", 16)
        ]
    else:
        candidates = [
            (font_family, font_size),
            ("montserrat", font_size),
            ("montserrat", 16)
        ]
    for (family, size) in candidates:
        try:
            if eval(f'lv.font_{family}_{size}'):
                global_font_cache[font_family + str(font_size)] = eval(f'lv.font_{family}_{size}')
                if family != font_family or size != font_size:
                    print(f'WARNING: lv.font_{family}_{size} is used!')
                return eval(f'lv.font_{family}_{size}')
        except AttributeError:
            try:
                load_font = lv.font_load(f"Z:MicroPython/lv_font_{family}_{size}.fnt")
                global_font_cache[font_family + str(font_size)] = load_font
                return load_font
            except:
                if family == font_family and size == font_size:
                    print(f'WARNING: lv.font_{family}_{size} is NOT supported!')

global_image_cache = {}
def load_image(file):
    global global_image_cache
    if file in global_image_cache:
        return global_image_cache[file]
    try:
        with open(file,'rb') as f:
            data = f.read()
    except:
        print(f'Could not open {file}')
        sys.exit()

    img = lv.img_dsc_t({
        'data_size': len(data),
        'data': data
    })
    global_image_cache[file] = img
    return img

def calendar_event_handler(e,obj):
    code = e.get_code()

    if code == lv.EVENT.VALUE_CHANGED:
        source = e.get_current_target()
        date = lv.calendar_date_t()
        if source.get_pressed_date(date) == lv.RES.OK:
            source.set_highlighted_dates([date], 1)

def spinbox_increment_event_cb(e, obj):
    code = e.get_code()
    if code == lv.EVENT.SHORT_CLICKED or code == lv.EVENT.LONG_PRESSED_REPEAT:
        obj.increment()
def spinbox_decrement_event_cb(e, obj):
    code = e.get_code()
    if code == lv.EVENT.SHORT_CLICKED or code == lv.EVENT.LONG_PRESSED_REPEAT:
        obj.decrement()

def digital_clock_cb(timer, obj, current_time, show_second, use_ampm):
    hour = int(current_time[0])
    minute = int(current_time[1])
    second = int(current_time[2])
    ampm = current_time[3]
    second = second + 1
    if second == 60:
        second = 0
        minute = minute + 1
        if minute == 60:
            minute = 0
            hour = hour + 1
            if use_ampm:
                if hour == 12:
                    if ampm == 'AM':
                        ampm = 'PM'
                    elif ampm == 'PM':
                        ampm = 'AM'
                if hour > 12:
                    hour = hour % 12
    hour = hour % 24
    if use_ampm:
        if show_second:
            obj.set_text("%02d:%02d:%02d %s" %(hour, minute, second, ampm))
        else:
            obj.set_text("%02d:%02d %s" %(hour, minute, ampm))
    else:
        if show_second:
            obj.set_text("%02d:%02d:%02d" %(hour, minute, second))
        else:
            obj.set_text("%02d:%02d" %(hour, minute))
    current_time[0] = hour
    current_time[1] = minute
    current_time[2] = second
    current_time[3] = ampm

def analog_clock_cb(timer, obj):
    datetime = time.localtime()
    hour = datetime[3]
    if hour >= 12: hour = hour - 12
    obj.set_time(hour, datetime[4], datetime[5])

def datetext_event_handler(e, obj):
    code = e.get_code()
    target = e.get_target()
    if code == lv.EVENT.FOCUSED:
        if obj is None:
            bg = lv.layer_top()
            bg.add_flag(lv.obj.FLAG.CLICKABLE)
            obj = lv.calendar(bg)
            scr = target.get_screen()
            scr_height = scr.get_height()
            scr_width = scr.get_width()
            obj.set_size(int(scr_width * 0.8), int(scr_height * 0.8))
            datestring = target.get_text()
            year = int(datestring.split('/')[0])
            month = int(datestring.split('/')[1])
            day = int(datestring.split('/')[2])
            obj.set_showed_date(year, month)
            highlighted_days=[lv.calendar_date_t({'year':year, 'month':month, 'day':day})]
            obj.set_highlighted_dates(highlighted_days, 1)
            obj.align(lv.ALIGN.CENTER, 0, 0)
            lv.calendar_header_arrow(obj)
            obj.add_event_cb(lambda e: datetext_calendar_event_handler(e, target), lv.EVENT.ALL, None)
            scr.update_layout()

def datetext_calendar_event_handler(e, obj):
    code = e.get_code()
    target = e.get_current_target()
    if code == lv.EVENT.VALUE_CHANGED:
        date = lv.calendar_date_t()
        if target.get_pressed_date(date) == lv.RES.OK:
            obj.set_text(f"{date.year}/{date.month}/{date.day}")
            bg = lv.layer_top()
            bg.clear_flag(lv.obj.FLAG.CLICKABLE)
            bg.set_style_bg_opa(lv.OPA.TRANSP, 0)
            target.delete()

def home_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def copyhome_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def copynext_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def scanhome_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def prthome_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def prtusb_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def prtmb_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def printit_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def setup_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def loader_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

def saved_ta_event_cb(e,kb):
    code = e.get_code()
    ta = e.get_target()
    if code == lv.EVENT.FOCUSED:
        kb.set_textarea(ta)
        kb.move_foreground()
        kb.clear_flag(lv.obj.FLAG.HIDDEN)

    if code == lv.EVENT.DEFOCUSED:
        kb.set_textarea(None)
        kb.move_background()
        kb.add_flag(lv.obj.FLAG.HIDDEN)

# Create home
home = lv.obj()
home.set_size(1024, 600)
home.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_cont1
home_cont1 = lv.obj(home)
home_cont1.set_pos(0, 0)
home_cont1.set_size(1024, 220)
home_cont1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_cont1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_cont1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_cont1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_labeldate
home_labeldate = lv.label(home)
home_labeldate.set_text("20 Nov 2020 08:08")
home_labeldate.set_long_mode(lv.label.LONG.WRAP)
home_labeldate.set_pos(512, 66)
home_labeldate.set_size(480, 66)
home_labeldate.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_labeldate, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_labeldate.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labeldate.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtncopy
home_imgbtncopy = lv.imgbtn(home)
home_imgbtncopy.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtncopy.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtncopy.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtncopy.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtncopy.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtncopy_label = lv.label(home_imgbtncopy)
home_imgbtncopy_label.set_text("")
home_imgbtncopy_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtncopy_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtncopy.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtncopy.set_pos(103, 264)
home_imgbtncopy.set_size(181, 206)
home_imgbtncopy.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtncopy, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtncopy.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtncopy.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtncopy.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtncopy.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtncopy.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtncopy, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtncopy.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtncopy.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtncopy.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtncopy.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtncopy, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtncopy.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtncopy.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtncopy.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtncopy.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_labelcopy
home_labelcopy = lv.label(home)
home_labelcopy.set_text("COPY")
home_labelcopy.set_long_mode(lv.label.LONG.WRAP)
home_labelcopy.set_pos(125, 398)
home_labelcopy.set_size(132, 44)
home_labelcopy.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_labelcopy, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_labelcopy.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelcopy.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtnset
home_imgbtnset = lv.imgbtn(home)
home_imgbtnset.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtnset.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtnset.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtnset.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtnset.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtnset_label = lv.label(home_imgbtnset)
home_imgbtnset_label.set_text("")
home_imgbtnset_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtnset_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtnset.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtnset.set_pos(679, 255)
home_imgbtnset.set_size(181, 220)
home_imgbtnset.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtnset, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtnset.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnset.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnset.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnset.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnset.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtnset, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtnset.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnset.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnset.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnset.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtnset, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtnset.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnset.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnset.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnset.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_imgbtnscan
home_imgbtnscan = lv.imgbtn(home)
home_imgbtnscan.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtnscan.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtnscan.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtnscan.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtnscan.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtnscan_label = lv.label(home_imgbtnscan)
home_imgbtnscan_label.set_text("")
home_imgbtnscan_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtnscan_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtnscan.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtnscan.set_pos(295, 255)
home_imgbtnscan.set_size(181, 220)
home_imgbtnscan.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtnscan, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtnscan.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnscan.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnscan.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnscan.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnscan.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtnscan, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtnscan.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnscan.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnscan.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnscan.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtnscan, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtnscan.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnscan.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnscan.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnscan.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_imgbtnprt
home_imgbtnprt = lv.imgbtn(home)
home_imgbtnprt.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtnprt.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtnprt.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtnprt.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtnprt.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtnprt_label = lv.label(home_imgbtnprt)
home_imgbtnprt_label.set_text("")
home_imgbtnprt_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtnprt_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtnprt.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtnprt.set_pos(487, 255)
home_imgbtnprt.set_size(181, 220)
home_imgbtnprt.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtnprt, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtnprt.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnprt.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnprt.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnprt.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtnprt.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtnprt, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtnprt.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnprt.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnprt.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtnprt.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtnprt, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtnprt.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnprt.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnprt.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtnprt.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_labelscan
home_labelscan = lv.label(home)
home_labelscan.set_text("SCAN")
home_labelscan.set_long_mode(lv.label.LONG.WRAP)
home_labelscan.set_pos(317, 398)
home_labelscan.set_size(128, 44)
home_labelscan.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_labelscan, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_labelscan.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelscan.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_labelprt
home_labelprt = lv.label(home)
home_labelprt.set_text("PRINT")
home_labelprt.set_long_mode(lv.label.LONG.WRAP)
home_labelprt.set_pos(509, 398)
home_labelprt.set_size(149, 44)
home_labelprt.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_labelprt, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_labelprt.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelprt.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_labelset
home_labelset = lv.label(home)
home_labelset.set_text("SETUP")
home_labelset.set_long_mode(lv.label.LONG.WRAP)
home_labelset.set_pos(696, 398)
home_labelset.set_size(160, 44)
home_labelset.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_labelset, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_labelset.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_labelset.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_wifi
home_wifi = lv.img(home)
home_wifi.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\wifi_61_41.png"))
home_wifi.add_flag(lv.obj.FLAG.CLICKABLE)
home_wifi.set_pivot(0,0)
home_wifi.set_angle(0)
home_wifi.set_pos(119, 68)
home_wifi.set_size(61, 41)
home_wifi.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_wifi, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_wifi.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_tel
home_tel = lv.img(home)
home_tel.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\tel_44_44.png"))
home_tel.add_flag(lv.obj.FLAG.CLICKABLE)
home_tel.set_pivot(0,0)
home_tel.set_angle(0)
home_tel.set_pos(224, 66)
home_tel.set_size(44, 44)
home_tel.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_tel, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_tel.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_eco
home_eco = lv.img(home)
home_eco.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\eco_44_44.png"))
home_eco.add_flag(lv.obj.FLAG.CLICKABLE)
home_eco.set_pivot(0,0)
home_eco.set_angle(0)
home_eco.set_pos(313, 66)
home_eco.set_size(44, 44)
home_eco.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_eco, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_eco.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_pc
home_pc = lv.img(home)
home_pc.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\pc_44_44.png"))
home_pc.add_flag(lv.obj.FLAG.CLICKABLE)
home_pc.set_pivot(0,0)
home_pc.set_angle(0)
home_pc.set_pos(422, 66)
home_pc.set_size(44, 44)
home_pc.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_pc, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_pc.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgcopy
home_imgcopy = lv.img(home)
home_imgcopy.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\copy_61_61.png"))
home_imgcopy.add_flag(lv.obj.FLAG.CLICKABLE)
home_imgcopy.set_pivot(0,0)
home_imgcopy.set_angle(0)
home_imgcopy.set_pos(189, 295)
home_imgcopy.set_size(61, 61)
home_imgcopy.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgcopy, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgcopy.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgscan
home_imgscan = lv.img(home)
home_imgscan.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\scan_61_61.png"))
home_imgscan.add_flag(lv.obj.FLAG.CLICKABLE)
home_imgscan.set_pivot(0,0)
home_imgscan.set_angle(0)
home_imgscan.set_pos(381, 295)
home_imgscan.set_size(61, 61)
home_imgscan.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgscan, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgscan.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgprt
home_imgprt = lv.img(home)
home_imgprt.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\print_61_61.png"))
home_imgprt.add_flag(lv.obj.FLAG.CLICKABLE)
home_imgprt.set_pivot(0,0)
home_imgprt.set_angle(0)
home_imgprt.set_pos(573, 295)
home_imgprt.set_size(61, 61)
home_imgprt.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgprt, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgprt.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgset
home_imgset = lv.img(home)
home_imgset.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\setup_61_61.png"))
home_imgset.add_flag(lv.obj.FLAG.CLICKABLE)
home_imgset.set_pivot(0,0)
home_imgset.set_angle(0)
home_imgset.set_pos(765, 295)
home_imgset.set_size(61, 61)
home_imgset.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgset, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgset.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtn_1
home_imgbtn_1 = lv.imgbtn(home)
home_imgbtn_1.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_1.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_1.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_1.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_1.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtn_1_label = lv.label(home_imgbtn_1)
home_imgbtn_1_label.set_text("")
home_imgbtn_1_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtn_1_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtn_1.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtn_1.set_pos(874, 255)
home_imgbtn_1.set_size(181, 220)
home_imgbtn_1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtn_1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtn_1.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_1.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_1.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtn_1, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtn_1.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_1.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_1.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtn_1, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtn_1.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_1.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_1.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_label_1
home_label_1 = lv.label(home)
home_label_1.set_text("SETUP")
home_label_1.set_long_mode(lv.label.LONG.WRAP)
home_label_1.set_pos(910, 376)
home_label_1.set_size(160, 44)
home_label_1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_label_1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_label_1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_img_1
home_img_1 = lv.img(home)
home_img_1.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\setup_61_61.png"))
home_img_1.add_flag(lv.obj.FLAG.CLICKABLE)
home_img_1.set_pivot(0,0)
home_img_1.set_angle(0)
home_img_1.set_pos(971, 297)
home_img_1.set_size(61, 61)
home_img_1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_img_1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_img_1.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtn_2
home_imgbtn_2 = lv.imgbtn(home)
home_imgbtn_2.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_2.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_2.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_2.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_2.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtn_2_label = lv.label(home_imgbtn_2)
home_imgbtn_2_label.set_text("")
home_imgbtn_2_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtn_2_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtn_2.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtn_2.set_pos(1070, 255)
home_imgbtn_2.set_size(181, 220)
home_imgbtn_2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtn_2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtn_2.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_2.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_2.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtn_2, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtn_2.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_2.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtn_2, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtn_2.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_2.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_label_2
home_label_2 = lv.label(home)
home_label_2.set_text("SETUP")
home_label_2.set_long_mode(lv.label.LONG.WRAP)
home_label_2.set_pos(1097, 404)
home_label_2.set_size(160, 44)
home_label_2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_label_2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_label_2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_img_2
home_img_2 = lv.img(home)
home_img_2.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\setup_61_61.png"))
home_img_2.add_flag(lv.obj.FLAG.CLICKABLE)
home_img_2.set_pivot(0,0)
home_img_2.set_angle(0)
home_img_2.set_pos(1155, 297)
home_img_2.set_size(61, 61)
home_img_2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_img_2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_img_2.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtn_6
home_imgbtn_6 = lv.imgbtn(home)
home_imgbtn_6.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_6.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_6.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_6.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_181_220.png"), None)
home_imgbtn_6.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtn_6_label = lv.label(home_imgbtn_6)
home_imgbtn_6_label.set_text("")
home_imgbtn_6_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtn_6_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtn_6.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtn_6.set_pos(1878, 263)
home_imgbtn_6.set_size(181, 220)
home_imgbtn_6.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtn_6, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtn_6.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_6.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_6.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_6.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_6.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtn_6, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtn_6.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_6.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_6.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_6.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtn_6, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtn_6.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_6.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_6.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_6.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_img_6
home_img_6 = lv.img(home)
home_img_6.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\setup_61_61.png"))
home_img_6.add_flag(lv.obj.FLAG.CLICKABLE)
home_img_6.set_pivot(0,0)
home_img_6.set_angle(0)
home_img_6.set_pos(1967, 300)
home_img_6.set_size(61, 61)
home_img_6.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_img_6, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_img_6.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_label_6
home_label_6 = lv.label(home)
home_label_6.set_text("SETUP")
home_label_6.set_long_mode(lv.label.LONG.WRAP)
home_label_6.set_pos(1900, 410)
home_label_6.set_size(160, 44)
home_label_6.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_label_6, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_label_6.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_6.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtn_5
home_imgbtn_5 = lv.imgbtn(home)
home_imgbtn_5.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtn_5.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtn_5.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtn_5.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_181_220.png"), None)
home_imgbtn_5.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtn_5_label = lv.label(home_imgbtn_5)
home_imgbtn_5_label.set_text("")
home_imgbtn_5_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtn_5_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtn_5.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtn_5.set_pos(1687, 260)
home_imgbtn_5.set_size(181, 220)
home_imgbtn_5.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtn_5, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtn_5.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_5.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_5.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_5.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_5.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtn_5, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtn_5.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_5.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_5.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_5.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtn_5, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtn_5.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_5.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_5.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_5.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_img_5
home_img_5 = lv.img(home)
home_img_5.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\print_61_61.png"))
home_img_5.add_flag(lv.obj.FLAG.CLICKABLE)
home_img_5.set_pivot(0,0)
home_img_5.set_angle(0)
home_img_5.set_pos(1775, 325)
home_img_5.set_size(61, 61)
home_img_5.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_img_5, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_img_5.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_label_5
home_label_5 = lv.label(home)
home_label_5.set_text("PRINT")
home_label_5.set_long_mode(lv.label.LONG.WRAP)
home_label_5.set_pos(1693, 414)
home_label_5.set_size(149, 44)
home_label_5.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_label_5, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_label_5.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_5.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtn_4
home_imgbtn_4 = lv.imgbtn(home)
home_imgbtn_4.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtn_4.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtn_4.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtn_4.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_181_220.png"), None)
home_imgbtn_4.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtn_4_label = lv.label(home_imgbtn_4)
home_imgbtn_4_label.set_text("")
home_imgbtn_4_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtn_4_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtn_4.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtn_4.set_pos(1486, 260)
home_imgbtn_4.set_size(181, 220)
home_imgbtn_4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtn_4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtn_4.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_4.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_4.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_4.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtn_4, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtn_4.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_4.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_4.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtn_4, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtn_4.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_4.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_4.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_img_4
home_img_4 = lv.img(home)
home_img_4.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\scan_61_61.png"))
home_img_4.add_flag(lv.obj.FLAG.CLICKABLE)
home_img_4.set_pivot(0,0)
home_img_4.set_angle(0)
home_img_4.set_pos(1564, 280)
home_img_4.set_size(61, 61)
home_img_4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_img_4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_img_4.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_imgbtn_3
home_imgbtn_3 = lv.imgbtn(home)
home_imgbtn_3.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtn_3.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtn_3.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtn_3.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn_bg_1_181_206.png"), None)
home_imgbtn_3.add_flag(lv.obj.FLAG.CHECKABLE)
home_imgbtn_3_label = lv.label(home_imgbtn_3)
home_imgbtn_3_label.set_text("")
home_imgbtn_3_label.set_long_mode(lv.label.LONG.WRAP)
home_imgbtn_3_label.align(lv.ALIGN.CENTER, 0, 0)
home_imgbtn_3.set_style_pad_all(0, lv.STATE.DEFAULT)
home_imgbtn_3.set_pos(1280, 261)
home_imgbtn_3.set_size(181, 206)
home_imgbtn_3.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_imgbtn_3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_imgbtn_3.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_3.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_3.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_3.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_imgbtn_3.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for home_imgbtn_3, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
home_imgbtn_3.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_3.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_3.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
home_imgbtn_3.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for home_imgbtn_3, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
home_imgbtn_3.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_3.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_3.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
home_imgbtn_3.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create home_label_3
home_label_3 = lv.label(home)
home_label_3.set_text("COPY")
home_label_3.set_long_mode(lv.label.LONG.WRAP)
home_label_3.set_pos(1304, 413)
home_label_3.set_size(132, 44)
home_label_3.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_label_3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_label_3.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_3.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_img_3
home_img_3 = lv.img(home)
home_img_3.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\copy_61_61.png"))
home_img_3.add_flag(lv.obj.FLAG.CLICKABLE)
home_img_3.set_pivot(0,0)
home_img_3.set_angle(0)
home_img_3.set_pos(1381, 298)
home_img_3.set_size(61, 61)
home_img_3.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_img_3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_img_3.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create home_label_4
home_label_4 = lv.label(home)
home_label_4.set_text("SCAN")
home_label_4.set_long_mode(lv.label.LONG.WRAP)
home_label_4.set_pos(1530, 413)
home_label_4.set_size(128, 44)
home_label_4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for home_label_4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
home_label_4.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
home_label_4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

home.update_layout()
# Create copyhome
copyhome = lv.obj()
copyhome.set_size(1024, 600)
copyhome.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_cont1
copyhome_cont1 = lv.obj(copyhome)
copyhome_cont1.set_pos(0, 0)
copyhome_cont1.set_size(1024, 220)
copyhome_cont1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_cont1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_cont1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_cont2
copyhome_cont2 = lv.obj(copyhome)
copyhome_cont2.set_pos(0, 220)
copyhome_cont2.set_size(1024, 379)
copyhome_cont2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_cont2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_cont2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_bg_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_bg_grad_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_label1
copyhome_label1 = lv.label(copyhome)
copyhome_label1.set_text("ADJUST IMAGE")
copyhome_label1.set_long_mode(lv.label.LONG.WRAP)
copyhome_label1.set_pos(290, 66)
copyhome_label1.set_size(480, 44)
copyhome_label1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_label1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_label1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_label1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_img3
copyhome_img3 = lv.img(copyhome)
copyhome_img3.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\example_640_379.png"))
copyhome_img3.add_flag(lv.obj.FLAG.CLICKABLE)
copyhome_img3.set_pivot(0,0)
copyhome_img3.set_angle(0)
copyhome_img3.set_pos(57, 165)
copyhome_img3.set_size(640, 379)
copyhome_img3.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_img3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_img3.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_cont4
copyhome_cont4 = lv.obj(copyhome)
copyhome_cont4.set_pos(785, 176)
copyhome_cont4.set_size(170, 286)
copyhome_cont4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_cont4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_cont4.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_cont4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_btncopynext
copyhome_btncopynext = lv.btn(copyhome)
copyhome_btncopynext_label = lv.label(copyhome_btncopynext)
copyhome_btncopynext_label.set_text("NEXT")
copyhome_btncopynext_label.set_long_mode(lv.label.LONG.WRAP)
copyhome_btncopynext_label.align(lv.ALIGN.CENTER, 0, 0)
copyhome_btncopynext.set_style_pad_all(0, lv.STATE.DEFAULT)
copyhome_btncopynext.set_pos(785, 487)
copyhome_btncopynext.set_size(170, 88)
copyhome_btncopynext.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_btncopynext, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_btncopynext.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_bg_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_bg_grad_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopynext.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_sliderhue
copyhome_sliderhue = lv.slider(copyhome)
copyhome_sliderhue.set_range(0,100)
copyhome_sliderhue.set_value(50, False)
copyhome_sliderhue.set_pos(896, 253)
copyhome_sliderhue.set_size(17, 176)
copyhome_sliderhue.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_sliderhue, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_sliderhue.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_outline_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for copyhome_sliderhue, Part: lv.PART.INDICATOR, State: lv.STATE.DEFAULT.
copyhome_sliderhue.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_grad_color(lv.color_hex(0xddd7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_radius(110, lv.PART.INDICATOR|lv.STATE.DEFAULT)

# Set style for copyhome_sliderhue, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
copyhome_sliderhue.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_bg_grad_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderhue.set_style_radius(110, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create copyhome_sliderbright
copyhome_sliderbright = lv.slider(copyhome)
copyhome_sliderbright.set_range(0,100)
copyhome_sliderbright.set_value(50, False)
copyhome_sliderbright.set_pos(810, 253)
copyhome_sliderbright.set_size(17, 176)
copyhome_sliderbright.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_sliderbright, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_sliderbright.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_outline_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for copyhome_sliderbright, Part: lv.PART.INDICATOR, State: lv.STATE.DEFAULT.
copyhome_sliderbright.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_grad_color(lv.color_hex(0xddd7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_radius(110, lv.PART.INDICATOR|lv.STATE.DEFAULT)

# Set style for copyhome_sliderbright, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
copyhome_sliderbright.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_bg_grad_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
copyhome_sliderbright.set_style_radius(110, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create copyhome_bright
copyhome_bright = lv.img(copyhome)
copyhome_bright.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\bright_51_51.png"))
copyhome_bright.add_flag(lv.obj.FLAG.CLICKABLE)
copyhome_bright.set_pivot(0,0)
copyhome_bright.set_angle(0)
copyhome_bright.set_pos(793, 180)
copyhome_bright.set_size(51, 51)
copyhome_bright.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_bright, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_bright.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_hue
copyhome_hue = lv.img(copyhome)
copyhome_hue.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\hue_44_44.png"))
copyhome_hue.add_flag(lv.obj.FLAG.CLICKABLE)
copyhome_hue.set_pivot(0,0)
copyhome_hue.set_angle(0)
copyhome_hue.set_pos(881, 183)
copyhome_hue.set_size(44, 44)
copyhome_hue.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_hue, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_hue.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copyhome_btncopyback
copyhome_btncopyback = lv.btn(copyhome)
copyhome_btncopyback_label = lv.label(copyhome_btncopyback)
copyhome_btncopyback_label.set_text("<")
copyhome_btncopyback_label.set_long_mode(lv.label.LONG.WRAP)
copyhome_btncopyback_label.align(lv.ALIGN.CENTER, 0, 0)
copyhome_btncopyback.set_style_pad_all(0, lv.STATE.DEFAULT)
copyhome_btncopyback.set_pos(106, 55)
copyhome_btncopyback.set_size(64, 64)
copyhome_btncopyback.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copyhome_btncopyback, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copyhome_btncopyback.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
copyhome_btncopyback.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

copyhome.update_layout()
# Create copynext
copynext = lv.obj()
copynext.set_size(1024, 600)
copynext.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_cont1
copynext_cont1 = lv.obj(copynext)
copynext_cont1.set_pos(0, 0)
copynext_cont1.set_size(1024, 220)
copynext_cont1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_cont1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_cont1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_cont2
copynext_cont2 = lv.obj(copynext)
copynext_cont2.set_pos(0, 220)
copynext_cont2.set_size(1024, 379)
copynext_cont2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_cont2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_cont2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_bg_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_bg_grad_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_label1
copynext_label1 = lv.label(copynext)
copynext_label1.set_text("ADJUST IMAGE")
copynext_label1.set_long_mode(lv.label.LONG.WRAP)
copynext_label1.set_pos(290, 66)
copynext_label1.set_size(480, 44)
copynext_label1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_label1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_label1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_label1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_img3
copynext_img3 = lv.img(copynext)
copynext_img3.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\example_533_302.png"))
copynext_img3.add_flag(lv.obj.FLAG.CLICKABLE)
copynext_img3.set_pivot(0,0)
copynext_img3.set_angle(0)
copynext_img3.set_pos(57, 165)
copynext_img3.set_size(533, 302)
copynext_img3.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_img3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_img3.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_cont4
copynext_cont4 = lv.obj(copynext)
copynext_cont4.set_pos(650, 176)
copynext_cont4.set_size(320, 286)
copynext_cont4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_cont4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_cont4.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_cont4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_ddlist2
copynext_ddlist2 = lv.dropdown(copynext)
copynext_ddlist2.set_options("72 DPI\n96 DPI\n150 DPI\n300 DPI\n600 DPI\n900 DPI\n1200 DPI")
copynext_ddlist2.set_pos(354, 502)
copynext_ddlist2.set_size(213, 44)
copynext_ddlist2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_ddlist2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_ddlist2.set_style_text_color(lv.color_hex(0x0D3055), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_border_width(1, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_border_color(lv.color_hex(0xe1e6ee), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_pad_left(6, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_pad_right(6, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_radius(6, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for copynext_ddlist2, Part: lv.PART.SELECTED, State: lv.STATE.CHECKED.
style_copynext_ddlist2_extra_list_selected_checked = lv.style_t()
style_copynext_ddlist2_extra_list_selected_checked.init()
style_copynext_ddlist2_extra_list_selected_checked.set_text_color(lv.color_hex(0xffffff))
style_copynext_ddlist2_extra_list_selected_checked.set_text_font(test_font("montserratMedium", 25))
style_copynext_ddlist2_extra_list_selected_checked.set_border_width(1)
style_copynext_ddlist2_extra_list_selected_checked.set_border_opa(255)
style_copynext_ddlist2_extra_list_selected_checked.set_border_color(lv.color_hex(0xe1e6ee))
style_copynext_ddlist2_extra_list_selected_checked.set_radius(6)
style_copynext_ddlist2_extra_list_selected_checked.set_bg_opa(255)
style_copynext_ddlist2_extra_list_selected_checked.set_bg_color(lv.color_hex(0x00a1b5))
copynext_ddlist2.get_list().add_style(style_copynext_ddlist2_extra_list_selected_checked, lv.PART.SELECTED|lv.STATE.CHECKED)
# Set style for copynext_ddlist2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
style_copynext_ddlist2_extra_list_main_default = lv.style_t()
style_copynext_ddlist2_extra_list_main_default.init()
style_copynext_ddlist2_extra_list_main_default.set_max_height(90)
style_copynext_ddlist2_extra_list_main_default.set_text_color(lv.color_hex(0x0D3055))
style_copynext_ddlist2_extra_list_main_default.set_text_font(test_font("arial", 25))
style_copynext_ddlist2_extra_list_main_default.set_border_width(1)
style_copynext_ddlist2_extra_list_main_default.set_border_opa(255)
style_copynext_ddlist2_extra_list_main_default.set_border_color(lv.color_hex(0xe1e6ee))
style_copynext_ddlist2_extra_list_main_default.set_radius(6)
style_copynext_ddlist2_extra_list_main_default.set_bg_opa(255)
style_copynext_ddlist2_extra_list_main_default.set_bg_color(lv.color_hex(0xffffff))
style_copynext_ddlist2_extra_list_main_default.set_bg_grad_dir(lv.GRAD_DIR.VER)
style_copynext_ddlist2_extra_list_main_default.set_bg_grad_color(lv.color_hex(0xffffff))
copynext_ddlist2.get_list().add_style(style_copynext_ddlist2_extra_list_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for copynext_ddlist2, Part: lv.PART.SCROLLBAR, State: lv.STATE.DEFAULT.
style_copynext_ddlist2_extra_list_scrollbar_default = lv.style_t()
style_copynext_ddlist2_extra_list_scrollbar_default.init()
style_copynext_ddlist2_extra_list_scrollbar_default.set_radius(6)
style_copynext_ddlist2_extra_list_scrollbar_default.set_bg_opa(255)
style_copynext_ddlist2_extra_list_scrollbar_default.set_bg_color(lv.color_hex(0x00ff00))
copynext_ddlist2.get_list().add_style(style_copynext_ddlist2_extra_list_scrollbar_default, lv.PART.SCROLLBAR|lv.STATE.DEFAULT)

# Create copynext_btncopyback
copynext_btncopyback = lv.btn(copynext)
copynext_btncopyback_label = lv.label(copynext_btncopyback)
copynext_btncopyback_label.set_text("<")
copynext_btncopyback_label.set_long_mode(lv.label.LONG.WRAP)
copynext_btncopyback_label.align(lv.ALIGN.CENTER, 0, 0)
copynext_btncopyback.set_style_pad_all(0, lv.STATE.DEFAULT)
copynext_btncopyback.set_pos(106, 55)
copynext_btncopyback.set_size(64, 64)
copynext_btncopyback.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_btncopyback, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_btncopyback.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_btncopyback.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_btncopyback.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_btncopyback.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_btncopyback.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_btncopyback.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_btncopyback.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_swcolor
copynext_swcolor = lv.switch(copynext)
copynext_swcolor.set_pos(689, 386)
copynext_swcolor.set_size(85, 44)
copynext_swcolor.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_swcolor, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_swcolor.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swcolor.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swcolor.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swcolor.set_style_bg_grad_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swcolor.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swcolor.set_style_radius(220, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swcolor.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for copynext_swcolor, Part: lv.PART.INDICATOR, State: lv.STATE.CHECKED.
copynext_swcolor.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swcolor.set_style_bg_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swcolor.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swcolor.set_style_bg_grad_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swcolor.set_style_border_width(0, lv.PART.INDICATOR|lv.STATE.CHECKED)

# Set style for copynext_swcolor, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
copynext_swcolor.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swcolor.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swcolor.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swcolor.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swcolor.set_style_border_width(0, lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swcolor.set_style_radius(220, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create copynext_labelcopy
copynext_labelcopy = lv.label(copynext)
copynext_labelcopy.set_text("Copies")
copynext_labelcopy.set_long_mode(lv.label.LONG.WRAP)
copynext_labelcopy.set_pos(738, 189)
copynext_labelcopy.set_size(136, 44)
copynext_labelcopy.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_labelcopy, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_labelcopy.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_text_color(lv.color_hex(0x201818), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcopy.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_up
copynext_up = lv.btn(copynext)
copynext_up_label = lv.label(copynext_up)
copynext_up_label.set_text("+")
copynext_up_label.set_long_mode(lv.label.LONG.WRAP)
copynext_up_label.align(lv.ALIGN.CENTER, 0, 0)
copynext_up.set_style_pad_all(0, lv.STATE.DEFAULT)
copynext_up.set_pos(889, 242)
copynext_up.set_size(42, 42)
copynext_up.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_up, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_up.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_radius(8, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_text_font(test_font("simsun", 38), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_up.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_down
copynext_down = lv.btn(copynext)
copynext_down_label = lv.label(copynext_down)
copynext_down_label.set_text("-")
copynext_down_label.set_long_mode(lv.label.LONG.WRAP)
copynext_down_label.align(lv.ALIGN.CENTER, 0, 0)
copynext_down.set_style_pad_all(0, lv.STATE.DEFAULT)
copynext_down.set_pos(686, 242)
copynext_down.set_size(42, 42)
copynext_down.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_down, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_down.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_radius(8, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_text_font(test_font("simsun", 38), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_down.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_labelcnt
copynext_labelcnt = lv.label(copynext)
copynext_labelcnt.set_text("1")
copynext_labelcnt.set_long_mode(lv.label.LONG.WRAP)
copynext_labelcnt.set_pos(742, 249)
copynext_labelcnt.set_size(119, 44)
copynext_labelcnt.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_labelcnt, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_labelcnt.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_text_color(lv.color_hex(0x0a0606), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcnt.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_labelcolor
copynext_labelcolor = lv.label(copynext)
copynext_labelcolor.set_text("Color")
copynext_labelcolor.set_long_mode(lv.label.LONG.WRAP)
copynext_labelcolor.set_pos(669, 322)
copynext_labelcolor.set_size(106, 44)
copynext_labelcolor.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_labelcolor, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_labelcolor.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelcolor.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_labelvert
copynext_labelvert = lv.label(copynext)
copynext_labelvert.set_text("Vertical")
copynext_labelvert.set_long_mode(lv.label.LONG.WRAP)
copynext_labelvert.set_pos(810, 322)
copynext_labelvert.set_size(149, 44)
copynext_labelvert.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_labelvert, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_labelvert.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_labelvert.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create copynext_swvert
copynext_swvert = lv.switch(copynext)
copynext_swvert.set_pos(832, 386)
copynext_swvert.set_size(85, 44)
copynext_swvert.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_swvert, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_swvert.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swvert.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swvert.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swvert.set_style_bg_grad_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swvert.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swvert.set_style_radius(220, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_swvert.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for copynext_swvert, Part: lv.PART.INDICATOR, State: lv.STATE.CHECKED.
copynext_swvert.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swvert.set_style_bg_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swvert.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swvert.set_style_bg_grad_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
copynext_swvert.set_style_border_width(0, lv.PART.INDICATOR|lv.STATE.CHECKED)

# Set style for copynext_swvert, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
copynext_swvert.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swvert.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swvert.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swvert.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swvert.set_style_border_width(0, lv.PART.KNOB|lv.STATE.DEFAULT)
copynext_swvert.set_style_radius(220, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create copynext_ddlist1
copynext_ddlist1 = lv.dropdown(copynext)
copynext_ddlist1.set_options("Best\nNormal\nDraft")
copynext_ddlist1.set_pos(66, 500)
copynext_ddlist1.set_size(213, 44)
copynext_ddlist1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_ddlist1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_ddlist1.set_style_text_color(lv.color_hex(0x0D3055), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_border_width(1, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_border_color(lv.color_hex(0xe1e6ee), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_pad_left(6, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_pad_right(6, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_radius(6, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_ddlist1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for copynext_ddlist1, Part: lv.PART.SELECTED, State: lv.STATE.CHECKED.
style_copynext_ddlist1_extra_list_selected_checked = lv.style_t()
style_copynext_ddlist1_extra_list_selected_checked.init()
style_copynext_ddlist1_extra_list_selected_checked.set_text_color(lv.color_hex(0xffffff))
style_copynext_ddlist1_extra_list_selected_checked.set_text_font(test_font("montserratMedium", 25))
style_copynext_ddlist1_extra_list_selected_checked.set_border_width(1)
style_copynext_ddlist1_extra_list_selected_checked.set_border_opa(255)
style_copynext_ddlist1_extra_list_selected_checked.set_border_color(lv.color_hex(0xe1e6ee))
style_copynext_ddlist1_extra_list_selected_checked.set_radius(6)
style_copynext_ddlist1_extra_list_selected_checked.set_bg_opa(255)
style_copynext_ddlist1_extra_list_selected_checked.set_bg_color(lv.color_hex(0x00a1b5))
copynext_ddlist1.get_list().add_style(style_copynext_ddlist1_extra_list_selected_checked, lv.PART.SELECTED|lv.STATE.CHECKED)
# Set style for copynext_ddlist1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
style_copynext_ddlist1_extra_list_main_default = lv.style_t()
style_copynext_ddlist1_extra_list_main_default.init()
style_copynext_ddlist1_extra_list_main_default.set_max_height(90)
style_copynext_ddlist1_extra_list_main_default.set_text_color(lv.color_hex(0x0D3055))
style_copynext_ddlist1_extra_list_main_default.set_text_font(test_font("arial", 25))
style_copynext_ddlist1_extra_list_main_default.set_border_width(1)
style_copynext_ddlist1_extra_list_main_default.set_border_opa(255)
style_copynext_ddlist1_extra_list_main_default.set_border_color(lv.color_hex(0xe1e6ee))
style_copynext_ddlist1_extra_list_main_default.set_radius(6)
style_copynext_ddlist1_extra_list_main_default.set_bg_opa(255)
style_copynext_ddlist1_extra_list_main_default.set_bg_color(lv.color_hex(0xffffff))
style_copynext_ddlist1_extra_list_main_default.set_bg_grad_dir(lv.GRAD_DIR.VER)
style_copynext_ddlist1_extra_list_main_default.set_bg_grad_color(lv.color_hex(0xffffff))
copynext_ddlist1.get_list().add_style(style_copynext_ddlist1_extra_list_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for copynext_ddlist1, Part: lv.PART.SCROLLBAR, State: lv.STATE.DEFAULT.
style_copynext_ddlist1_extra_list_scrollbar_default = lv.style_t()
style_copynext_ddlist1_extra_list_scrollbar_default.init()
style_copynext_ddlist1_extra_list_scrollbar_default.set_radius(6)
style_copynext_ddlist1_extra_list_scrollbar_default.set_bg_opa(255)
style_copynext_ddlist1_extra_list_scrollbar_default.set_bg_color(lv.color_hex(0x00ff00))
copynext_ddlist1.get_list().add_style(style_copynext_ddlist1_extra_list_scrollbar_default, lv.PART.SCROLLBAR|lv.STATE.DEFAULT)

# Create copynext_print
copynext_print = lv.btn(copynext)
copynext_print_label = lv.label(copynext_print)
copynext_print_label.set_text("PRINT")
copynext_print_label.set_long_mode(lv.label.LONG.WRAP)
copynext_print_label.align(lv.ALIGN.CENTER, 0, 0)
copynext_print.set_style_pad_all(0, lv.STATE.DEFAULT)
copynext_print.set_pos(682, 491)
copynext_print.set_size(251, 88)
copynext_print.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for copynext_print, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
copynext_print.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_bg_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_bg_grad_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
copynext_print.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

copynext.update_layout()
# Create scanhome
scanhome = lv.obj()
scanhome.set_size(1024, 600)
scanhome.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_cont0
scanhome_cont0 = lv.obj(scanhome)
scanhome_cont0.set_pos(0, 0)
scanhome_cont0.set_size(1024, 220)
scanhome_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_label1
scanhome_label1 = lv.label(scanhome)
scanhome_label1.set_text("ADJUST IMAGE")
scanhome_label1.set_long_mode(lv.label.LONG.WRAP)
scanhome_label1.set_pos(290, 66)
scanhome_label1.set_size(480, 44)
scanhome_label1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_label1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_label1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_label1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_cont2
scanhome_cont2 = lv.obj(scanhome)
scanhome_cont2.set_pos(0, 220)
scanhome_cont2.set_size(1024, 379)
scanhome_cont2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_cont2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_cont2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_bg_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_bg_grad_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_img3
scanhome_img3 = lv.img(scanhome)
scanhome_img3.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\example_640_379.png"))
scanhome_img3.add_flag(lv.obj.FLAG.CLICKABLE)
scanhome_img3.set_pivot(0,0)
scanhome_img3.set_angle(0)
scanhome_img3.set_pos(57, 165)
scanhome_img3.set_size(640, 379)
scanhome_img3.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_img3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_img3.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_cont4
scanhome_cont4 = lv.obj(scanhome)
scanhome_cont4.set_pos(785, 176)
scanhome_cont4.set_size(170, 286)
scanhome_cont4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_cont4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_cont4.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_cont4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_btnscansave
scanhome_btnscansave = lv.btn(scanhome)
scanhome_btnscansave_label = lv.label(scanhome_btnscansave)
scanhome_btnscansave_label.set_text("SAVE")
scanhome_btnscansave_label.set_long_mode(lv.label.LONG.WRAP)
scanhome_btnscansave_label.align(lv.ALIGN.CENTER, 0, 0)
scanhome_btnscansave.set_style_pad_all(0, lv.STATE.DEFAULT)
scanhome_btnscansave.set_pos(785, 487)
scanhome_btnscansave.set_size(170, 88)
scanhome_btnscansave.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_btnscansave, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_btnscansave.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_bg_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_bg_grad_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscansave.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_sliderhue
scanhome_sliderhue = lv.slider(scanhome)
scanhome_sliderhue.set_range(0,100)
scanhome_sliderhue.set_value(50, False)
scanhome_sliderhue.set_pos(896, 253)
scanhome_sliderhue.set_size(17, 176)
scanhome_sliderhue.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_sliderhue, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_sliderhue.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_outline_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for scanhome_sliderhue, Part: lv.PART.INDICATOR, State: lv.STATE.DEFAULT.
scanhome_sliderhue.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_grad_color(lv.color_hex(0xddd7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_radius(110, lv.PART.INDICATOR|lv.STATE.DEFAULT)

# Set style for scanhome_sliderhue, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
scanhome_sliderhue.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_bg_grad_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderhue.set_style_radius(110, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create scanhome_sliderbright
scanhome_sliderbright = lv.slider(scanhome)
scanhome_sliderbright.set_range(0,100)
scanhome_sliderbright.set_value(50, False)
scanhome_sliderbright.set_pos(810, 253)
scanhome_sliderbright.set_size(17, 176)
scanhome_sliderbright.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_sliderbright, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_sliderbright.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_outline_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for scanhome_sliderbright, Part: lv.PART.INDICATOR, State: lv.STATE.DEFAULT.
scanhome_sliderbright.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_grad_color(lv.color_hex(0xddd7d9), lv.PART.INDICATOR|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_radius(110, lv.PART.INDICATOR|lv.STATE.DEFAULT)

# Set style for scanhome_sliderbright, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
scanhome_sliderbright.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_bg_grad_color(lv.color_hex(0x293041), lv.PART.KNOB|lv.STATE.DEFAULT)
scanhome_sliderbright.set_style_radius(110, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create scanhome_bright
scanhome_bright = lv.img(scanhome)
scanhome_bright.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\bright_51_51.png"))
scanhome_bright.add_flag(lv.obj.FLAG.CLICKABLE)
scanhome_bright.set_pivot(0,0)
scanhome_bright.set_angle(0)
scanhome_bright.set_pos(793, 180)
scanhome_bright.set_size(51, 51)
scanhome_bright.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_bright, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_bright.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_hue
scanhome_hue = lv.img(scanhome)
scanhome_hue.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\hue_44_44.png"))
scanhome_hue.add_flag(lv.obj.FLAG.CLICKABLE)
scanhome_hue.set_pivot(0,0)
scanhome_hue.set_angle(0)
scanhome_hue.set_pos(881, 183)
scanhome_hue.set_size(44, 44)
scanhome_hue.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_hue, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_hue.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create scanhome_btnscanback
scanhome_btnscanback = lv.btn(scanhome)
scanhome_btnscanback_label = lv.label(scanhome_btnscanback)
scanhome_btnscanback_label.set_text("<")
scanhome_btnscanback_label.set_long_mode(lv.label.LONG.WRAP)
scanhome_btnscanback_label.align(lv.ALIGN.CENTER, 0, 0)
scanhome_btnscanback.set_style_pad_all(0, lv.STATE.DEFAULT)
scanhome_btnscanback.set_pos(106, 55)
scanhome_btnscanback.set_size(64, 64)
scanhome_btnscanback.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for scanhome_btnscanback, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
scanhome_btnscanback.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
scanhome_btnscanback.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

scanhome.update_layout()
# Create prthome
prthome = lv.obj()
prthome.set_size(1024, 600)
prthome.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_cont0
prthome_cont0 = lv.obj(prthome)
prthome_cont0.set_pos(0, 0)
prthome_cont0.set_size(1024, 220)
prthome_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_cont3
prthome_cont3 = lv.obj(prthome)
prthome_cont3.set_pos(0, 220)
prthome_cont3.set_size(1024, 379)
prthome_cont3.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_cont3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_cont3.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_bg_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_bg_grad_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont3.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_cont1
prthome_cont1 = lv.obj(prthome)
prthome_cont1.set_pos(85, 132)
prthome_cont1.set_size(853, 308)
prthome_cont1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_cont1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_cont1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_cont1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_label4
prthome_label4 = lv.label(prthome)
prthome_label4.set_text("PRINT MENU")
prthome_label4.set_long_mode(lv.label.LONG.WRAP)
prthome_label4.set_pos(360, 35)
prthome_label4.set_size(292, 66)
prthome_label4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_label4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_label4.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_imgbtnit
prthome_imgbtnit = lv.imgbtn(prthome)
prthome_imgbtnit.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_245_308.png"), None)
prthome_imgbtnit.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_245_308.png"), None)
prthome_imgbtnit.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_245_308.png"), None)
prthome_imgbtnit.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn4_245_308.png"), None)
prthome_imgbtnit.add_flag(lv.obj.FLAG.CHECKABLE)
prthome_imgbtnit_label = lv.label(prthome_imgbtnit)
prthome_imgbtnit_label.set_text("")
prthome_imgbtnit_label.set_long_mode(lv.label.LONG.WRAP)
prthome_imgbtnit_label.align(lv.ALIGN.CENTER, 0, 0)
prthome_imgbtnit.set_style_pad_all(0, lv.STATE.DEFAULT)
prthome_imgbtnit.set_pos(693, 132)
prthome_imgbtnit.set_size(245, 308)
prthome_imgbtnit.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_imgbtnit, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_imgbtnit.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnit.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnit.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnit.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnit.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for prthome_imgbtnit, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
prthome_imgbtnit.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnit.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnit.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnit.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for prthome_imgbtnit, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
prthome_imgbtnit.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnit.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnit.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnit.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create prthome_imgbtnusb
prthome_imgbtnusb = lv.imgbtn(prthome)
prthome_imgbtnusb.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_245_308.png"), None)
prthome_imgbtnusb.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_245_308.png"), None)
prthome_imgbtnusb.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_245_308.png"), None)
prthome_imgbtnusb.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn2_245_308.png"), None)
prthome_imgbtnusb.add_flag(lv.obj.FLAG.CHECKABLE)
prthome_imgbtnusb_label = lv.label(prthome_imgbtnusb)
prthome_imgbtnusb_label.set_text("")
prthome_imgbtnusb_label.set_long_mode(lv.label.LONG.WRAP)
prthome_imgbtnusb_label.align(lv.ALIGN.CENTER, 0, 0)
prthome_imgbtnusb.set_style_pad_all(0, lv.STATE.DEFAULT)
prthome_imgbtnusb.set_pos(85, 132)
prthome_imgbtnusb.set_size(245, 308)
prthome_imgbtnusb.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_imgbtnusb, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_imgbtnusb.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnusb.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnusb.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnusb.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnusb.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for prthome_imgbtnusb, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
prthome_imgbtnusb.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnusb.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnusb.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnusb.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for prthome_imgbtnusb, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
prthome_imgbtnusb.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnusb.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnusb.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnusb.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create prthome_imgbtnmobile
prthome_imgbtnmobile = lv.imgbtn(prthome)
prthome_imgbtnmobile.set_src(lv.imgbtn.STATE.RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_245_308.png"), None)
prthome_imgbtnmobile.set_src(lv.imgbtn.STATE.PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_245_308.png"), None)
prthome_imgbtnmobile.set_src(lv.imgbtn.STATE.CHECKED_RELEASED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_245_308.png"), None)
prthome_imgbtnmobile.set_src(lv.imgbtn.STATE.CHECKED_PRESSED, None, load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\btn3_245_308.png"), None)
prthome_imgbtnmobile.add_flag(lv.obj.FLAG.CHECKABLE)
prthome_imgbtnmobile_label = lv.label(prthome_imgbtnmobile)
prthome_imgbtnmobile_label.set_text("")
prthome_imgbtnmobile_label.set_long_mode(lv.label.LONG.WRAP)
prthome_imgbtnmobile_label.align(lv.ALIGN.CENTER, 0, 0)
prthome_imgbtnmobile.set_style_pad_all(0, lv.STATE.DEFAULT)
prthome_imgbtnmobile.set_pos(390, 132)
prthome_imgbtnmobile.set_size(245, 308)
prthome_imgbtnmobile.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_imgbtnmobile, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_imgbtnmobile.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnmobile.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnmobile.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnmobile.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_imgbtnmobile.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for prthome_imgbtnmobile, Part: lv.PART.MAIN, State: lv.STATE.PRESSED.
prthome_imgbtnmobile.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnmobile.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnmobile.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.PRESSED)
prthome_imgbtnmobile.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.PRESSED)
# Set style for prthome_imgbtnmobile, Part: lv.PART.MAIN, State: lv.STATE.CHECKED.
prthome_imgbtnmobile.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnmobile.set_style_text_color(lv.color_hex(0xFF33FF), lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnmobile.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.CHECKED)
prthome_imgbtnmobile.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.CHECKED)

# Create prthome_labelusb
prthome_labelusb = lv.label(prthome)
prthome_labelusb.set_text("USB")
prthome_labelusb.set_long_mode(lv.label.LONG.WRAP)
prthome_labelusb.set_pos(123, 352)
prthome_labelusb.set_size(157, 44)
prthome_labelusb.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_labelusb, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_labelusb.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelusb.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_labelmobile
prthome_labelmobile = lv.label(prthome)
prthome_labelmobile.set_text("MOBILE")
prthome_labelmobile.set_long_mode(lv.label.LONG.WRAP)
prthome_labelmobile.set_pos(422, 352)
prthome_labelmobile.set_size(157, 44)
prthome_labelmobile.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_labelmobile, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_labelmobile.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelmobile.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_labelit
prthome_labelit = lv.label(prthome)
prthome_labelit.set_text("INTERNET")
prthome_labelit.set_long_mode(lv.label.LONG.WRAP)
prthome_labelit.set_pos(725, 352)
prthome_labelit.set_size(181, 44)
prthome_labelit.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_labelit, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_labelit.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_labelit.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_label2
prthome_label2 = lv.label(prthome)
prthome_label2.set_text("From where do you want to print ?")
prthome_label2.set_long_mode(lv.label.LONG.WRAP)
prthome_label2.set_pos(34, 480)
prthome_label2.set_size(938, 66)
prthome_label2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_label2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_label2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_text_color(lv.color_hex(0x251d1d), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_label2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_usb
prthome_usb = lv.img(prthome)
prthome_usb.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\usb_64_64.png"))
prthome_usb.add_flag(lv.obj.FLAG.CLICKABLE)
prthome_usb.set_pivot(0,0)
prthome_usb.set_angle(0)
prthome_usb.set_pos(213, 187)
prthome_usb.set_size(64, 64)
prthome_usb.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_usb, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_usb.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_mobile
prthome_mobile = lv.img(prthome)
prthome_mobile.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\mobile_64_64.png"))
prthome_mobile.add_flag(lv.obj.FLAG.CLICKABLE)
prthome_mobile.set_pivot(0,0)
prthome_mobile.set_angle(0)
prthome_mobile.set_pos(516, 187)
prthome_mobile.set_size(64, 64)
prthome_mobile.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_mobile, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_mobile.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_internet
prthome_internet = lv.img(prthome)
prthome_internet.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\internet_64_64.png"))
prthome_internet.add_flag(lv.obj.FLAG.CLICKABLE)
prthome_internet.set_pivot(0,0)
prthome_internet.set_angle(0)
prthome_internet.set_pos(817, 187)
prthome_internet.set_size(64, 64)
prthome_internet.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_internet, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_internet.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prthome_btnprintback
prthome_btnprintback = lv.btn(prthome)
prthome_btnprintback_label = lv.label(prthome_btnprintback)
prthome_btnprintback_label.set_text("<")
prthome_btnprintback_label.set_long_mode(lv.label.LONG.WRAP)
prthome_btnprintback_label.align(lv.ALIGN.CENTER, 0, 0)
prthome_btnprintback.set_style_pad_all(0, lv.STATE.DEFAULT)
prthome_btnprintback.set_pos(106, 28)
prthome_btnprintback.set_size(64, 64)
prthome_btnprintback.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prthome_btnprintback, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prthome_btnprintback.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prthome_btnprintback.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

prthome.update_layout()
# Create prtusb
prtusb = lv.obj()
prtusb.set_size(1024, 600)
prtusb.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_cont0
prtusb_cont0 = lv.obj(prtusb)
prtusb_cont0.set_pos(0, 0)
prtusb_cont0.set_size(1024, 220)
prtusb_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_cont2
prtusb_cont2 = lv.obj(prtusb)
prtusb_cont2.set_pos(0, 220)
prtusb_cont2.set_size(1024, 379)
prtusb_cont2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_cont2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_cont2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_bg_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_bg_grad_color(lv.color_hex(0xdedede), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_labeltitle
prtusb_labeltitle = lv.label(prtusb)
prtusb_labeltitle.set_text("PRINTING FROM USB")
prtusb_labeltitle.set_long_mode(lv.label.LONG.WRAP)
prtusb_labeltitle.set_pos(290, 66)
prtusb_labeltitle.set_size(480, 66)
prtusb_labeltitle.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_labeltitle, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_labeltitle.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labeltitle.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_cont4
prtusb_cont4 = lv.obj(prtusb)
prtusb_cont4.set_pos(650, 176)
prtusb_cont4.set_size(320, 286)
prtusb_cont4.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_cont4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_cont4.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_cont4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_btnprint
prtusb_btnprint = lv.btn(prtusb)
prtusb_btnprint_label = lv.label(prtusb_btnprint)
prtusb_btnprint_label.set_text("PRINT")
prtusb_btnprint_label.set_long_mode(lv.label.LONG.WRAP)
prtusb_btnprint_label.align(lv.ALIGN.CENTER, 0, 0)
prtusb_btnprint.set_style_pad_all(0, lv.STATE.DEFAULT)
prtusb_btnprint.set_pos(682, 491)
prtusb_btnprint.set_size(251, 88)
prtusb_btnprint.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_btnprint, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_btnprint.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_bg_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_bg_grad_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_btnprint.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_back
prtusb_back = lv.btn(prtusb)
prtusb_back_label = lv.label(prtusb_back)
prtusb_back_label.set_text("<")
prtusb_back_label.set_long_mode(lv.label.LONG.WRAP)
prtusb_back_label.align(lv.ALIGN.CENTER, 0, 0)
prtusb_back.set_style_pad_all(0, lv.STATE.DEFAULT)
prtusb_back.set_pos(106, 55)
prtusb_back.set_size(64, 64)
prtusb_back.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_back, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_back.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_back.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_swcolor
prtusb_swcolor = lv.switch(prtusb)
prtusb_swcolor.set_pos(689, 386)
prtusb_swcolor.set_size(85, 44)
prtusb_swcolor.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_swcolor, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_swcolor.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_bg_grad_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_radius(220, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for prtusb_swcolor, Part: lv.PART.INDICATOR, State: lv.STATE.CHECKED.
prtusb_swcolor.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swcolor.set_style_bg_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swcolor.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swcolor.set_style_bg_grad_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swcolor.set_style_border_width(0, lv.PART.INDICATOR|lv.STATE.CHECKED)

# Set style for prtusb_swcolor, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
prtusb_swcolor.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_border_width(0, lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swcolor.set_style_radius(220, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create prtusb_labelcopy
prtusb_labelcopy = lv.label(prtusb)
prtusb_labelcopy.set_text("Copies")
prtusb_labelcopy.set_long_mode(lv.label.LONG.WRAP)
prtusb_labelcopy.set_pos(736, 191)
prtusb_labelcopy.set_size(136, 44)
prtusb_labelcopy.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_labelcopy, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_labelcopy.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcopy.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_up
prtusb_up = lv.btn(prtusb)
prtusb_up_label = lv.label(prtusb_up)
prtusb_up_label.set_text("+")
prtusb_up_label.set_long_mode(lv.label.LONG.WRAP)
prtusb_up_label.align(lv.ALIGN.CENTER, 0, 0)
prtusb_up.set_style_pad_all(0, lv.STATE.DEFAULT)
prtusb_up.set_pos(874, 242)
prtusb_up.set_size(42, 42)
prtusb_up.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_up, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_up.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_radius(8, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_text_font(test_font("simsun", 38), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_up.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_down
prtusb_down = lv.btn(prtusb)
prtusb_down_label = lv.label(prtusb_down)
prtusb_down_label.set_text("-")
prtusb_down_label.set_long_mode(lv.label.LONG.WRAP)
prtusb_down_label.align(lv.ALIGN.CENTER, 0, 0)
prtusb_down.set_style_pad_all(0, lv.STATE.DEFAULT)
prtusb_down.set_pos(686, 242)
prtusb_down.set_size(42, 42)
prtusb_down.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_down, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_down.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_radius(8, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_text_font(test_font("simsun", 38), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_down.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_labelcnt
prtusb_labelcnt = lv.label(prtusb)
prtusb_labelcnt.set_text("1")
prtusb_labelcnt.set_long_mode(lv.label.LONG.WRAP)
prtusb_labelcnt.set_pos(708, 242)
prtusb_labelcnt.set_size(192, 66)
prtusb_labelcnt.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_labelcnt, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_labelcnt.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_text_color(lv.color_hex(0x141010), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_text_font(test_font("arial", 44), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcnt.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_labelcolor
prtusb_labelcolor = lv.label(prtusb)
prtusb_labelcolor.set_text("Color")
prtusb_labelcolor.set_long_mode(lv.label.LONG.WRAP)
prtusb_labelcolor.set_pos(669, 322)
prtusb_labelcolor.set_size(106, 44)
prtusb_labelcolor.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_labelcolor, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_labelcolor.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelcolor.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_labelvert
prtusb_labelvert = lv.label(prtusb)
prtusb_labelvert.set_text("Vertical")
prtusb_labelvert.set_long_mode(lv.label.LONG.WRAP)
prtusb_labelvert.set_pos(810, 322)
prtusb_labelvert.set_size(149, 44)
prtusb_labelvert.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_labelvert, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_labelvert.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_labelvert.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtusb_swvert
prtusb_swvert = lv.switch(prtusb)
prtusb_swvert.set_pos(832, 386)
prtusb_swvert.set_size(85, 44)
prtusb_swvert.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_swvert, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_swvert.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swvert.set_style_bg_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swvert.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swvert.set_style_bg_grad_color(lv.color_hex(0xd4d7d9), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swvert.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swvert.set_style_radius(220, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_swvert.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for prtusb_swvert, Part: lv.PART.INDICATOR, State: lv.STATE.CHECKED.
prtusb_swvert.set_style_bg_opa(255, lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swvert.set_style_bg_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swvert.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swvert.set_style_bg_grad_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.CHECKED)
prtusb_swvert.set_style_border_width(0, lv.PART.INDICATOR|lv.STATE.CHECKED)

# Set style for prtusb_swvert, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
prtusb_swvert.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swvert.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swvert.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swvert.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swvert.set_style_border_width(0, lv.PART.KNOB|lv.STATE.DEFAULT)
prtusb_swvert.set_style_radius(220, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create prtusb_list16
prtusb_list16 = lv.list(prtusb)
prtusb_list16_item0 = prtusb_list16.add_btn(lv.SYMBOL.FILE, "Contract 12.pdf")
prtusb_list16_item1 = prtusb_list16.add_btn(lv.SYMBOL.FILE, "Scanned_05_21.pdf")
prtusb_list16_item2 = prtusb_list16.add_btn(lv.SYMBOL.FILE, "Photo_2.jpg")
prtusb_list16_item3 = prtusb_list16.add_btn(lv.SYMBOL.FILE, "Photo_3.jpg")
prtusb_list16.set_pos(66, 183)
prtusb_list16.set_size(512, 273)
prtusb_list16.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_list16, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_list16.set_style_pad_top(5, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_pad_left(5, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_pad_right(5, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_pad_bottom(5, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_border_width(1, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_border_color(lv.color_hex(0xe1e6ee), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_radius(6, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for prtusb_list16, Part: lv.PART.SCROLLBAR, State: lv.STATE.DEFAULT.
prtusb_list16.set_style_radius(6, lv.PART.SCROLLBAR|lv.STATE.DEFAULT)
prtusb_list16.set_style_bg_opa(255, lv.PART.SCROLLBAR|lv.STATE.DEFAULT)
prtusb_list16.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.SCROLLBAR|lv.STATE.DEFAULT)
# Set style for prtusb_list16, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
style_prtusb_list16_extra_btns_main_default = lv.style_t()
style_prtusb_list16_extra_btns_main_default.init()
style_prtusb_list16_extra_btns_main_default.set_pad_top(5)
style_prtusb_list16_extra_btns_main_default.set_pad_left(5)
style_prtusb_list16_extra_btns_main_default.set_pad_right(5)
style_prtusb_list16_extra_btns_main_default.set_pad_bottom(5)
style_prtusb_list16_extra_btns_main_default.set_border_width(0)
style_prtusb_list16_extra_btns_main_default.set_text_color(lv.color_hex(0x0D3055))
style_prtusb_list16_extra_btns_main_default.set_text_font(test_font("arial", 25))
style_prtusb_list16_extra_btns_main_default.set_radius(6)
style_prtusb_list16_extra_btns_main_default.set_bg_opa(255)
style_prtusb_list16_extra_btns_main_default.set_bg_color(lv.color_hex(0xffffff))
style_prtusb_list16_extra_btns_main_default.set_bg_grad_dir(lv.GRAD_DIR.VER)
style_prtusb_list16_extra_btns_main_default.set_bg_grad_color(lv.color_hex(0xffffff))
prtusb_list16_item3.add_style(style_prtusb_list16_extra_btns_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16_item2.add_style(style_prtusb_list16_extra_btns_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16_item1.add_style(style_prtusb_list16_extra_btns_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_list16_item0.add_style(style_prtusb_list16_extra_btns_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for prtusb_list16, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
style_prtusb_list16_extra_texts_main_default = lv.style_t()
style_prtusb_list16_extra_texts_main_default.init()
style_prtusb_list16_extra_texts_main_default.set_pad_top(5)
style_prtusb_list16_extra_texts_main_default.set_pad_left(5)
style_prtusb_list16_extra_texts_main_default.set_pad_right(5)
style_prtusb_list16_extra_texts_main_default.set_pad_bottom(5)
style_prtusb_list16_extra_texts_main_default.set_border_width(0)
style_prtusb_list16_extra_texts_main_default.set_text_color(lv.color_hex(0x0D3055))
style_prtusb_list16_extra_texts_main_default.set_text_font(test_font("montserratMedium", 25))
style_prtusb_list16_extra_texts_main_default.set_radius(6)
style_prtusb_list16_extra_texts_main_default.set_bg_opa(255)
style_prtusb_list16_extra_texts_main_default.set_bg_color(lv.color_hex(0xffffff))

# Create prtusb_ddlist1
prtusb_ddlist1 = lv.dropdown(prtusb)
prtusb_ddlist1.set_options("Best\nNormal\nDraft")
prtusb_ddlist1.set_pos(59, 485)
prtusb_ddlist1.set_size(213, 46)
prtusb_ddlist1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_ddlist1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_ddlist1.set_style_text_color(lv.color_hex(0x0D3055), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_border_width(1, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_border_color(lv.color_hex(0xe1e6ee), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_pad_top(4, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_pad_left(6, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_pad_right(6, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_radius(6, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for prtusb_ddlist1, Part: lv.PART.SELECTED, State: lv.STATE.CHECKED.
style_prtusb_ddlist1_extra_list_selected_checked = lv.style_t()
style_prtusb_ddlist1_extra_list_selected_checked.init()
style_prtusb_ddlist1_extra_list_selected_checked.set_text_color(lv.color_hex(0xffffff))
style_prtusb_ddlist1_extra_list_selected_checked.set_text_font(test_font("montserratMedium", 25))
style_prtusb_ddlist1_extra_list_selected_checked.set_border_width(1)
style_prtusb_ddlist1_extra_list_selected_checked.set_border_opa(255)
style_prtusb_ddlist1_extra_list_selected_checked.set_border_color(lv.color_hex(0xe1e6ee))
style_prtusb_ddlist1_extra_list_selected_checked.set_radius(6)
style_prtusb_ddlist1_extra_list_selected_checked.set_bg_opa(255)
style_prtusb_ddlist1_extra_list_selected_checked.set_bg_color(lv.color_hex(0x00a1b5))
prtusb_ddlist1.get_list().add_style(style_prtusb_ddlist1_extra_list_selected_checked, lv.PART.SELECTED|lv.STATE.CHECKED)
# Set style for prtusb_ddlist1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
style_prtusb_ddlist1_extra_list_main_default = lv.style_t()
style_prtusb_ddlist1_extra_list_main_default.init()
style_prtusb_ddlist1_extra_list_main_default.set_max_height(90)
style_prtusb_ddlist1_extra_list_main_default.set_text_color(lv.color_hex(0x0D3055))
style_prtusb_ddlist1_extra_list_main_default.set_text_font(test_font("arial", 25))
style_prtusb_ddlist1_extra_list_main_default.set_border_width(1)
style_prtusb_ddlist1_extra_list_main_default.set_border_opa(255)
style_prtusb_ddlist1_extra_list_main_default.set_border_color(lv.color_hex(0xe1e6ee))
style_prtusb_ddlist1_extra_list_main_default.set_radius(6)
style_prtusb_ddlist1_extra_list_main_default.set_bg_opa(255)
style_prtusb_ddlist1_extra_list_main_default.set_bg_color(lv.color_hex(0xffffff))
style_prtusb_ddlist1_extra_list_main_default.set_bg_grad_dir(lv.GRAD_DIR.VER)
style_prtusb_ddlist1_extra_list_main_default.set_bg_grad_color(lv.color_hex(0xffffff))
prtusb_ddlist1.get_list().add_style(style_prtusb_ddlist1_extra_list_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for prtusb_ddlist1, Part: lv.PART.SCROLLBAR, State: lv.STATE.DEFAULT.
style_prtusb_ddlist1_extra_list_scrollbar_default = lv.style_t()
style_prtusb_ddlist1_extra_list_scrollbar_default.init()
style_prtusb_ddlist1_extra_list_scrollbar_default.set_radius(6)
style_prtusb_ddlist1_extra_list_scrollbar_default.set_bg_opa(255)
style_prtusb_ddlist1_extra_list_scrollbar_default.set_bg_color(lv.color_hex(0x00ff00))
prtusb_ddlist1.get_list().add_style(style_prtusb_ddlist1_extra_list_scrollbar_default, lv.PART.SCROLLBAR|lv.STATE.DEFAULT)

# Create prtusb_ddlist2
prtusb_ddlist2 = lv.dropdown(prtusb)
prtusb_ddlist2.set_options("72 DPI\n96 DPI\n150 DPI\n300 DPI\n600 DPI\n900 DPI\n1200 DPI")
prtusb_ddlist2.set_pos(354, 485)
prtusb_ddlist2.set_size(213, 50)
prtusb_ddlist2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtusb_ddlist2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtusb_ddlist2.set_style_text_color(lv.color_hex(0x0D3055), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_text_font(test_font("arial", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_border_width(1, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_border_color(lv.color_hex(0xe1e6ee), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_pad_top(4, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_pad_left(6, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_pad_right(6, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_radius(6, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_bg_grad_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtusb_ddlist2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for prtusb_ddlist2, Part: lv.PART.SELECTED, State: lv.STATE.CHECKED.
style_prtusb_ddlist2_extra_list_selected_checked = lv.style_t()
style_prtusb_ddlist2_extra_list_selected_checked.init()
style_prtusb_ddlist2_extra_list_selected_checked.set_text_color(lv.color_hex(0xffffff))
style_prtusb_ddlist2_extra_list_selected_checked.set_text_font(test_font("montserratMedium", 25))
style_prtusb_ddlist2_extra_list_selected_checked.set_border_width(1)
style_prtusb_ddlist2_extra_list_selected_checked.set_border_opa(255)
style_prtusb_ddlist2_extra_list_selected_checked.set_border_color(lv.color_hex(0xe1e6ee))
style_prtusb_ddlist2_extra_list_selected_checked.set_radius(6)
style_prtusb_ddlist2_extra_list_selected_checked.set_bg_opa(255)
style_prtusb_ddlist2_extra_list_selected_checked.set_bg_color(lv.color_hex(0x00a1b5))
prtusb_ddlist2.get_list().add_style(style_prtusb_ddlist2_extra_list_selected_checked, lv.PART.SELECTED|lv.STATE.CHECKED)
# Set style for prtusb_ddlist2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
style_prtusb_ddlist2_extra_list_main_default = lv.style_t()
style_prtusb_ddlist2_extra_list_main_default.init()
style_prtusb_ddlist2_extra_list_main_default.set_max_height(90)
style_prtusb_ddlist2_extra_list_main_default.set_text_color(lv.color_hex(0x0D3055))
style_prtusb_ddlist2_extra_list_main_default.set_text_font(test_font("arial", 25))
style_prtusb_ddlist2_extra_list_main_default.set_border_width(1)
style_prtusb_ddlist2_extra_list_main_default.set_border_opa(255)
style_prtusb_ddlist2_extra_list_main_default.set_border_color(lv.color_hex(0xe1e6ee))
style_prtusb_ddlist2_extra_list_main_default.set_radius(6)
style_prtusb_ddlist2_extra_list_main_default.set_bg_opa(255)
style_prtusb_ddlist2_extra_list_main_default.set_bg_color(lv.color_hex(0xffffff))
style_prtusb_ddlist2_extra_list_main_default.set_bg_grad_dir(lv.GRAD_DIR.VER)
style_prtusb_ddlist2_extra_list_main_default.set_bg_grad_color(lv.color_hex(0xffffff))
prtusb_ddlist2.get_list().add_style(style_prtusb_ddlist2_extra_list_main_default, lv.PART.MAIN|lv.STATE.DEFAULT)
# Set style for prtusb_ddlist2, Part: lv.PART.SCROLLBAR, State: lv.STATE.DEFAULT.
style_prtusb_ddlist2_extra_list_scrollbar_default = lv.style_t()
style_prtusb_ddlist2_extra_list_scrollbar_default.init()
style_prtusb_ddlist2_extra_list_scrollbar_default.set_radius(6)
style_prtusb_ddlist2_extra_list_scrollbar_default.set_bg_opa(255)
style_prtusb_ddlist2_extra_list_scrollbar_default.set_bg_color(lv.color_hex(0x00ff00))
prtusb_ddlist2.get_list().add_style(style_prtusb_ddlist2_extra_list_scrollbar_default, lv.PART.SCROLLBAR|lv.STATE.DEFAULT)

prtusb.update_layout()
# Create prtmb
prtmb = lv.obj()
prtmb.set_size(1024, 600)
prtmb.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtmb, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtmb.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtmb_cont0
prtmb_cont0 = lv.obj(prtmb)
prtmb_cont0.set_pos(0, 0)
prtmb_cont0.set_size(1024, 600)
prtmb_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtmb_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtmb_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtmb_btnback
prtmb_btnback = lv.btn(prtmb)
prtmb_btnback_label = lv.label(prtmb_btnback)
prtmb_btnback_label.set_text("BACK")
prtmb_btnback_label.set_long_mode(lv.label.LONG.WRAP)
prtmb_btnback_label.align(lv.ALIGN.CENTER, 0, 0)
prtmb_btnback.set_style_pad_all(0, lv.STATE.DEFAULT)
prtmb_btnback.set_pos(364, 432)
prtmb_btnback.set_size(285, 86)
prtmb_btnback.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtmb_btnback, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtmb_btnback.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_border_width(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_border_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_text_color(lv.color_hex(0xf4ecec), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_btnback.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtmb_label2
prtmb_label2 = lv.label(prtmb)
prtmb_label2.set_text("Put your phone near to the printer")
prtmb_label2.set_long_mode(lv.label.LONG.WRAP)
prtmb_label2.set_pos(21, 322)
prtmb_label2.set_size(981, 66)
prtmb_label2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtmb_label2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtmb_label2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
prtmb_label2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtmb_printer
prtmb_printer = lv.img(prtmb)
prtmb_printer.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\printer2_128_121.png"))
prtmb_printer.add_flag(lv.obj.FLAG.CLICKABLE)
prtmb_printer.set_pivot(0,0)
prtmb_printer.set_angle(0)
prtmb_printer.set_pos(328, 154)
prtmb_printer.set_size(128, 121)
prtmb_printer.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtmb_printer, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtmb_printer.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtmb_img
prtmb_img = lv.img(prtmb)
prtmb_img.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\wave_53_53.png"))
prtmb_img.add_flag(lv.obj.FLAG.CLICKABLE)
prtmb_img.set_pivot(0,0)
prtmb_img.set_angle(0)
prtmb_img.set_pos(501, 183)
prtmb_img.set_size(53, 53)
prtmb_img.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtmb_img, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtmb_img.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create prtmb_cloud
prtmb_cloud = lv.img(prtmb)
prtmb_cloud.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\phone_96_121.png"))
prtmb_cloud.add_flag(lv.obj.FLAG.CLICKABLE)
prtmb_cloud.set_pivot(0,0)
prtmb_cloud.set_angle(0)
prtmb_cloud.set_pos(597, 158)
prtmb_cloud.set_size(96, 121)
prtmb_cloud.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for prtmb_cloud, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
prtmb_cloud.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

prtmb.update_layout()
# Create printit
printit = lv.obj()
g_kb_printit = lv.keyboard(printit)
g_kb_printit.add_event_cb(lambda e: printit_ta_event_cb(e, g_kb_printit), lv.EVENT.ALL, None)
g_kb_printit.add_flag(lv.obj.FLAG.HIDDEN)
g_kb_printit.set_style_text_font(test_font("simsun", 18), lv.PART.MAIN|lv.STATE.DEFAULT)
printit.set_size(1024, 600)
printit.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for printit, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
printit.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create printit_cont0
printit_cont0 = lv.obj(printit)
printit_cont0.set_pos(0, 0)
printit_cont0.set_size(1024, 600)
printit_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for printit_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
printit_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_bg_color(lv.color_hex(0xd20000), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_bg_grad_color(lv.color_hex(0xd20000), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create printit_btnprtitback
printit_btnprtitback = lv.btn(printit)
printit_btnprtitback_label = lv.label(printit_btnprtitback)
printit_btnprtitback_label.set_text("BACK")
printit_btnprtitback_label.set_long_mode(lv.label.LONG.WRAP)
printit_btnprtitback_label.align(lv.ALIGN.CENTER, 0, 0)
printit_btnprtitback.set_style_pad_all(0, lv.STATE.DEFAULT)
printit_btnprtitback.set_pos(347, 419)
printit_btnprtitback.set_size(285, 86)
printit_btnprtitback.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for printit_btnprtitback, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
printit_btnprtitback.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_bg_color(lv.color_hex(0xd20000), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_bg_grad_color(lv.color_hex(0xd20000), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_border_width(2, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_border_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_btnprtitback.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create printit_label2
printit_label2 = lv.label(printit)
printit_label2.set_text("No internet connection")
printit_label2.set_long_mode(lv.label.LONG.WRAP)
printit_label2.set_pos(21, 322)
printit_label2.set_size(981, 66)
printit_label2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for printit_label2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
printit_label2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
printit_label2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create printit_printer
printit_printer = lv.img(printit)
printit_printer.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\printer2_128_121.png"))
printit_printer.add_flag(lv.obj.FLAG.CLICKABLE)
printit_printer.set_pivot(0,0)
printit_printer.set_angle(0)
printit_printer.set_pos(328, 154)
printit_printer.set_size(128, 121)
printit_printer.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for printit_printer, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
printit_printer.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create printit_imgnotit
printit_imgnotit = lv.img(printit)
printit_imgnotit.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\no_internet_53_53.png"))
printit_imgnotit.add_flag(lv.obj.FLAG.CLICKABLE)
printit_imgnotit.set_pivot(0,0)
printit_imgnotit.set_angle(0)
printit_imgnotit.set_pos(462, 136)
printit_imgnotit.set_size(53, 53)
printit_imgnotit.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for printit_imgnotit, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
printit_imgnotit.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create printit_cloud
printit_cloud = lv.img(printit)
printit_cloud.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\cloud_117_88.png"))
printit_cloud.add_flag(lv.obj.FLAG.CLICKABLE)
printit_cloud.set_pivot(0,0)
printit_cloud.set_angle(0)
printit_cloud.set_pos(550, 66)
printit_cloud.set_size(117, 88)
printit_cloud.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for printit_cloud, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
printit_cloud.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

printit.update_layout()
# Create setup
setup = lv.obj()
g_kb_setup = lv.keyboard(setup)
g_kb_setup.add_event_cb(lambda e: setup_ta_event_cb(e, g_kb_setup), lv.EVENT.ALL, None)
g_kb_setup.add_flag(lv.obj.FLAG.HIDDEN)
g_kb_setup.set_style_text_font(test_font("simsun", 18), lv.PART.MAIN|lv.STATE.DEFAULT)
setup.set_size(1024, 600)
setup.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for setup, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
setup.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create setup_cont0
setup_cont0 = lv.obj(setup)
setup_cont0.set_pos(0, 0)
setup_cont0.set_size(1024, 600)
setup_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for setup_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
setup_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_bg_color(lv.color_hex(0xd20000), lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_bg_grad_color(lv.color_hex(0xd20000), lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create setup_btnsetback
setup_btnsetback = lv.btn(setup)
setup_btnsetback_label = lv.label(setup_btnsetback)
setup_btnsetback_label.set_text("BACK")
setup_btnsetback_label.set_long_mode(lv.label.LONG.WRAP)
setup_btnsetback_label.align(lv.ALIGN.CENTER, 0, 0)
setup_btnsetback.set_style_pad_all(0, lv.STATE.DEFAULT)
setup_btnsetback.set_pos(343, 432)
setup_btnsetback.set_size(285, 86)
setup_btnsetback.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for setup_btnsetback, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
setup_btnsetback.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_border_width(2, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_border_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
setup_btnsetback.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create setup_label2
setup_label2 = lv.label(setup)
setup_label2.set_text("You have no permission to change the settings")
setup_label2.set_long_mode(lv.label.LONG.WRAP)
setup_label2.set_pos(21, 322)
setup_label2.set_size(981, 66)
setup_label2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for setup_label2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
setup_label2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
setup_label2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create setup_printer
setup_printer = lv.img(setup)
setup_printer.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\printer2_128_121.png"))
setup_printer.add_flag(lv.obj.FLAG.CLICKABLE)
setup_printer.set_pivot(0,0)
setup_printer.set_angle(0)
setup_printer.set_pos(328, 154)
setup_printer.set_size(128, 121)
setup_printer.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for setup_printer, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
setup_printer.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create setup_img
setup_img = lv.img(setup)
setup_img.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\no_internet_53_53.png"))
setup_img.add_flag(lv.obj.FLAG.CLICKABLE)
setup_img.set_pivot(0,0)
setup_img.set_angle(0)
setup_img.set_pos(462, 136)
setup_img.set_size(53, 53)
setup_img.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for setup_img, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
setup_img.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create setup_cloud
setup_cloud = lv.img(setup)
setup_cloud.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\cloud_117_88.png"))
setup_cloud.add_flag(lv.obj.FLAG.CLICKABLE)
setup_cloud.set_pivot(0,0)
setup_cloud.set_angle(0)
setup_cloud.set_pos(550, 66)
setup_cloud.set_size(117, 88)
setup_cloud.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for setup_cloud, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
setup_cloud.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

setup.update_layout()
# Create loader
loader = lv.obj()
g_kb_loader = lv.keyboard(loader)
g_kb_loader.add_event_cb(lambda e: loader_ta_event_cb(e, g_kb_loader), lv.EVENT.ALL, None)
g_kb_loader.add_flag(lv.obj.FLAG.HIDDEN)
g_kb_loader.set_style_text_font(test_font("simsun", 18), lv.PART.MAIN|lv.STATE.DEFAULT)
loader.set_size(1024, 600)
loader.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for loader, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
loader.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create loader_cont0
loader_cont0 = lv.obj(loader)
loader_cont0.set_pos(0, 0)
loader_cont0.set_size(1024, 600)
loader_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for loader_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
loader_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_bg_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_bg_grad_color(lv.color_hex(0x2f3243), lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create loader_loadarc
loader_loadarc = lv.arc(loader)
loader_loadarc.set_mode(lv.arc.MODE.NORMAL)
loader_loadarc.set_range(0, 100)
loader_loadarc.set_bg_angles(0, 360)
loader_loadarc.set_angles(271, 271)
loader_loadarc.set_rotation(0)
loader_loadarc.set_style_arc_rounded(0,  lv.PART.INDICATOR|lv.STATE.DEFAULT)
loader_loadarc.set_style_arc_rounded(0, lv.STATE.DEFAULT)
loader_loadarc.set_pos(384, 176)
loader_loadarc.set_size(234, 234)
loader_loadarc.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for loader_loadarc, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
loader_loadarc.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_arc_width(12, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_arc_color(lv.color_hex(0xe6e6e6), lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_radius(13, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_pad_top(20, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_pad_bottom(20, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_pad_left(20, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_pad_right(20, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadarc.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for loader_loadarc, Part: lv.PART.INDICATOR, State: lv.STATE.DEFAULT.
loader_loadarc.set_style_arc_width(12, lv.PART.INDICATOR|lv.STATE.DEFAULT)
loader_loadarc.set_style_arc_color(lv.color_hex(0x2195f6), lv.PART.INDICATOR|lv.STATE.DEFAULT)

# Set style for loader_loadarc, Part: lv.PART.KNOB, State: lv.STATE.DEFAULT.
loader_loadarc.set_style_bg_opa(255, lv.PART.KNOB|lv.STATE.DEFAULT)
loader_loadarc.set_style_bg_color(lv.color_hex(0x2195f6), lv.PART.KNOB|lv.STATE.DEFAULT)
loader_loadarc.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.KNOB|lv.STATE.DEFAULT)
loader_loadarc.set_style_bg_grad_color(lv.color_hex(0x2195f6), lv.PART.KNOB|lv.STATE.DEFAULT)
loader_loadarc.set_style_pad_all(5, lv.PART.KNOB|lv.STATE.DEFAULT)

# Create loader_loadlabel
loader_loadlabel = lv.label(loader)
loader_loadlabel.set_text("0 %")
loader_loadlabel.set_long_mode(lv.label.LONG.WRAP)
loader_loadlabel.set_pos(428, 275)
loader_loadlabel.set_size(170, 44)
loader_loadlabel.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for loader_loadlabel, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
loader_loadlabel.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
loader_loadlabel.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

loader.update_layout()
# Create saved
saved = lv.obj()
g_kb_saved = lv.keyboard(saved)
g_kb_saved.add_event_cb(lambda e: saved_ta_event_cb(e, g_kb_saved), lv.EVENT.ALL, None)
g_kb_saved.add_flag(lv.obj.FLAG.HIDDEN)
g_kb_saved.set_style_text_font(test_font("simsun", 18), lv.PART.MAIN|lv.STATE.DEFAULT)
saved.set_size(1024, 600)
saved.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for saved, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
saved.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create saved_cont0
saved_cont0 = lv.obj(saved)
saved_cont0.set_pos(0, 0)
saved_cont0.set_size(1024, 600)
saved_cont0.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for saved_cont0, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
saved_cont0.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_bg_color(lv.color_hex(0x4ab243), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_bg_grad_color(lv.color_hex(0x4ab243), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_cont0.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create saved_btnsavecontinue
saved_btnsavecontinue = lv.btn(saved)
saved_btnsavecontinue_label = lv.label(saved_btnsavecontinue)
saved_btnsavecontinue_label.set_text("CONTINUE")
saved_btnsavecontinue_label.set_long_mode(lv.label.LONG.WRAP)
saved_btnsavecontinue_label.align(lv.ALIGN.CENTER, 0, 0)
saved_btnsavecontinue.set_style_pad_all(0, lv.STATE.DEFAULT)
saved_btnsavecontinue.set_pos(358, 430)
saved_btnsavecontinue.set_size(298, 88)
saved_btnsavecontinue.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for saved_btnsavecontinue, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
saved_btnsavecontinue.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_bg_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_bg_grad_color(lv.color_hex(0x4ab241), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_border_width(2, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_border_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_radius(110, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_text_font(test_font("simsun", 25), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_btnsavecontinue.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create saved_label2
saved_label2 = lv.label(saved)
saved_label2.set_text("File saved")
saved_label2.set_long_mode(lv.label.LONG.WRAP)
saved_label2.set_pos(320, 352)
saved_label2.set_size(384, 44)
saved_label2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for saved_label2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
saved_label2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_text_font(test_font("arial", 34), lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_text_letter_space(2, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
saved_label2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create saved_img1
saved_img1 = lv.img(saved)
saved_img1.set_src(load_image(r"I:\NXP\GUI-Guider-Projects\UISlide\generated\MicroPython\ready_213_213.png"))
saved_img1.add_flag(lv.obj.FLAG.CLICKABLE)
saved_img1.set_pivot(0,0)
saved_img1.set_angle(0)
saved_img1.set_pos(394, 88)
saved_img1.set_size(213, 213)
saved_img1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for saved_img1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
saved_img1.set_style_img_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)

saved.update_layout()

def home_imgbtncopy_event_handler(e):
    code = e.get_code()
    if (code == lv.EVENT.RELEASED):
        guider_load_screen(ScreenEnum.SCR_LOADER)
        add_loader(load_copy)
        

home_imgbtncopy.add_event_cb(lambda e: home_imgbtncopy_event_handler(e), lv.EVENT.ALL, None)

def home_imgbtnset_event_handler(e):
    code = e.get_code()
    if (code == lv.EVENT.PRESSED):
        guider_load_screen(ScreenEnum.SCR_SETUP)
        lv_demo_printer_anim_in_all(setup, 200)

home_imgbtnset.add_event_cb(lambda e: home_imgbtnset_event_handler(e), lv.EVENT.ALL, None)

def home_imgbtnscan_event_handler(e):
    code = e.get_code()
    if (code == lv.EVENT.RELEASED):
        guider_load_screen(ScreenEnum.SCR_LOADER)
        add_loader(load_scan)

home_imgbtnscan.add_event_cb(lambda e: home_imgbtnscan_event_handler(e), lv.EVENT.ALL, None)

def home_imgbtnprt_event_handler(e):
    code = e.get_code()
    if (code == lv.EVENT.PRESSED):
        guider_load_screen(ScreenEnum.SCR_LOADER)
        add_loader(load_print)

home_imgbtnprt.add_event_cb(lambda e: home_imgbtnprt_event_handler(e), lv.EVENT.ALL, None)

def home_imgcopy_event_handler(e):
    code = e.get_code()

home_imgcopy.add_event_cb(lambda e: home_imgcopy_event_handler(e), lv.EVENT.ALL, None)

def home_imgscan_event_handler(e):
    code = e.get_code()

home_imgscan.add_event_cb(lambda e: home_imgscan_event_handler(e), lv.EVENT.ALL, None)

def home_imgprt_event_handler(e):
    code = e.get_code()

home_imgprt.add_event_cb(lambda e: home_imgprt_event_handler(e), lv.EVENT.ALL, None)

def home_imgset_event_handler(e):
    code = e.get_code()

home_imgset.add_event_cb(lambda e: home_imgset_event_handler(e), lv.EVENT.ALL, None)

# content from custom.py
class ScreenEnum:
    SCR_HOME = 0
    SCR_COPY_HOME = 1
    SCR_COPY_NEXT = 2
    SCR_SCAN_HOME = 3
    SCR_PRT_HOME = 4
    SCR_PRT_USB = 5
    SCR_PRT_MB = 6
    SCR_PRT_IT = 7
    SCR_SETUP = 8
    SCR_LOADER = 9
    SCR_SAVED = 10

LV_DEMO_PRINTER_ANIM_DELAY=40
LV_DEMO_PRINTER_ANIM_TIME=150
LV_DEMO_PRINTER_ANIM_TIME_BG=300
LOAD_ANIM_TIME=1000
lightness_act = 0
hue_act = 180
cur_scr = ScreenEnum.SCR_HOME
copy_counter = 1
prtusb_counter = 1
save_src = 0

def get_scr_by_id(scr_id):
    if(scr_id == ScreenEnum.SCR_HOME):
        return home
    elif(scr_id == ScreenEnum.SCR_COPY_HOME):
        return copyhome
    elif(scr_id == ScreenEnum.SCR_COPY_NEXT):
        return copynext
    elif(scr_id == ScreenEnum.SCR_SCAN_HOME):
        return scanhome
    elif(scr_id == ScreenEnum.SCR_PRT_HOME):
        return prthome
    elif(scr_id == ScreenEnum.SCR_PRT_USB):
        return prtusb
    elif(scr_id == ScreenEnum.SCR_PRT_MB):
        return prtmb
    elif(scr_id == ScreenEnum.SCR_PRT_IT):
        return printit
    elif(scr_id == ScreenEnum.SCR_SETUP):
        return setup
    elif(scr_id == ScreenEnum.SCR_LOADER):
        return loader
    elif(scr_id == ScreenEnum.SCR_SAVED):
        return saved

def load_disbtn_home_cb(e):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
        guider_load_screen(ScreenEnum.SCR_HOME)
        lv_demo_printer_anim_in_all(home, 100)

def load_copy_next_cb(e):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
         guider_load_screen(ScreenEnum.SCR_COPY_NEXT)
         lv_demo_printer_anim_in_all(copynext, 200)

def hue_slider_event_cb(e, obj):
    src = e.get_target()
    code = e.get_code()
    if(code == lv.EVENT.VALUE_CHANGED):
        global hue_act
        hue_act = src.get_value()
        scan_img_color_refr(obj)

def lightness_slider_event_cb(e, obj):
    src = e.get_target()
    code = e.get_code()
    if(code == lv.EVENT.VALUE_CHANGED):
        global lightness_act
        lightness_act = src.get_value()
        scan_img_color_refr(obj)

def loader_anim_cb(arc, v):
    if(v > 100):
        v = 100
    arc.set_angles(270, int(v * 360 / 100 + 270))
    loader_loadlabel.set_text(str(v))

def copy_counter_event_cb(e, obj):
    code = e.get_code()
    print(code)
    if(code == lv.EVENT.PRESSED):
        global copy_counter
        if (obj == copynext_up):
            if(copy_counter < 200):
                copy_counter += 1
        else:
            if (copy_counter > 1):
                copy_counter -= 1
        print(copy_counter, obj)
        copynext_labelcnt.set_text(str(copy_counter))

def prtusb_counter_event_cb(e, obj):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
        global prtusb_counter
        if (obj == prtusb_up):
            if(prtusb_counter < 200):
                prtusb_counter += 1
        else:
            if (prtusb_counter > 1):
                prtusb_counter -= 1
        print(prtusb_counter, obj)
        prtusb_labelcnt.set_text(str(prtusb_counter))

def load_print_finish_cb(e):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
        global save_src
        save_src = 2
        guider_load_screen(ScreenEnum.SCR_LOADER)
        add_loader(load_save)

def load_save_cb(e):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
        guider_load_screen(ScreenEnum.SCR_LOADER)
        global save_src
        save_src = 1
        add_loader(load_save)

def load_print_usb_cb(e):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
        guider_load_screen(ScreenEnum.SCR_PRT_USB)
        lv_demo_printer_anim_in_all(prtusb, 200)

def load_print_mobile_cb(e):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
        guider_load_screen(ScreenEnum.SCR_PRT_MB)
        lv_demo_printer_anim_in_all(prtmb, 200)

def load_print_it_cb(e):
    code = e.get_code()
    if(code == lv.EVENT.PRESSED):
        guider_load_screen(ScreenEnum.SCR_PRT_IT)
        lv_demo_printer_anim_in_all(printit, 200)

def copy_home_event_init():
    copyhome_btncopyback.add_event_cb(lambda e: load_disbtn_home_cb(e), lv.EVENT.CLICKED, None)

def scan_img_color_refr(obj):
    if lightness_act > 0:
        s = 100 - lightness_act
        v = 100
    else:
        s = 100
        v = 100 + lightness_act
    c = lv.color_hsv_to_rgb(hue_act,s,v)
    obj.set_style_img_recolor_opa(v, 0)
    obj.set_style_img_recolor(c, 0)

def anim_y_cb(obj, v):
    obj.set_y(v)

def lv_demo_printer_anim_in_all(obj, delay):
    child_cnts = lv.obj.get_child_cnt(obj)
    for i in range(child_cnts):
        child = lv.obj.get_child(obj, i)
        child.update_layout()
        # a = lv.anim_t()
        # a.init()
        # a.set_var(child)
        # a.set_time(LV_DEMO_PRINTER_ANIM_TIME)
        # a.set_delay(delay)
        # a.set_custom_exec_cb(lambda a, val: anim_y_cb(child,val))
        # a.set_values(child.get_y() - int(lv.scr_act().get_disp().driver.ver_res / 20), child.get_y())
        # lv.anim_t.start(a)
        child.fade_in(LV_DEMO_PRINTER_ANIM_TIME - 100, delay)

def load_copy(a):
    guider_load_screen(ScreenEnum.SCR_COPY_HOME)
    lv_demo_printer_anim_in_all(copyhome, 200)

def load_save(a):
    guider_load_screen(ScreenEnum.SCR_SAVED)
    if(save_src == 1):
        saved_label2.set_x(157)
        saved_label2.set_text("File saved")
    elif(save_src == 2):
        saved_label2.set_x(157)
        saved_label2.set_text("Printing finished")
    else:
        saved_label2.set_x(157)
        saved_label2.set_text("File saved")
    lv_demo_printer_anim_in_all(saved, 200)

def load_home(a):
    guider_load_screen(ScreenEnum.SCR_HOME)

def load_scan(a):
    guider_load_screen(ScreenEnum.SCR_SCAN_HOME)
    lv_demo_printer_anim_in_all(scanhome, 200)

def load_setup(a):
    guider_load_screen(ScreenEnum.SCR_SETUP)
    lv_demo_printer_anim_in_all(setup, 200)

def load_print(a):
    guider_load_screen(ScreenEnum.SCR_PRT_HOME)
    lv_demo_printer_anim_in_all(prthome, 200)

# loading event function support.
def add_loader(end_cb):
    loader_loadarc.set_angles(270, 270)
    a = lv.anim_t()
    a.init()
    a.set_time(LOAD_ANIM_TIME)
    a.set_values(0, 110)
    a.set_var(loader_loadarc)
    a.set_custom_exec_cb(lambda a,val: loader_anim_cb(loader_loadarc,val))
    a.set_ready_cb(end_cb)
    lv.anim_t.start(a)

def guider_load_screen(scr_id):
    scr = None
    if(scr_id == ScreenEnum.SCR_HOME):
        scr = home
    elif(scr_id == ScreenEnum.SCR_COPY_HOME):
        scr = copyhome
        copyhome_img3.set_style_radius(8, lv.STATE.DEFAULT)
        copyhome_img3.set_style_clip_corner(True, lv.STATE.DEFAULT)
        copyhome_img3.set_style_bg_img_recolor_opa(180, lv.STATE.DEFAULT)

    elif(scr_id == ScreenEnum.SCR_COPY_NEXT):
        scr = copynext
        global copy_counter
        copy_counter = 1
        copynext_labelcnt.set_text(str(copy_counter))
        copynext_print.clear_flag(False)
    elif(scr_id == ScreenEnum.SCR_SCAN_HOME):
        scr = scanhome
        scanhome_img3.set_style_radius(8, lv.STATE.DEFAULT)
        scanhome_img3.set_style_clip_corner(True, lv.STATE.DEFAULT)
        scanhome_img3.set_style_bg_img_recolor_opa(180, lv.STATE.DEFAULT)
    elif(scr_id == ScreenEnum.SCR_PRT_HOME):
        scr = prthome
        global prtusb_counter
        prtusb_counter = 1
        prtusb_labelcnt.set_text(str(prtusb_counter))
    elif(scr_id == ScreenEnum.SCR_PRT_USB):
        scr = prtusb
    elif(scr_id == ScreenEnum.SCR_PRT_MB):
        scr = prtmb
    elif(scr_id == ScreenEnum.SCR_PRT_IT):
        scr = printit
    elif(scr_id == ScreenEnum.SCR_SETUP):
        scr = setup
    elif(scr_id == ScreenEnum.SCR_LOADER):
        scr = loader
        # loader_loadarc.add_style(lv.STATE.DEFAULT)
    elif(scr_id == ScreenEnum.SCR_SAVED):
        scr = saved
        saved_btnsavecontinue_label.set_style_text_font(lv.font_montserrat_14, lv.STATE.DEFAULT)
    else:
        scr = None
    
    lv.scr_load(scr)

copyhome_btncopyback.add_event_cb(load_disbtn_home_cb, lv.EVENT.ALL, None)
copyhome_btncopynext.add_event_cb(load_copy_next_cb, lv.EVENT.ALL, None)
copyhome_sliderhue.add_event_cb(lambda e: hue_slider_event_cb(e, copyhome_img3), lv.EVENT.ALL, None)
copyhome_sliderbright.add_event_cb(lambda e: lightness_slider_event_cb(e, copyhome_img3), lv.EVENT.ALL, None)
copynext_up.add_event_cb(lambda e: copy_counter_event_cb(e, copynext_up),lv.EVENT.ALL, None)
copynext_down.add_event_cb(lambda e: copy_counter_event_cb(e, copynext_down),lv.EVENT.ALL, None)
copynext_print.add_event_cb(lambda e: load_print_finish_cb(e), lv.EVENT.ALL, None)
scanhome_btnscanback.add_event_cb(lambda e: load_disbtn_home_cb(e), lv.EVENT.ALL, None)
scanhome_btnscansave.add_event_cb(lambda e: load_save_cb(e), lv.EVENT.ALL, None)
scanhome_sliderhue.add_event_cb(lambda e: hue_slider_event_cb(e, scanhome_img3), lv.EVENT.ALL, None)
scanhome_sliderbright.add_event_cb(lambda e: lightness_slider_event_cb(e, scanhome_img3), lv.EVENT.ALL, None)
prthome_imgbtnusb.add_event_cb(load_print_usb_cb, lv.EVENT.ALL, None)
prthome_imgbtnmobile.add_event_cb(load_print_mobile_cb, lv.EVENT.ALL, None)
prthome_imgbtnit.add_event_cb(load_print_it_cb, lv.EVENT.ALL, None)
prthome_btnprintback.add_event_cb(load_disbtn_home_cb, lv.EVENT.ALL, None)
prtusb_back.add_event_cb(load_disbtn_home_cb, lv.EVENT.ALL, None)
prtusb_btnprint.add_event_cb(load_print_finish_cb, lv.EVENT.ALL, None)
prtusb_up.add_event_cb(lambda e: prtusb_counter_event_cb(e, prtusb_up),lv.EVENT.ALL, None)
prtusb_down.add_event_cb(lambda e: prtusb_counter_event_cb(e, prtusb_down),lv.EVENT.ALL, None)
prtmb_btnback.add_event_cb(load_disbtn_home_cb, lv.EVENT.ALL, None)
printit_btnprtitback.add_event_cb(load_disbtn_home_cb, lv.EVENT.ALL, None)
setup_btnsetback.add_event_cb(load_disbtn_home_cb, lv.EVENT.ALL, None)
saved_btnsavecontinue.add_event_cb(lambda e: load_disbtn_home_cb(e), lv.EVENT.ALL, None)

# Load the default screen
lv.scr_load(home)

while SDL.check():
    time.sleep_ms(5)

