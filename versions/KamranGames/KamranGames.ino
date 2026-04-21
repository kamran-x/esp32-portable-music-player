#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define BTN_ROT  4
#define BTN_R    3
#define BTN_L    1
#define SDA_PIN  5
#define SCL_PIN  6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum AppState { MENU, TETRIS, SNAKE, BLASTER };
AppState appState = MENU;

// EEPROM addresses for hi-scores
#define EE_TETRIS_HI  0
#define EE_SNAKE_HI   2
#define EE_BLASTER_HI 4

// Include games after display/pins are declared
#include "tetris.h"
#include "snake.h"
#include "blaster.h"

// --- Menu ---
const char* gameNames[] = { "TETRIS", "SNAKE", "BLASTER" };
const int NUM_GAMES = 3;
int menuSel = 0;
uint32_t lastMenuInput = 0;

// Back-button: hold ROT for 2 seconds in-game → return to menu
uint32_t rotHeldSince = 0;
bool rotWasHeld = false;



void drawMenu() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Title
  display.setTextSize(1);
  display.setCursor(20, 2);
  display.print(F("KAMRAN GAMES"));
  display.drawFastHLine(0, 11, 128, WHITE);

  // Game list
  for (int i = 0; i < NUM_GAMES; i++) {
    int y = 16 + i * 14;
    if (i == menuSel) {
      display.fillRect(0, y, 128, 12, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(8, y+1);
    display.print(gameNames[i]);

    // Show hi-score next to name
    display.setCursor(70, y);
    uint16_t hi = 0;
    if (i == 0) EEPROM.get(EE_TETRIS_HI,  hi);
    if (i == 1) EEPROM.get(EE_SNAKE_HI,   hi);
    if (i == 2) EEPROM.get(EE_BLASTER_HI, hi);
  }
  display.display();
}

void menuInput() {
  if (millis() - lastMenuInput < 200) return;
  if (digitalRead(BTN_R) == LOW) { menuSel = (menuSel + NUM_GAMES - 1) % NUM_GAMES; lastMenuInput = millis(); }
  if (digitalRead(BTN_L) == LOW) { menuSel = (menuSel + 1) % NUM_GAMES;              lastMenuInput = millis(); }
  if (digitalRead(BTN_ROT) == LOW) {
    lastMenuInput = millis();
    if      (menuSel == 0) { tetrisStart();  appState = TETRIS;  }
    else if (menuSel == 1) { snakeStart();   appState = SNAKE;   }
    else if (menuSel == 2) { blasterStart(); appState = BLASTER; }
  }
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  pinMode(BTN_ROT, INPUT_PULLUP);
  pinMode(BTN_R,   INPUT_PULLUP);
  pinMode(BTN_L,   INPUT_PULLUP);
  randomSeed(analogRead(0));
}

void loop() {
  switch (appState) {
    case MENU:    menuInput(); drawMenu();    break;
    case TETRIS:  tetrisLoop();              break;
    case SNAKE:   snakeLoop();               break;
    case BLASTER: blasterLoop();             break;
  }
}
