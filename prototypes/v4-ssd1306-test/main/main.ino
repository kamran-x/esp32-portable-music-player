#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C  
#define SDA_PIN 5
#define SCL_PIN 6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 not found problem with I2C address");
    while (true) delay(1000);
  }

  Serial.println("OLED OK");

  //clear screen
  display.clearDisplay();

  // text
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("♪ Now Playing");
  display.setCursor(0, 6);
  display.println("-------------");
  display.println("song_name.wav");
  display.println("-------------");
  display.drawRect(0, 45, 128, 8, SSD1306_WHITE);
  display.fillRect(0, 45, 64, 8, SSD1306_WHITE);
  display.display();  // should be always called 
}

void loop() {}
