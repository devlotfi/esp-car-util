#include <Arduino.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecSBC.h"
#include "AudioTools/AudioCodecs/CodecAACHelix.h"
#include "BluetoothA2DPSink.h"
#include "A2DPDecoderSBC.h"
#include "A2DPDecoderAAC.h"
#include "Properties.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

SBCDecoder sbc_decoder;
A2DPDecoderSBC a2dp_sbc(sbc_decoder);
AACDecoderHelix aac_decoder;
A2DPDecoderAAC a2dp_aac(aac_decoder);

void setup()
{
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = I2S_BCK_PIN;
  cfg.pin_ws = I2S_LRCK_PIN;
  cfg.pin_data = I2S_DIN_PIN;
  i2s.begin(cfg);

  a2dp_sink.add_decoder(a2dp_aac);
  a2dp_sink.add_decoder(a2dp_sbc);
  a2dp_sink.set_auto_reconnect(true, 1000);
  a2dp_sink.start(BT_NAME);
  a2dp_sink.reconnect();
}

void loop()
{
  a2dp_sink.delay_ms(1000);
}
