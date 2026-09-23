#pragma once

#define TEMP_MIN 40
#define TEMP_MAX 120
#define TEMP_HOT_THRESHOLD 100

uint8_t elm327Address[6] = {0x00, 0x10, 0xCC, 0x4F, 0x36, 0x03};
const char *elm327Pin = "1234";

// Set to 1 to print every command and the raw response bytes in hex (0D = CR)
#define DEBUG_RAW 0

const uint32_t INIT_TIMEOUT_MS = 5000;   // wait for replies to AT commands
const uint32_t PID_TIMEOUT_MS = 12000;   // wait for a PID reply (first one after ATSP0
                                         // triggers the protocol search, which is slow)
const uint32_t READ_INTERVAL_MS = 1000;  // time between coolant temp reads
const uint32_t BT_RETRY_DELAY_MS = 3000; // delay between Bluetooth connect attempts

// ---------------- Safety limits ----------------
const int BT_CONNECT_ATTEMPTS = 5;         // Bluetooth connect tries per cycle
const int INIT_ATTEMPTS = 3;               // full AT init sequence tries per cycle
const int MAX_CONNECT_CYCLES = 3;          // (Bluetooth + init) cycles before restarting
const int MAX_READ_FAILURES = 5;           // consecutive failed reads before reconnecting
const int MAX_RECOVERIES_WITHOUT_DATA = 3; // reconnects in a row that never got a valid