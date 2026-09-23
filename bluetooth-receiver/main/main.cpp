#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include "Properties.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup()
{
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = I2S_BCK_PIN;
  cfg.pin_ws = I2S_LRCK_PIN;
  cfg.pin_data = I2S_DIN_PIN;
  i2s.begin(cfg);
  a2dp_sink.start("esp-car-util");
}

void loop()
{
  a2dp_sink.delay_ms(500);
}
