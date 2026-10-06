#pragma once

#include <Arduino.h>
#include "Properties.h"
#include "Vars.h"

static void buzzer_task(void *arg)
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    digitalWrite(BUZZER_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(BUZZER_PIN, LOW);
  }
}