#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define BTN_ROT  4
#define BTN_R    3
#define BTN_L    1
#define SDA_PIN  5
#define SCL_PIN  6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum AppState { MENU, TETRIS, SNAKE, BLASTER, PACMAN, FLAPPY, DINO, BREAKOUT, MARIO };
AppState appState = MENU;

// --- Shared exit system ---
int exitPressCount = 0;
bool lastLeftState = HIGH;
uint32_t exitLastPressTime = 0;

// Include games
#include "tetris.h"
#include "snake.h"
#include "blaster.h"
#include "pacman.h"
#include "flappy.h"
#include "dino.h"
#include "breakout.h"
#include "mario.h"

// --- Menu ---
const char* gameNames[] = { "TETRIS", "SNAKE", "BLASTER", "PACMAN", "FLAPPY", "DINO", "BREAKOUT", "MARIO" };
const int NUM_GAMES = 8;

int menuSel = 0;
uint32_t lastMenuInput = 0;

// Smooth scrolling
float smoothOffset = 0;
int targetOffset = 0;
const int VISIBLE_ITEMS = 3;

// --- DRAW MENU ---
void drawMenu() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Title
  display.setTextSize(1);
  display.setCursor(20, 2);
  display.print(F("KAMRAN GAMES"));
  display.drawFastHLine(0, 11, 128, WHITE);

  // Smooth animation
  smoothOffset += (targetOffset - smoothOffset) * 0.2;

  // Draw items
  for (int i = 0; i < NUM_GAMES; i++) {
    float yFloat = 16 + (i - smoothOffset) * 14;
    int y = (int)yFloat;

    if (y < -12 || y > 64) continue;

    if (i == menuSel) {
      display.fillRect(0, y, 128, 12, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.setTextColor(WHITE);
    }

    display.setCursor(8, y + 1);
    display.print(gameNames[i]);
  }

  display.display();
}

// --- INPUT ---
void menuInput() {
  if (millis() - lastMenuInput < 200) return;

  if (digitalRead(BTN_R) == LOW) {
    menuSel = (menuSel + NUM_GAMES - 1) % NUM_GAMES;
    lastMenuInput = millis();
  }

  if (digitalRead(BTN_L) == LOW) {
    menuSel = (menuSel + 1) % NUM_GAMES;
    lastMenuInput = millis();
  }

  // Scroll follow
  if (menuSel < targetOffset) {
    targetOffset = menuSel;
  }
  if (menuSel >= targetOffset + VISIBLE_ITEMS) {
    targetOffset = menuSel - VISIBLE_ITEMS + 1;
  }

  // Select
  if (digitalRead(BTN_ROT) == LOW) {
    lastMenuInput = millis();
    if      (menuSel == 0) { tetrisStart();  appState = TETRIS; }
    else if (menuSel == 1) { snakeStart();   appState = SNAKE; }
    else if (menuSel == 2) { blasterStart(); appState = BLASTER; }
    else if (menuSel == 3) { pacmanStart();  appState = PACMAN; }
    else if (menuSel == 4) { flappyStart();  appState = FLAPPY; }
    else if (menuSel == 5) { dinoStart();    appState = DINO; }
    else if (menuSel == 6) { breakoutStart(); appState = BREAKOUT; }
    else if (menuSel == 7) { marioStart(); appState = MARIO; }

  }
}

// --- SETUP ---
void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  pinMode(BTN_ROT, INPUT_PULLUP);
  pinMode(BTN_R,   INPUT_PULLUP);
  pinMode(BTN_L,   INPUT_PULLUP);

  randomSeed(analogRead(0));
}

// --- LOOP ---
void loop() {
  switch (appState) {
    case MENU:    menuInput(); drawMenu(); break;
    case TETRIS:  tetrisLoop(); break;
    case SNAKE:   snakeLoop(); break;
    case BLASTER: blasterLoop(); break;
    case PACMAN:  pacmanLoop(); break;
    case FLAPPY:  flappyLoop(); break;
    case DINO:    dinoLoop(); break;
    case BREAKOUT: breakoutLoop(); break;
    case MARIO: marioLoop(); break;

  }
}

