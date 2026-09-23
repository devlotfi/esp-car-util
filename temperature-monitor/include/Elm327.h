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

BluetoothSerial SerialBT;

static int readFailures = 0;
static int recoveriesWithoutData = 0;

// True only while Bluetooth is linked AND the ELM327 is initialized.
// volatile so other tasks can read it safely (single bool, no lock needed).
volatile bool elmConnected = false;

void onTemperature(int tempC)
{
  Serial.printf("Coolant temp: %d C\n", tempC);
  LvglMessage lvglMessage{};
  lvglMessage.type = LvglMessageType::UpdateCoolantTemperature;
  lvglMessage.data.updateCoolantTemperatureLvglMessage.temperature = tempC;
  xQueueSend(lvgl_message_queue_handle, &lvglMessage, 0);
}

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

// Updates the flag and fires the callback only on a real state change
static void setConnected(bool state)
{
  if (elmConnected == state)
    return;
  elmConnected = state;
  if (state)
    onConnected();
  else
    onDisconnected();
}

bool readElmResponse(String &response, uint32_t timeoutMs)
{
  response = "";
  uint32_t lastActivity = millis();

  while (millis() - lastActivity < timeoutMs)
  {
    while (SerialBT.available())
    {
      char c = SerialBT.read();
      lastActivity = millis();
      if (c == '>')
        return true;
      response += c;
    }
    delay(1);
  }
  return false;
}

// Makes a response readable in the log (CR -> space, trimmed)
String cleanForLog(String s)
{
  s.replace('\r', ' ');
  s.replace('\n', ' ');
  s.trim();
  return s;
}

// Sends a command terminated with CR only, as ONE Bluetooth write, and waits
// for the reply. Returns true if the '>' prompt was received.
bool sendCommand(const char *cmd, String &response, uint32_t timeoutMs)
{
  // Drop any stale data left over from a previous command
  while (SerialBT.available())
    SerialBT.read();

#if DEBUG_RAW
  Serial.printf(">> %s\n", cmd);
#endif

  // Command + CR in a single buffer (one SPP packet). Splitting them into two
  // writes makes some ELM327 clones hang waiting for the line ending.
  String out = String(cmd) + "\r";
  SerialBT.write((const uint8_t *)out.c_str(), out.length());
  SerialBT.flush();

  bool gotPrompt = readElmResponse(response, timeoutMs);

#if DEBUG_RAW
  Serial.print("[raw] ");
  for (size_t i = 0; i < response.length(); i++)
  {
    Serial.printf("%02X ", (uint8_t)response[i]);
  }
  Serial.println();
  Serial.printf("<< %s\n", cleanForLog(response).c_str());
  if (!gotPrompt)
    Serial.println("(timeout: no '>' prompt received)");
#endif

  return gotPrompt;
}

// ---------------- Initialization ----------------

struct InitStep
{
  const char *cmd;
  const char *expect; // text that must appear in the reply
};

const InitStep INIT_STEPS[] = {
    {"ATZ", "ELM"},  // reset, replies with the version string
    {"ATE0", "OK"},  // echo off
    {"ATL0", "OK"},  // linefeeds off
    {"ATS0", "OK"},  // spaces off
    {"ATH0", "OK"},  // headers off
    {"ATSP0", "OK"}, // protocol: automatic
};

bool initElm()
{
  for (int attempt = 1; attempt <= INIT_ATTEMPTS; attempt++)
  {
    Serial.printf("Initializing ELM327 (attempt %d/%d)...\n", attempt, INIT_ATTEMPTS);

    bool ok = true;
    for (const InitStep &step : INIT_STEPS)
    {
      String response;
      bool gotPrompt = sendCommand(step.cmd, response, INIT_TIMEOUT_MS);

      if (!gotPrompt || response.indexOf(step.expect) < 0)
      {
        Serial.printf("  %s failed (got: \"%s\")\n", step.cmd, cleanForLog(response).c_str());
        ok = false;
        break;
      }
      Serial.printf("  %s ok\n", step.cmd);
    }

    if (ok)
    {
      Serial.println("ELM327 initialized");
      return true;
    }
    delay(1000);
  }
  return false;
}

// ---------------- Coolant temperature ----------------

// Parses one cleaned line (no spaces, upper case) looking for "4105 XX".
bool parseCoolantLine(const String &line, int &tempC)
{
  if (line.length() < 6 || !line.startsWith("4105"))
    return false;
  if (!isxdigit(line[4]) || !isxdigit(line[5]))
    return false;

  int a = (int)strtol(line.substring(4, 6).c_str(), nullptr, 16);
  tempC = a - 40; // OBD-II PID 05: temp (C) = A - 40
  return true;
}

// Scans the whole reply line by line, ignoring noise such as "SEARCHING...".
// Works with spaces on or off (spaces are stripped before matching).
bool parseCoolantTemp(const String &response, int &tempC)
{
  String line;
  for (size_t i = 0; i <= response.length(); i++)
  {
    char c = (i < response.length()) ? response[i] : '\r'; // flush last line

    if (c == '\r' || c == '\n')
    {
      if (parseCoolantLine(line, tempC))
        return true;
      line = "";
    }
    else if (c != ' ')
    {
      line += (char)toupper(c);
    }
  }
  return false;
}

bool readCoolantTemp(int &tempC)
{
  String response;
  if (!sendCommand("0105", response, PID_TIMEOUT_MS))
  {
    Serial.println("Read failed: timeout waiting for reply");
    return false;
  }
  if (!parseCoolantTemp(response, tempC))
  {
    Serial.printf("Read failed: could not parse \"%s\"\n", cleanForLog(response).c_str());
    return false;
  }
  return true;
}

// ---------------- Connection management ----------------

void restartEsp(const char *reason)
{
  setConnected(false);
  Serial.printf("FATAL: %s. Restarting ESP32 in 3 seconds...\n", reason);
  Serial.flush();
  delay(3000);
  ESP.restart();
}

bool connectBluetooth()
{
  for (int attempt = 1; attempt <= BT_CONNECT_ATTEMPTS; attempt++)
  {
    Serial.printf("Connecting to ELM327 over Bluetooth (attempt %d/%d)...\n",
                  attempt, BT_CONNECT_ATTEMPTS);

    SerialBT.disconnect(); // clean up any half-open link (no-op if none)

    if (SerialBT.connect(elm327Address))
    {
      Serial.println("Bluetooth connected");
      // Let the dongle settle and discard anything it sent on connect
      delay(1000);
      while (SerialBT.available())
        SerialBT.read();
      return true;
    }

    Serial.println("Bluetooth connection failed");
    delay(BT_RETRY_DELAY_MS);
  }
  return false;
}

// Bluetooth connect + ELM327 init, repeated as a whole a few times.
// If nothing works, the ESP32 restarts. Returns only when connected and ready.
void establishConnection()
{
  for (int cycle = 1; cycle <= MAX_CONNECT_CYCLES; cycle++)
  {
    Serial.printf("=== Connection cycle %d/%d ===\n", cycle, MAX_CONNECT_CYCLES);

    if (connectBluetooth() && initElm())
    {
      readFailures = 0;
      setConnected(true);
      return;
    }
  }
  restartEsp("Could not connect to / initialize the ELM327");
}

// Used when a working connection stops giving data. Repeated recoveries that
// never produce a valid temperature lead to an ESP32 restart.
void recover(const char *reason)
{
  Serial.printf("%s\n", reason);
  setConnected(false);

  recoveriesWithoutData++;
  if (recoveriesWithoutData > MAX_RECOVERIES_WITHOUT_DATA)
  {
    restartEsp("Reconnecting did not restore valid temperature data");
  }

  Serial.printf("Recovery %d/%d...\n", recoveriesWithoutData, MAX_RECOVERIES_WITHOUT_DATA);
  establishConnection();
}

static void elm327_task(void *arg)
{
  SerialBT.setPin(elm327Pin, 4);
  if (!SerialBT.begin("ESP32_OBD", true))
  {
    Serial.println("Failed to initialize Bluetooth");
    restartEsp("Bluetooth stack failed to start");
  }
  establishConnection();

  uint32_t lastReadMs = 0;

  while (true)
  {
    // Bluetooth link dropped
    if (!SerialBT.connected())
    {
      recover("Bluetooth link lost");
      lastReadMs = millis();
      continue;
    }

    if (millis() - lastReadMs < READ_INTERVAL_MS)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    int tempC;
    if (readCoolantTemp(tempC))
    {
      Serial.printf("Coolant temp: %d C\n", tempC);
      readFailures = 0;
      recoveriesWithoutData = 0;
      onTemperature(tempC);
    }
    else
    {
      readFailures++;
      Serial.printf("Read failure %d/%d\n", readFailures, MAX_READ_FAILURES);

      if (readFailures >= MAX_READ_FAILURES)
      {
        recover("Too many consecutive read failures");
      }
    }

    lastReadMs = millis();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}