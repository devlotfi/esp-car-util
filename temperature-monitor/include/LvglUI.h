#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "Properties.h"

enum ScreenType
{
    SCREEN_DISCONNECTED,
    SCREEN_COOLANT_GAUGE
};

static ScreenType current_screen = ScreenType::SCREEN_DISCONNECTED;
static lv_obj_t *temp_arc;
static lv_obj_t *temp_label;

// ==================================================
// Update coolant temperature
// ==================================================

void update_coolant_temp(int temp_c)
{
    if (current_screen != ScreenType::SCREEN_COOLANT_GAUGE)
        return;

    // Clamp to arc range
    if (temp_c < TEMP_MIN)
        temp_c = TEMP_MIN;

    if (temp_c > TEMP_MAX)
        temp_c = TEMP_MAX;

    lv_arc_set_value(temp_arc, temp_c);

    char buf[16];

    lv_snprintf(
        buf,
        sizeof(buf),
        "%d°C",
        temp_c);

    lv_label_set_text(temp_label, buf);

    bool hot = temp_c >= TEMP_HOT_THRESHOLD;

    lv_color_t color =
        hot
            ? lv_color_hex(0xff0e00)
            : lv_color_hex(0x0052FF);

    lv_obj_set_style_arc_color(
        temp_arc,
        color,
        LV_PART_INDICATOR);

    lv_obj_set_style_text_color(
        temp_label,
        color,
        0);
}

// ==================================================
// Create coolant gauge
// ==================================================

void create_coolant_gauge(lv_obj_t *parent)
{
    temp_arc = lv_arc_create(parent);

    lv_obj_set_size(
        temp_arc,
        220,
        220);

    lv_arc_set_rotation(
        temp_arc,
        135);

    lv_arc_set_bg_angles(
        temp_arc,
        0,
        270);

    lv_arc_set_range(
        temp_arc,
        TEMP_MIN,
        TEMP_MAX);

    lv_arc_set_value(
        temp_arc,
        TEMP_MIN);

    // Display-only gauge
    lv_obj_remove_style(
        temp_arc,
        NULL,
        LV_PART_KNOB);

    lv_obj_clear_flag(
        temp_arc,
        LV_OBJ_FLAG_CLICKABLE);

    // Background arc
    lv_obj_set_style_arc_color(
        temp_arc,
        lv_color_hex(0x161824),
        LV_PART_MAIN);

    lv_obj_set_style_arc_width(
        temp_arc,
        14,
        LV_PART_MAIN);

    // Indicator arc
    lv_obj_set_style_arc_color(
        temp_arc,
        lv_color_hex(0x0052FF),
        LV_PART_INDICATOR);

    lv_obj_set_style_arc_width(
        temp_arc,
        14,
        LV_PART_INDICATOR);

    lv_obj_center(temp_arc);

    // Temperature label
    temp_label = lv_label_create(parent);

    lv_obj_set_style_text_font(
        temp_label,
        &lv_font_montserrat_48,
        0);

    lv_obj_set_style_text_color(
        temp_label,
        lv_color_hex(0x0052FF),
        0);

    lv_label_set_text(
        temp_label,
        "--°C");

    lv_obj_center(temp_label);

    // Caption
    lv_obj_t *caption_label =
        lv_label_create(parent);

    lv_obj_set_style_text_font(
        caption_label,
        &lv_font_montserrat_14,
        0);

    lv_obj_set_style_text_color(
        caption_label,
        lv_color_hex(0x9AA5CE),
        0);

    lv_label_set_text(
        caption_label,
        "COOLANT");

    lv_obj_align_to(
        caption_label,
        temp_label,
        LV_ALIGN_OUT_BOTTOM_MID,
        0,
        20);

    update_coolant_temp(TEMP_MIN);
}

// ==================================================
// Create disconnected screen
// ==================================================

void create_disconnected_screen(lv_obj_t *parent)
{
    lv_obj_t *label =
        lv_label_create(parent);

    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_32,
        0);

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(0xff0e00),
        0);

    lv_label_set_text(
        label,
        "Disconnected");

    lv_obj_center(label);
}

// ==================================================
// Show disconnected screen
// ==================================================

void show_disconnected_screen()
{
    current_screen = ScreenType::SCREEN_DISCONNECTED;

    lv_obj_t *screen =
        lv_screen_active();

    // Remove everything currently on screen
    lv_obj_clean(screen);

    // Create disconnected UI
    create_disconnected_screen(screen);
}

// ==================================================
// Show coolant gauge
// ==================================================

void show_coolant_gauge()
{
    current_screen = SCREEN_COOLANT_GAUGE;

    lv_obj_t *screen =
        lv_screen_active();

    // Remove everything currently on screen
    lv_obj_clean(screen);

    // Create gauge UI
    create_coolant_gauge(screen);
}