#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------- Pins --------
#define SDA_PIN       5
#define SCL_PIN       6
#define BTN_PLAY      4   // GPIO1 — play/pause
#define BTN_NEXT      1   // GPIO3 — next track
#define BTN_PREV      3  // GPIO4 — previous track

// -------- OLED --------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------- Fake playlist --------
const char* songs[] = {
  "Bohemian Rhapsody",
  "Blinding Lights",
  "Stairway to Heaven",
  "Hotel California",
  "Shape of You",
  "Smells Like Teen Spirit",
  "Billie Jean",
  "One More Time"
};
const int TOTAL_SONGS = 8;

// -------- State --------
int  currentSong   = 0;
int  menuOffset    = 0;   // which song is at top of visible list
bool isPlaying     = false;
bool inMenu        = true;  // true = song list, false = now playing

uint32_t playStartMs  = 0;  // when current fake play started
uint32_t fakeDuration = 210; // fake song duration in seconds

// fake durations per song
const uint32_t durations[] = {
  354, 200, 482, 391, 234, 301, 294, 320
};

// -------- Button state --------
bool lastPlay = HIGH;
bool lastNext = HIGH;
bool lastPrev = HIGH;

// -------- Button helper --------
bool btnPressed(int pin, bool &lastState) {
  bool current = digitalRead(pin);
  if (lastState == HIGH && current == LOW) {
    delay(10);  // debounce
    if (digitalRead(pin) == LOW) {
      lastState = current;
      return true;
    }
  }
  lastState = current;
  return false;
}


void drawMenu() {
  display.clearDisplay();

  // header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Songs");
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  // show 4 songs at a time
  for (int i = 0; i < 4; i++) {
    int idx = menuOffset + i;
    if (idx >= TOTAL_SONGS) break;

    int y = 17 + i * 13;

    if (idx == currentSong) {
      // highlight selected
      display.fillRect(0, y - 1, 128, 12, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.setCursor(4, y);

    // truncate long names
    char truncated[18];
    strncpy(truncated, songs[idx], 17);
    truncated[17] = '\0';
    display.print(truncated);
  }

  display.setTextColor(SSD1306_WHITE);

  // scroll indicator
  display.setCursor(89, 0);
  display.print(currentSong + 1);
  display.setCursor(96, 0);
  display.print("/");
  display.setCursor(103, 0);
  display.print(TOTAL_SONGS);

  display.display();
}

// -------- OLED: now playing --------
void drawNowPlaying() {
  display.clearDisplay();

  // header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(isPlaying ? "> Now Playing" : "|| Paused");
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  // song name — truncate if needed
  display.setCursor(0, 16);
  char truncated[22];
  strncpy(truncated, songs[currentSong], 21);
  truncated[21] = '\0';
  display.print(truncated);

  // calculate fake progress
  uint32_t elapsed = 0;
  if (isPlaying) {
    elapsed = (millis() - playStartMs) / 1000;
    if (elapsed > durations[currentSong]) {
      elapsed = durations[currentSong];
    }
  }
  uint32_t total = durations[currentSong];

  // progress bar
  int barX = 0;
  int barY = 34;
  int barW = 128;
  int barH = 4;
  display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
  if (total > 0 && isPlaying) {
    float progress = (float)elapsed / (float)total;
    int filled = (int)(progress * barW);
    if (filled > 0) display.fillRect(barX, barY, filled, barH, SSD1306_WHITE);
    display.fillCircle(barX + filled, barY + 2, 3, SSD1306_WHITE);
  }

  // time
  char left[6], right[6];
  sprintf(left,  "%02d:%02d", elapsed / 60, elapsed % 60);
  sprintf(right, "%02d:%02d", total   / 60, total   % 60);
  display.setCursor(0, 42);
  display.print(left);
  display.setCursor(98, 42);
  display.print(right);

  // bottom hint
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print("PLAY=pause NEXT=menu");

  display.display();
}

// -------- Handle auto advance --------
void checkAutoAdvance() {
  if (!isPlaying) return;
  uint32_t elapsed = (millis() - playStartMs) / 1000;
  if (elapsed >= durations[currentSong]) {
    // auto advance to next song
    currentSong = (currentSong + 1) % TOTAL_SONGS;
    playStartMs = millis();
    // keep menuOffset in sync
    if (currentSong < menuOffset || currentSong >= menuOffset + 4) {
      menuOffset = currentSong;
    }
  }
}

// -------- Setup --------
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BTN_PLAY, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found");
    while (true) delay(1000);
  }

  Serial.println("Ready");
  drawMenu();
}

// -------- Loop --------
void loop() {

  // ---- PLAY button ----
  if (btnPressed(BTN_PLAY, lastPlay)) {
    if (inMenu) {
      // select song and go to now playing
      inMenu      = false;
      isPlaying   = true;
      playStartMs = millis();
      Serial.printf("Playing: %s\n", songs[currentSong]);
    } else {
      // toggle play/pause
      if (isPlaying) {
        isPlaying = false;
        Serial.println("Paused");
      } else {
        isPlaying   = true;
        playStartMs = millis();  // resume from beginning for simulation
        Serial.println("Resumed");
      }
    }
  }

  // ---- NEXT button ----
  if (btnPressed(BTN_NEXT, lastNext)) {
    if (inMenu) {
      // navigate down in menu
      if (currentSong < TOTAL_SONGS - 1) {
        currentSong++;
        if (currentSong >= menuOffset +4) menuOffset++;
      }
      Serial.printf("Menu select: %s\n", songs[currentSong]);
    } else {
      // next track
      if (currentSong < TOTAL_SONGS - 1){
        currentSong++;
        playStartMs = millis();
        isPlaying   = true;
        if (currentSong >= menuOffset + 4) menuOffset++;
        
      }
      Serial.printf("Next: %s\n", songs[currentSong]);
    }
  }

  // ---- PREV button ----
  if (btnPressed(BTN_PREV, lastPrev)) {
    if (inMenu) {
      // navigate up in menu
      if (currentSong > 0){
        currentSong--;
        if (currentSong < menuOffset) menuOffset--;
      }
      Serial.printf("Menu select: %s\n", songs[currentSong]);
    } else {
      // previous track or go back to menu
      if (inMenu == false) {
        inMenu    = true;
        isPlaying = false;
        Serial.println("Back to menu");
      }
    }
  }

  // auto advance when song finishes
  checkAutoAdvance();

  // refresh display
  if (inMenu) {
    drawMenu();
  } else {
    drawNowPlaying();
  }

  delay(50);  // refresh rate
}