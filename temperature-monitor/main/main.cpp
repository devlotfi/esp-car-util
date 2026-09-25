#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "BluetoothSerial.h"
#include "Properties.h"
#include "Vars.h"
#include "Elm327.h"
#include "LvglUI.h"
#include "LvglMain.h"

void setup()
{
  Serial.begin(115200);

  lvgl_message_queue_handle = xQueueCreate(
      10,
      sizeof(LvglMessage));
  if (lvgl_message_queue_handle == NULL)
  {
    Serial.println("Failed to create message queue");
    return;
  }

  BaseType_t task_result_lvgl =
      xTaskCreate(
          lvgl_task,
          "lvgl",
          8192,
          NULL,
          5,
          &lvgl_task_handle);
  if (task_result_lvgl != pdPASS)
  {
    Serial.println("Failed to create LVGL task");
    return;
  }

  /* BaseType_t task_result_elm327 =
      xTaskCreate(
          elm327_task,
          "elm327",
          8192,
          NULL,
          5,
          &elm327_task_handle);
  if (task_result_elm327 != pdPASS)
  {
    Serial.println("Failed to create ELM327 task");
    return;
  } */
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(10));
}