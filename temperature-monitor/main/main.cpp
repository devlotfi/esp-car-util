#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup()
{
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = 25;
  cfg.pin_ws = 26;
  cfg.pin_data = 27;
  i2s.begin(cfg);
  a2dp_sink.start("esp-car-util");
}

void loop()
{
  a2dp_sink.delay_ms(500); // or use vTaskDelay()
}
