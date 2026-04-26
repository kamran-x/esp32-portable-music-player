#pragma once

extern AppState appState;
extern int menuSel;

// -- Constants --
const int DR_W         = 128;
const int DR_H         = 64;
const int DR_GROUND    = 54;       // y of ground line
const int DR_DINO_X    = 14;
const int DR_DINO_W    = 8;
const int DR_DINO_H    = 14;       // standing height
const int DR_DINO_DH   = 6;        // ducking height
const int DR_GRAVITY   = 5;        // sub-pixel ×4
const int DR_JUMP_V    = -39;      // sub-pixel ×4
const int DR_NOBS      = 4;        // max obstacles
const int DR_NBIRDS    = 2;        // max flying obstacles

struct DRCactus { int x, w, h; bool alive; };
struct DRBird   { int x, y;    bool alive; };

int      drDinoY4;    // dino Y top × 4 (sub-pixel)
int      drDinoV4;    // velocity × 4
bool     drDucking;
bool     drOnGround;
int      drScore;
int      drBestScore;
bool     drGameOver;
bool     drStarted;
DRCactus drObs[DR_NOBS];
DRBird   drBirds[DR_NBIRDS];
uint32_t drLastFrame;
int      drSpeed4;    // pixels per frame × 4
uint8_t  drAnimTick;
uint8_t  drSpawnTimer;

// -- Helpers --
int drDinoTop()    { return drDinoY4 / 4; }
int drDinoBottom() { return drDinoTop() + (drDucking ? DR_DINO_DH : DR_DINO_H); }

void drSpawnObs() {
  // Find a free cactus slot
  for (auto &o : drObs) {
    if (!o.alive) {
      o.x = DR_W + 4;
      o.w = 5 + random(4);        // width 5-8
      o.h = 10 + random(10);      // height 10-19
      o.alive = true;
      return;
    }
  }
}

void drSpawnBird() {
  for (auto &b : drBirds) {
    if (!b.alive) {
      b.x = DR_W + 4;
      // Fly at 3 possible heights: high, mid, low
      int heights[3] = { DR_GROUND - DR_DINO_H - 8,
                         DR_GROUND - DR_DINO_H + 2,
                         DR_GROUND - DR_DINO_DH - 2 };
      b.y = heights[random(3)];
      b.alive = true;
      return;
    }
  }
}

// -- Init --
void dinoStart() {
  drDinoY4    = (DR_GROUND - DR_DINO_H) * 4;
  drDinoV4    = 0;
  drDucking   = false;
  drOnGround  = true;
  drScore     = 0;
  drGameOver  = false;
  drStarted   = false;
  drSpeed4    = 8;    // 2 px/frame to start
  drAnimTick  = 0;
  drSpawnTimer = 30;
  for (auto &o : drObs)   o.alive = false;
  for (auto &b : drBirds) b.alive = false;
  drLastFrame = millis();
}

// -- Draw --
void drDrawDino() {
  int x = DR_DINO_X;
  int y = drDinoTop();
  bool leg = (drAnimTick / 4) % 2;

  if (drDucking) {
    // Ducking body (wide, short)
    display.fillRect(x,   y,     DR_DINO_W,     DR_DINO_DH,     WHITE);
    display.fillRect(x+1, y - 1, DR_DINO_W - 2, DR_DINO_DH + 1, WHITE);
    // Eye
    display.drawPixel(x + DR_DINO_W - 2, y + 1, BLACK);
    // Tail
    display.drawPixel(x - 1, y + 2, WHITE);
    display.drawPixel(x - 2, y + 3, WHITE);
    // Legs
    display.drawFastVLine(x + 2, y + DR_DINO_DH, leg ? 3 : 2, WHITE);
    display.drawFastVLine(x + 6, y + DR_DINO_DH, leg ? 2 : 3, WHITE);
  } else {
    // Head
    display.fillRect(x + 3, y,     DR_DINO_W - 3, 6, WHITE);
    display.fillRect(x + 2, y + 1, DR_DINO_W - 2, 5, WHITE);
    // Eye
    display.drawPixel(x + DR_DINO_W - 2, y + 1, BLACK);
    // Mouth / jaw
    display.drawPixel(x + DR_DINO_W - 1, y + 4, WHITE);
    // Body
    display.fillRect(x,     y + 5, DR_DINO_W - 1, 7, WHITE);
    display.fillRect(x + 1, y + 4, DR_DINO_W - 2, 8, WHITE);
    // Tail
    display.drawPixel(x - 1, y + 6, WHITE);
    display.drawPixel(x - 2, y + 7, WHITE);
    display.drawPixel(x - 1, y + 8, WHITE);
    // Legs
    display.drawFastVLine(x + 3, y + 12, leg ? 4 : 2, WHITE);
    display.drawFastVLine(x + 7, y + 12, leg ? 2 : 4, WHITE);
  }
}

void drDraw() {
  display.clearDisplay();

  // Ground
  display.drawFastHLine(0, DR_GROUND, DR_W, WHITE);
  // Ground texture dots
  for (int gx = (drScore * 2) % 16; gx < DR_W; gx += 16)
    display.drawPixel(gx, DR_GROUND + 1, WHITE);

  // Cacti
  for (auto &o : drObs) {
    if (!o.alive) continue;
    int oy = DR_GROUND - o.h;
    display.fillRect(o.x, oy, o.w, o.h, WHITE);
    // Arms
    display.fillRect(o.x - 2, oy + 3, 2, 4, WHITE);
    display.fillRect(o.x + o.w, oy + 5, 2, 4, WHITE);
  }

  // Birds (simple W shape)
  for (auto &b : drBirds) {
    if (!b.alive) continue;
    bool flap = (drAnimTick / 3) % 2;
    if (flap) {
      display.drawFastHLine(b.x,     b.y,     5, WHITE);
      display.drawFastHLine(b.x + 5, b.y,     5, WHITE);
    } else {
      display.drawFastHLine(b.x,     b.y + 2, 5, WHITE);
      display.drawFastHLine(b.x + 5, b.y + 2, 5, WHITE);
      display.drawPixel(b.x + 2, b.y,     WHITE);
      display.drawPixel(b.x + 7, b.y,     WHITE);
    }
  }

  drDrawDino();

  // Score
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(80, 2);
  display.print(F("HI:"));
  display.print(drBestScore);
  display.setCursor(80, 12);
  display.print(drScore);

  if (!drStarted) {
    display.setCursor(14, 28);
    display.print(F("ROT=jump  L=duck"));
  }

  display.display();
}

// -- Loop --
void dinoLoop() {
  if (drGameOver) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(16, 4); display.print(F("GAME OVER"));
    display.setTextSize(1);
    display.setCursor(4, 26); display.print(F("Score: ")); display.print(drScore);
    display.setCursor(4, 36); display.print(F("Best:  ")); display.print(drBestScore);
    display.setCursor(4, 46); display.print(F("R-btn > Play again"));
    display.setCursor(4, 56); display.print(F("C-btn > Main menu"));
    display.display();
    delay(400);
    if (digitalRead(BTN_R)   == LOW) { dinoStart(); delay(300); }
    if (digitalRead(BTN_ROT) == LOW) { appState = MENU; menuSel = 0; delay(300); }
    return;
  }

  uint32_t now = millis();
  if (now - drLastFrame < 40) return;   // 25 fps
  drLastFrame = now;

  drAnimTick++;

  // -- Input --
  bool rotNow = digitalRead(BTN_ROT) == LOW;
  bool lNow   = digitalRead(BTN_L)   == LOW;

  static bool drRotLast = false;
  if (rotNow && !drRotLast && drOnGround) {
    drDinoV4  = DR_JUMP_V;
    drOnGround = false;
    drStarted  = true;
    drDucking  = false;
  }
  drRotLast = rotNow;

  if (lNow && drOnGround) {
    drDucking = true;
    drStarted = true;
  } else if (!lNow) {
    drDucking = false;
  }

  if (!drStarted) { drDraw(); return; }

  // -- Physics --
  if (!drOnGround) {
    drDinoV4 += DR_GRAVITY;
    drDinoY4 += drDinoV4;
    int groundY4 = (DR_GROUND - DR_DINO_H) * 4;
    if (drDinoY4 >= groundY4) {
      drDinoY4   = groundY4;
      drDinoV4   = 0;
      drOnGround = true;
    }
  }

  // -- Score & speed --
  drScore++;
  if (drScore % 100 == 0 && drSpeed4 < 24) drSpeed4++;

  // -- Spawn obstacles --
  if (--drSpawnTimer == 0) {
    drSpawnTimer = 25 + random(35);
    if (random(3) == 0 && drScore > 200)
      drSpawnBird();
    else
      drSpawnObs();
  }

  // -- Move obstacles --
  int dx = drSpeed4 / 4;
  if (dx < 1) dx = 1;
  for (auto &o : drObs)   { if (o.alive) { o.x -= dx; if (o.x + o.w < 0) o.alive = false; } }
  for (auto &b : drBirds) { if (b.alive) { b.x -= dx; if (b.x + 10 < 0)  b.alive = false; } }

  // -- Collision --
  int dx1 = DR_DINO_X + 2;
  int dx2 = DR_DINO_X + DR_DINO_W - 2;
  int dy1 = drDinoTop() + 2;
  int dy2 = drDinoBottom() - 1;

  for (auto &o : drObs) {
    if (!o.alive) continue;
    int oy = DR_GROUND - o.h;
    if (dx2 > o.x + 1 && dx1 < o.x + o.w - 1 && dy2 > oy + 1) {
      drGameOver = true;
      if (drScore > drBestScore) drBestScore = drScore;
      return;
    }
  }

  for (auto &b : drBirds) {
    if (!b.alive) continue;
    if (dx2 > b.x + 1 && dx1 < b.x + 8 && dy1 < b.y + 4 && dy2 > b.y) {
      drGameOver = true;
      if (drScore > drBestScore) drBestScore = drScore;
      return;
    }
  }

  drDraw();
}
