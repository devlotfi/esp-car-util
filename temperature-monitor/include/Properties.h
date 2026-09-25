#pragma once

#include "ELMduino.h"

#define TEMP_MIN 40
#define TEMP_MAX 120
#define TEMP_HOT_THRESHOLD 100

uint8_t elm327Address[6] = {0x00, 0x10, 0xCC, 0x4F, 0x36, 0x03};
const char *elm327Pin = "1234";
const char elm327Protocol = ISO_15765_11_BIT_500_KBAUD;
const bool ELM_DEBUG = false;
const uint16_t ELM_TIMEOUT_MS = 2000;
const uint32_t READ_INTERVAL_MS = 1000;
const uint32_t RETRY_DELAY_MS = 3000;