#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Properties.h"

static QueueHandle_t lvgl_message_queue_handle = nullptr;
static TaskHandle_t lvgl_task_handle = nullptr;
static TaskHandle_t elm327_task_handle = nullptr;