#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "driver/i2s.h"

//Pins 
#define SDA_PIN         5
#define SCL_PIN         6
#define SD_CS_PIN       21
#define I2S_PORT        I2S_NUM_0
#define I2S_BCLK        7
#define I2S_WS          8
#define I2S_DOUT        9
#define I2S_MCLK        1

#define WM8960_ADDR     0x1A
#define I2S_BUFFER_SIZE 4096

static bool i2s_installed = false;

//WAV info 

struct WavInfo {
  uint16_t numChannels;
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint32_t dataOffset;
  uint32_t dataSize;
};

//wm8960
void wm8960_write(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(WM8960_ADDR);
  Wire.write((reg << 1) | ((val >> 8) & 0x01));
  Wire.write(val & 0xFF);
  Wire.endTransmission();
}

void wm8960_init() {
  delay(100);
  wm8960_write(0x0F, 0x000);  // Reset
  delay(10);

  // Exact same init as original working code
  wm8960_write(0x19, 0x1F8);
  wm8960_write(0x1A, 0x1FF);
  wm8960_write(0x2F, 0x03C);
  wm8960_write(0x04, 0x000);
  wm8960_write(0x07, 0x002);  // I2S 16-bit
  wm8960_write(0x05, 0x000);
  wm8960_write(0x0A, 0x1FF);  // Left DAC volume
  wm8960_write(0x0B, 0x1FF);  // Right DAC volume
  wm8960_write(0x22, 0x180);  // Left output mixer
  wm8960_write(0x25, 0x180);  // Right output mixer
  wm8960_write(0x02, 0x17F);  // Left headphone volume
  wm8960_write(0x03, 0x17F);  // Right headphone volume
}

void wm8960_set_word_length(uint16_t bits) {
  uint16_t wl;
  switch (bits) {
    case 16: wl = 0x00; break;
    case 20: wl = 0x04; break;
    case 24: wl = 0x08; break;
    case 32: wl = 0x0C; break;
    default:  wl = 0x00; break;
  }
  wm8960_write(0x07, 0x002 | wl);
}

//I2S
void setupI2S(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
  if (i2s_installed) {
    i2s_driver_uninstall(I2S_PORT);
    i2s_installed = false;
  }

  i2s_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate          = sampleRate;
  cfg.bits_per_sample      = (i2s_bits_per_sample_t)bitsPerSample;
  cfg.channel_format       = (numChannels == 1)
                               ? I2S_CHANNEL_FMT_ONLY_LEFT
                               : I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
  cfg.dma_buf_count        = 16;
  cfg.dma_buf_len          = 1024;
  cfg.use_apll             = true;
  cfg.tx_desc_auto_clear   = true;
  cfg.fixed_mclk           = sampleRate * 256;

  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_installed = true;

  i2s_pin_config_t pins = {};
  pins.bck_io_num    = I2S_BCLK;
  pins.ws_io_num     = I2S_WS;
  pins.data_out_num  = I2S_DOUT;
  pins.data_in_num   = I2S_PIN_NO_CHANGE;
  pins.mck_io_num    = I2S_MCLK;

  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

//parse WAV chunks 
bool parseWav(File &f, WavInfo &info) {
  uint8_t id[4];
  f.seek(0);

  f.read(id, 4);
  if (strncmp((char*)id, "RIFF", 4)) { Serial.println("Not RIFF"); return false; }
  uint32_t fileSize;
  f.read((uint8_t*)&fileSize, 4);
  f.read(id, 4);
  if (strncmp((char*)id, "WAVE", 4)) { Serial.println("Not WAVE"); return false; }

  bool gotFmt = false, gotData = false;

  while (f.available() && !(gotFmt && gotData)) {
    f.read(id, 4);
    uint32_t chunkSize;
    f.read((uint8_t*)&chunkSize, 4);
    uint32_t chunkStart = f.position();

    if (strncmp((char*)id, "fmt ", 4) == 0) {
      uint16_t audioFormat;
      f.read((uint8_t*)&audioFormat,        2);
      f.read((uint8_t*)&info.numChannels,   2);
      f.read((uint8_t*)&info.sampleRate,    4);
      uint32_t byteRate;
      f.read((uint8_t*)&byteRate,           4);
      uint16_t blockAlign;
      f.read((uint8_t*)&blockAlign,         2);
      f.read((uint8_t*)&info.bitsPerSample, 2);
      if (audioFormat != 1) { Serial.println("Not PCM"); return false; }
      gotFmt = true;
      f.seek(chunkStart + chunkSize);

    } else if (strncmp((char*)id, "data", 4) == 0) {
      info.dataSize   = chunkSize;
      info.dataOffset = f.position();
      gotData = true;

    } else {
      Serial.printf("Skipping chunk: %.4s (%u bytes)\n", id, chunkSize);
      f.seek(chunkStart + chunkSize);
    }
  }

  return gotFmt && gotData;
}

// playing wav form sd via psram 
bool playWavFile(const char* path) {

  Serial.println("Opening SD...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD mount failed");
    return false;
  }

  File f = SD.open(path);
  if (!f) {
    Serial.printf("Cannot open: %s\n", path);
    SD.end();
    return false;
  }

  WavInfo info;
  if (!parseWav(f, info)) {
    f.close();
    SD.end();
    return false;
  }

  Serial.printf("WAV: %u Hz  %u-bit  %s  %u bytes\n",
    info.sampleRate, info.bitsPerSample,
    info.numChannels == 1 ? "mono" : "stereo",
    info.dataSize);

  if (info.dataSize > ESP.getFreePsram()) {
    Serial.printf("File too large! Need %u, free %u\n",
      info.dataSize, ESP.getFreePsram());
    f.close();
    SD.end();
    return false;
  }

  uint8_t* audioBuf = (uint8_t*)ps_malloc(info.dataSize);
  if (!audioBuf) {
    Serial.println("PSRAM alloc failed");
    f.close();
    SD.end();
    return false;
  }

  f.seek(info.dataOffset);
  uint32_t totalRead = 0;
  while (totalRead < info.dataSize && f.available()) {
    size_t toRead = min((size_t)(info.dataSize - totalRead), (size_t)I2S_BUFFER_SIZE);
    size_t got = f.read(audioBuf + totalRead, toRead);
    if (got == 0) break;
    totalRead += got;
  }
  f.close();
  SD.end();
  Serial.printf("Loaded %u bytes. Playing...\n", totalRead);

  wm8960_set_word_length(info.bitsPerSample);
  setupI2S(info.sampleRate, info.bitsPerSample, info.numChannels);
  delay(100);

  size_t bytesWritten;
  uint32_t remaining = totalRead;
  uint8_t* ptr = audioBuf;

  while (remaining > 0) {
    size_t toWrite = min((size_t)remaining, (size_t)I2S_BUFFER_SIZE);
    i2s_write(I2S_PORT, ptr, toWrite, &bytesWritten, portMAX_DELAY);
    ptr      += bytesWritten;
    remaining -= bytesWritten;
  }

  i2s_zero_dma_buffer(I2S_PORT);

  if (i2s_installed) {
    i2s_driver_uninstall(I2S_PORT);
    i2s_installed = false;
  }

  free(audioBuf);
  return true;
}

//Playlist helper
int collectWavFiles(const char* dir, String* paths, int maxFiles) {
  File root = SD.open(dir);
  if (!root || !root.isDirectory()) return 0;

  int count = 0;
  File entry = root.openNextFile();
  while (entry && count < maxFiles) {
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      if (name.endsWith(".wav") || name.endsWith(".WAV")) {
        paths[count++] = String(dir) + name;
      }
    }
    entry = root.openNextFile();
  }
  return count;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!psramFound()) {
    Serial.println("ERROR: PSRAM not found! Check Tools > PSRAM > OPI PSRAM");
    while(1) delay(1000);
  }
  Serial.printf("PSRAM size: %u bytes\n", ESP.getPsramSize());
  Serial.printf("PSRAM free: %u bytes\n", ESP.getFreePsram());

  Wire.begin(SDA_PIN, SCL_PIN);
  wm8960_init();
  Serial.println("Ready.");
}

void loop() {
  playWavFile("/test_48k.wav");
  delay(500);
}
