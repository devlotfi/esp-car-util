#pragma once

#include <Arduino.h>
#include "BluetoothSerial.h"
#include "Properties.h"
#include "Vars.h"
#include "LvglUI.h"
#include "LvglMain.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Use an ESP32 with Bluetooth Classic support.
#endif

// Wraps another Stream (e.g. BluetoothSerial) and holds outgoing bytes until a
// CR ('\r') is written, then sends everything as ONE write (one SPP packet).
//
// Needed because ELMduino sends a command with print(cmd) followed by
// print('\r'), which BluetoothSerial turns into two separate packets that some
// ELM327 clones mishandle.
class SingleWriteStream : public Stream
{
public:
  explicit SingleWriteStream(Stream &inner) : _inner(inner) {}

  using Print::write; // keep the other write() overloads visible

  // ---- Input: passed straight through ----
  int available() override { return _inner.available(); }
  int read() override { return _inner.read(); }
  int peek() override { return _inner.peek(); }

  // ---- Output: buffered until CR, then sent in one go ----
  size_t write(uint8_t b) override
  {
    if (_len >= sizeof(_buf))
      flush(); // safety: never overflow
    _buf[_len++] = b;
    if (b == '\r')
      flush();
    return 1;
  }

  size_t write(const uint8_t *data, size_t size) override
  {
    for (size_t i = 0; i < size; i++)
      write(data[i]);
    return size;
  }

  void flush() override
  {
    if (_len > 0)
    {
      _inner.write(_buf, _len);
      _inner.flush();
      _len = 0;
    }
  }

private:
  Stream &_inner;
  uint8_t _buf[128];
  size_t _len = 0;
};

BluetoothSerial SerialBT;
SingleWriteStream elmPort(SerialBT);
ELM327 elm327;
volatile bool querying = false;
volatile uint32_t lastReadMs = 0;

void onConnected()
{
  LvglMessage lvglMessage{};
  lvglMessage.type = LvglMessageType::ShowScreen;
  lvglMessage.data.showScreenLvglMessage.screen = ScreenType::SCREEN_COOLANT_GAUGE;
  xQueueSend(lvgl_message_queue_handle, &lvglMessage, 0);
}

void onDisconnected()
{
  LvglMessage lvglMessage{};
  lvglMessage.type = LvglMessageType::ShowScreen;
  lvglMessage.data.showScreenLvglMessage.screen = ScreenType::SCREEN_DISCONNECTED;
  xQueueSend(lvgl_message_queue_handle, &lvglMessage, 0);
}

void onTemperature(float coolantC)
{
  LvglMessage lvglMessage{};
  lvglMessage.type = LvglMessageType::UpdateCoolantTemperature;
  lvglMessage.data.updateCoolantTemperatureLvglMessage.temperature = (int)coolantC;
  xQueueSend(lvgl_message_queue_handle, &lvglMessage, 0);
}

bool connectAndInitElm()
{
  Serial.println("Connecting to ELM327 over Bluetooth...");
  if (!SerialBT.connect(elm327Address))
  {
    Serial.println("Bluetooth connection failed");
    return false;
  }
  Serial.println("Bluetooth connected");

  // Let the dongle settle and discard anything it sent on connect
  delay(1000);
  while (SerialBT.available())
    SerialBT.read();

  // begin() allocates the payload buffer each call and never frees a previous
  // one, so free it ourselves before retrying (global object => starts as nullptr)
  if (elm327.payload)
  {
    free(elm327.payload);
    elm327.payload = nullptr;
  }

  Serial.println("Initializing ELM327 (protocol search can take a while)...");
  if (!elm327.begin(elmPort, ELM_DEBUG, ELM_TIMEOUT_MS, elm327Protocol))
  {
    Serial.println("ELM327 init failed (is the ignition on?)");
    return false;
  }

  Serial.println("ELM327 ready");
  onConnected();
  return true;
}

static void elm327_task(void *arg)
{
  SerialBT.setPin(elm327Pin, 4);

  if (!SerialBT.begin("ESP32_OBD", true))
  {
    Serial.println("Failed to initialize Bluetooth");
    while (true)
      delay(1000);
  }

  while (!connectAndInitElm())
  {
    Serial.println("Retrying...");
    delay(RETRY_DELAY_MS);
  }

  lastReadMs = millis();

  while (true)
  {
    if (!SerialBT.connected())
    {
      Serial.println("Bluetooth link lost, reconnecting...");
      querying = false;
      onDisconnected();
      while (!connectAndInitElm())
      {
        Serial.println("Retrying...");
        delay(RETRY_DELAY_MS);
      }
      lastReadMs = millis();
      continue;
    }

    // Wait for the next read slot (unless a query is already in flight)
    if (!querying && (millis() - lastReadMs < READ_INTERVAL_MS))
    {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    // Non-blocking: the first call sends the query, later calls poll for the reply
    querying = true;
    float coolantC = elm327.engineCoolantTemp();

    if (elm327.nb_rx_state == ELM_SUCCESS)
    {
      Serial.printf("Coolant temp: %.1f C\n", coolantC);
      onTemperature(coolantC);
      querying = false;
      lastReadMs = millis();
    }
    else if (elm327.nb_rx_state != ELM_GETTING_MSG)
    {
      elm327.printError(); // timeout, no data, etc.
      querying = false;
      lastReadMs = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}