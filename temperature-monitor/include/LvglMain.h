#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "Properties.h"
#include "LvglUI.h"

static TFT_eSPI tft = TFT_eSPI();
static lv_display_t *lv_display = nullptr;
static uint8_t *draw_buf;

enum class LvglMessageType : uint8_t
{
  ShowScreen,
  UpdateStats
};

struct UpdateStatsLvglMessage
{
  int temperature_c;
  int speed_kmh;
  int load_percent;
};

struct ShowScreenLvglMessage
{
  ScreenType screen;
};

struct LvglMessage
{
  LvglMessageType type;
  union
  {
    ShowScreenLvglMessage showScreenLvglMessage;
    UpdateStatsLvglMessage updateStatsLvglMessage;
  } data;
};

void my_flush_cb(
    lv_display_t *disp,
    const lv_area_t *area,
    uint8_t *px_map)
{
  uint32_t w =
      area->x2 - area->x1 + 1;

  uint32_t h =
      area->y2 - area->y1 + 1;

  tft.startWrite();

  tft.setAddrWindow(
      area->x1,
      area->y1,
      w,
      h);

  tft.pushColors(
      (uint16_t *)px_map,
      w * h,
      true);

  tft.endWrite();

  lv_display_flush_ready(disp);
}

static void lvgl_task(void *arg)
{
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  lv_init();
  lv_tick_set_cb(millis);

  size_t buffer_size =
      320 * 24;
  draw_buf =
      (uint8_t *)heap_caps_malloc(
          buffer_size * sizeof(lv_color_t),
          MALLOC_CAP_INTERNAL |
              MALLOC_CAP_8BIT);
  lv_display =
      lv_display_create(
          320,
          240);
  lv_display_set_flush_cb(
      lv_display,
      my_flush_cb);
  lv_display_set_buffers(
      lv_display,
      draw_buf,
      nullptr,
      buffer_size,
      LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_obj_t *screen =
      lv_screen_active();
  lv_obj_set_style_bg_color(
      screen,
      lv_color_hex(0x000000),
      0);
  lv_obj_set_style_bg_opa(
      screen,
      LV_OPA_COVER,
      0);

  show_disconnected_screen();

  while (true)
  {
    LvglMessage lvglMessage;
    while (xQueueReceive(lvgl_message_queue_handle, &lvglMessage, 0) == pdTRUE)
    {
      if (lvglMessage.type == LvglMessageType::ShowScreen)
      {
        if (lvglMessage.data.showScreenLvglMessage.screen == ScreenType::SCREEN_COOLANT_GAUGE)
        {
          show_coolant_gauge();
        }
        else if (lvglMessage.data.showScreenLvglMessage.screen == ScreenType::SCREEN_DISCONNECTED)
        {
          show_disconnected_screen();
        }
      }
      else if (lvglMessage.type == LvglMessageType::UpdateStats)
      {
        Serial.println("Recived data messsage");
        if (current_screen == ScreenType::SCREEN_COOLANT_GAUGE)
        {
          Serial.printf("Recived data lvgl: %f %f %f\n", (float)lvglMessage.data.updateStatsLvglMessage.temperature_c, (float)lvglMessage.data.updateStatsLvglMessage.speed_kmh, (float)lvglMessage.data.updateStatsLvglMessage.load_percent);
          update_coolant_temp(lvglMessage.data.updateStatsLvglMessage.temperature_c);
          update_speed(lvglMessage.data.updateStatsLvglMessage.speed_kmh);
          update_load(lvglMessage.data.updateStatsLvglMessage.load_percent);
        }
      }
    }

    lv_tick_inc(5);
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}