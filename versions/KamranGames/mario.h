#pragma once

extern Adafruit_SSD1306 display;
extern AppState appState;
extern int menuSel;

// ===== BUTTONS =====
#define BTN_ROT  4
#define BTN_R    3
#define BTN_L    1

// ===== CONSTANTS =====
#define TILE 8
#define LVL_W 64
#define LVL_H 8

#define PLAYER_W 6
#define PLAYER_H 6

// ===== PHYSICS (Mario-like) =====
#define MAX_RUN 3
#define RUN_ACCEL 1
#define AIR_ACCEL 1
#define FRICTION 1

#define GRAVITY 1
#define MAX_FALL 6

#define JUMP_VEL -6
#define JUMP_HOLD -1
#define JUMP_TIME 6

// ===== PLAYER =====
int px, py;
int vx, vy;
bool onGround;

int jumpTimer = 0;

// ===== CAMERA =====
int camX;

// ===== GAME STATE =====
bool marioGameOver = false;
bool marioGameWin = false;

// ===== TIMING =====
uint32_t lastFrame = 0;

// ===== LEVEL =====
#define EMPTY_ROW { \
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }

#define GROUND_ROW { \
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }

const uint8_t level[LVL_H][LVL_W] PROGMEM = {
  EMPTY_ROW,
  EMPTY_ROW,
  EMPTY_ROW,
  EMPTY_ROW,
  EMPTY_ROW,
  EMPTY_ROW,
  EMPTY_ROW,
  GROUND_ROW
};

// ===== ENEMY =====
#define ENEMY_W 6
#define ENEMY_H 6

int ex, ey, evx;

// ===== GOAL =====
int goalX;

// ===== TILE CHECK =====
bool solidAt(int tx, int ty) {
  if (tx < 0 || tx >= LVL_W || ty < 0 || ty >= LVL_H) return false;
  return pgm_read_byte(&level[ty][tx]) == 1;
}

// ===== START =====
void marioStart() {
  px = 10;
  py = ((LVL_H - 1) * TILE) - PLAYER_H;

  vx = 0;
  vy = 0;
  onGround = true;

  camX = 0;

  marioGameOver = false;
  marioGameWin = false;

  ex = 80;
  ey = ((LVL_H - 1) * TILE) - ENEMY_H;
  evx = -1;

  goalX = (LVL_W * TILE) - 20;
}

// ===== MOVE + COLLISION =====
void movePlayer() {

  // --- horizontal ---
  px += vx;

  int left = px / TILE;
  int right = (px + PLAYER_W - 1) / TILE;
  int top = py / TILE;
  int bottom = (py + PLAYER_H - 1) / TILE;

  if (vx > 0 && (solidAt(right, top) || solidAt(right, bottom))) {
    px = right * TILE - PLAYER_W;
    vx = 0;
  }

  if (vx < 0 && (solidAt(left, top) || solidAt(left, bottom))) {
    px = (left + 1) * TILE;
    vx = 0;
  }

  // --- vertical ---
  py += vy;
  onGround = false;

  left = px / TILE;
  right = (px + PLAYER_W - 1) / TILE;
  top = py / TILE;
  bottom = (py + PLAYER_H - 1) / TILE;

  // falling
  if (vy > 0 && (solidAt(left, bottom) || solidAt(right, bottom))) {
    py = bottom * TILE - PLAYER_H;
    vy = 0;
    onGround = true;
  }

  // jumping
  if (vy < 0 && (solidAt(left, top) || solidAt(right, top))) {
    py = (top + 1) * TILE;
    vy = 0;
  }

  // clamp to level
  if (px < 0) px = 0;
  if (px > LVL_W * TILE - PLAYER_W)
    px = LVL_W * TILE - PLAYER_W;
}

// ===== LOOP =====
void marioLoop() {

  // EXIT
  if (!digitalRead(BTN_L) && !digitalRead(BTN_R)) {
    appState = MENU;
    menuSel = 0;
    return;
  }

  if (millis() - lastFrame < 16) return;
  lastFrame = millis();

  // END SCREEN
  if (marioGameOver || marioGameWin) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.print(marioGameWin ? "WIN" : "OVER");
    display.display();
    return;
  }

  // INPUT
  bool left = !digitalRead(BTN_L);
  bool right = !digitalRead(BTN_R);
  bool jump = !digitalRead(BTN_ROT);

  // --- horizontal accel ---
  if (left) vx -= (onGround ? RUN_ACCEL : AIR_ACCEL);
  if (right) vx += (onGround ? RUN_ACCEL : AIR_ACCEL);

  if (vx > MAX_RUN) vx = MAX_RUN;
  if (vx < -MAX_RUN) vx = -MAX_RUN;

  // friction
  if (!left && !right && onGround) {
    if (vx > 0) vx -= FRICTION;
    else if (vx < 0) vx += FRICTION;
  }

  // jump start
  if (jump && onGround) {
    vy = JUMP_VEL;
    jumpTimer = JUMP_TIME;
  }

  // variable jump height
  if (jump && jumpTimer > 0) {
    vy += JUMP_HOLD;
    jumpTimer--;
  } else {
    jumpTimer = 0;
  }

  // gravity
  vy += GRAVITY;
  if (vy > MAX_FALL) vy = MAX_FALL;

  movePlayer();

  // ===== ENEMY =====
  ex += evx;

  int aheadX = (ex + (evx > 0 ? ENEMY_W : -1)) / TILE;
  int belowY = (ey + ENEMY_H) / TILE;

  if (!solidAt(aheadX, belowY)) evx *= -1;

  // collision
  if (abs(px - ex) < 6 && abs(py - ey) < 6) {
    if (vy > 0) {
      vy = -4;
      ex = -100;
    } else {
      marioGameOver = true;
    }
  }

  // ===== CAMERA (smooth) =====
  int targetCam = px - 64;
  if (targetCam < 0) targetCam = 0;
  camX += (targetCam - camX) / 4;

  // ===== WIN =====
  if (px + PLAYER_W > goalX) marioGameWin = true;

  // ===== DRAW =====
  display.clearDisplay();

  int first = camX / TILE;
  int last = (camX + 128) / TILE + 1;

  for (int y = 0; y < LVL_H; y++) {
    for (int x = first; x < last; x++) {
      if (solidAt(x, y)) {
        display.fillRect(x * TILE - camX, y * TILE, TILE, TILE, WHITE);
      }
    }
  }

  // enemy
  display.fillRect(ex - camX, ey, ENEMY_W, ENEMY_H, WHITE);

  // player
  display.fillRect(px - camX, py, PLAYER_W, PLAYER_H, WHITE);

  // goal
  display.drawRect(goalX - camX, (LVL_H - 2) * TILE, 6, 12, WHITE);

  display.display();
}