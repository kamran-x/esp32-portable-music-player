#pragma once

extern AppState appState;
extern int menuSel;

// -- Constants --
const int FB_W        = 128;
const int FB_H        = 64;
const int FB_BIRD_X   = 26;
const int FB_BIRD_W   = 5;
const int FB_BIRD_H   = 3;
const int FB_GAP      = 35;       // gap between pipes
const int FB_PIPE_W   = 11;
const int FB_GRAVITY  = 1;        // added to vel each frame (scaled ×4)
const int FB_FLAP     = -6;       // velocity on flap (scaled ×4)
const int FB_GROUND   = 58;       // y of ground line
const int FB_NPIIPES  = 3;        // number of pipes in flight

struct FBPipe { int x, topH; };   // topH = height of top pipe

int      fbBirdY4;   // bird Y × 4 (sub-pixel)
int      fbBirdV4;   // bird velocity × 4
int      fbScore;
int      fbBestScore;
bool     fbGameOver;
bool     fbStarted;
FBPipe   fbPipes[FB_NPIIPES];
uint32_t fbLastFrame;
int      fbPipeSpeed;  // pixels per frame (×4)

// -- Init --
void fbResetPipes() {
  int spacing = 52;
  for (int i = 0; i < FB_NPIIPES; i++) {
    fbPipes[i].x    = FB_W + 20 + i * spacing;
    fbPipes[i].topH = 8 + random(FB_GROUND - FB_GAP - 14);
  }
}

void flappyStart() {
  fbBirdY4   = (FB_GROUND / 2) * 4;
  fbBirdV4   = 0;
  fbScore    = 0;
  fbGameOver = false;
  fbStarted  = false;
  fbPipeSpeed = 6;   // ×4 sub-pixel → 1.5 px/frame
  fbResetPipes();
  fbLastFrame = millis();
}

// -- Draw --
void fbDraw() {
  display.clearDisplay();

  // Ground
  display.drawFastHLine(0, FB_GROUND, FB_W, WHITE);

  // Pipes
  for (auto &p : fbPipes) {
    if (p.x > FB_W || p.x + FB_PIPE_W < 0) continue;
    // Top pipe
    display.fillRect(p.x, 0, FB_PIPE_W, p.topH, WHITE);
    // Cap
    display.fillRect(p.x - 1, p.topH - 3, FB_PIPE_W + 2, 3, WHITE);
    // Bottom pipe
    int botY = p.topH + FB_GAP;
    display.fillRect(p.x, botY, FB_PIPE_W, FB_GROUND - botY, WHITE);
    // Cap
    display.fillRect(p.x - 1, botY, FB_PIPE_W + 2, 3, WHITE);
  }

  // Bird
  int by = fbBirdY4 / 4;
  // Body
  display.fillRect(FB_BIRD_X + 1, by,     FB_BIRD_W - 2, FB_BIRD_H,     WHITE);
  display.fillRect(FB_BIRD_X,     by + 1, FB_BIRD_W,     FB_BIRD_H - 2, WHITE);
  // Eye
  display.drawPixel(FB_BIRD_X + FB_BIRD_W - 2, by + 1, BLACK);
  // Wing (flap based on velocity)
  int wingY = (fbBirdV4 > 0) ? by + FB_BIRD_H - 2 : by + 1;
  display.drawFastHLine(FB_BIRD_X + 1, wingY, 3, BLACK);

  // Score
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(54, 2);
  display.print(fbScore);

  if (!fbStarted) {
    display.setCursor(18, 24);
    display.print(F("Press ROT to flap!"));
  }

  display.display();
}

// -- Loop --
void flappyLoop() {
  if (fbGameOver) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(16, 4); display.print(F("GAME OVER"));
    display.setTextSize(1);
    display.setCursor(4, 26); display.print(F("Score: ")); display.print(fbScore);
    display.setCursor(4, 36); display.print(F("Best:  ")); display.print(fbBestScore);
    display.setCursor(4, 46); display.print(F("Left = Exit"));
    display.setCursor(4, 56); display.print(F("To leave, press the left button"));
    display.display();
    delay(100);
    if (digitalRead(BTN_R)   == LOW) { flappyStart(); delay(300); }
    if (digitalRead(BTN_L) == LOW) { appState = MENU; menuSel = 0; delay(300); }
    return;
  }

  uint32_t now = millis();
  if (now - fbLastFrame < 50) return;   // 20 fps
  fbLastFrame = now;

  // Flap
  static bool fbRotLast = false;
  bool fbRotNow = digitalRead(BTN_ROT) == LOW;
  if (fbRotNow && !fbRotLast) {
    fbBirdV4 = FB_FLAP * 4;
    fbStarted = true;
  }
  fbRotLast = fbRotNow;

  if (!fbStarted) { fbDraw(); return; }

  // Physics
  fbBirdV4 += FB_GRAVITY * 4;
  if (fbBirdV4 > 20) fbBirdV4 = 20;   // terminal velocity
  fbBirdY4 += fbBirdV4;

  int by = fbBirdY4 / 4;

  // Hit ground or ceiling
  if (by + FB_BIRD_H >= FB_GROUND || by < 0) {
    fbGameOver = true;
    if (fbScore > fbBestScore) fbBestScore = fbScore;
    return;
  }

  // Move pipes
  for (auto &p : fbPipes) {
    p.x -= fbPipeSpeed / 4;
    if (p.x + FB_PIPE_W < 0) {
      // Recycle pipe
      int maxX = 0;
      for (auto &q : fbPipes) if (q.x > maxX) maxX = q.x;
      p.x    = maxX + 52;
      p.topH = 8 + random(FB_GROUND - FB_GAP - 14);
      fbScore++;
      // Speed up every 5 points
      if (fbScore % 5 == 0 && fbPipeSpeed < 14) fbPipeSpeed++;
    }
  }

  // Collision with pipes
  for (auto &p : fbPipes) {
    if (FB_BIRD_X + FB_BIRD_W < p.x - 1 || FB_BIRD_X > p.x + FB_PIPE_W) continue;
    if (by < p.topH || by + FB_BIRD_H > p.topH + FB_GAP) {
      fbGameOver = true;
      if (fbScore > fbBestScore) fbBestScore = fbScore;
      return;
    }
  }

  fbDraw();
}
