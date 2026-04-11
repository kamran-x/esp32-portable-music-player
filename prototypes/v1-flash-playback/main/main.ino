#include <Arduino.h>
#include <Wire.h>
#include "driver/i2s.h"

#include "audio.h"   

//I2C
#define SDA_PIN 5
#define SCL_PIN 6
#define WM8960_ADDR 0x1A

#define I2S_PORT I2S_NUM_0
#define I2S_BCLK 7
#define I2S_WS   8
#define I2S_DOUT 9
#define I2S_MCLK 1   // GPIO1 (D0)

#define SAMPLE_RATE 48000

//wm8960 write
void wm8960_write(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(WM8960_ADDR);
  Wire.write((reg << 1) | ((val >> 8) & 0x01));
  Wire.write(val & 0xFF);
  Wire.endTransmission();
}

//wm8960 init
void wm8960_init() {
  delay(100);

  wm8960_write(0x0F, 0x000);  // reset

  wm8960_write(0x19, 0x1F8);
  wm8960_write(0x1A, 0x1FF);
  wm8960_write(0x2F, 0x03C);

  wm8960_write(0x04, 0x000);
  wm8960_write(0x07, 0x0002); // I2S 16-bit

  wm8960_write(0x05, 0x000);

  wm8960_write(0x0A, 0x1FF);
  wm8960_write(0x0B, 0x1FF);

  wm8960_write(0x22, 0x180);
  wm8960_write(0x25, 0x180);

  wm8960_write(0x02, 0x17F);
  wm8960_write(0x03, 0x17F);
}

//setting up I2S
void setupI2S() {
  i2s_config_t config;
  memset(&config, 0, sizeof(config));

  config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = SAMPLE_RATE;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  config.communication_format = I2S_COMM_FORMAT_I2S;
  config.dma_buf_count = 8;
  config.dma_buf_len = 512;
  config.use_apll = true;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = SAMPLE_RATE * 256;

  i2s_driver_install(I2S_PORT, &config, 0, NULL);

  i2s_pin_config_t pins;
  pins.bck_io_num = I2S_BCLK;
  pins.ws_io_num = I2S_WS;
  pins.data_out_num = I2S_DOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  pins.mck_io_num = I2S_MCLK;

  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

//playing wav 
void playWav() {
  size_t bytes_written;

  const uint8_t* audio_ptr = rickroll_wav + 44;
  size_t audio_len = rickroll_wav_len - 44;

  i2s_write(I2S_PORT, audio_ptr, audio_len, &bytes_written, portMAX_DELAY);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  wm8960_init();
  setupI2S();

  Serial.println("Playing sound...");
  playWav();
}

void loop() {}
