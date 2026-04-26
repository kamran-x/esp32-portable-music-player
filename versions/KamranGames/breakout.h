#pragma once

extern Adafruit_SSD1306 display;
extern AppState appState;
extern int menuSel;

#define BTN_ROT  4
#define BTN_R    3
#define BTN_L    1

// paddle
int paddleX;
const int paddleW = 20;

// ball
float ballX, ballY;
float ballVX, ballVY;

// bricks
#define BRICK_ROWS 4
#define BRICK_COLS 10
bool bricks[BRICK_COLS][BRICK_ROWS];

bool gameOver = false;
bool gameWin = false;

// --- START ---
void breakoutStart() {
  paddleX = 54;

  ballX = 64;
  ballY = 40;
  ballVX = 1.5;
  ballVY = -1.5;

  gameOver = false;
  gameWin = false;

  // fill bricks
  for (int x = 0; x < BRICK_COLS; x++) {
    for (int y = 0; y < BRICK_ROWS; y++) {
      bricks[x][y] = true;
    }
  }
}

// --- DRAW BRICKS ---
void drawBricks() {
  for (int x = 0; x < BRICK_COLS; x++) {
    for (int y = 0; y < BRICK_ROWS; y++) {
      if (bricks[x][y]) {
        display.fillRect(x * 12 + 2, y * 6 + 2, 10, 4, WHITE);
      }
    }
  }
}

// --- CHECK WIN ---
bool checkWin() {
  for (int x = 0; x < BRICK_COLS; x++) {
    for (int y = 0; y < BRICK_ROWS; y++) {
      if (bricks[x][y]) return false;
    }
  }
  return true;
}

// --- LOOP ---
void breakoutLoop() {

  // ===== GAME OVER / WIN SCREEN =====
  if (gameOver || gameWin) {
    display.clearDisplay();
    display.setTextColor(WHITE);

    display.setTextSize(2);
    display.setCursor(10, 5);
    if (gameWin)
      display.print("YOU WIN");
    else
      display.print("GAME OVER");

    display.setTextSize(1);
    display.setCursor(4, 30);
    display.print("Right = Replay");

    display.setCursor(4, 42);
    display.print("Left = Exit");

    display.setCursor(4, 54);
    display.print("BREAKOUT");

    display.display();
    delay(100);

    if (digitalRead(BTN_R) == LOW) {
      breakoutStart();
      delay(300);
    }

    if (digitalRead(BTN_L) == LOW) {
      appState = MENU;
      menuSel = 0;
      delay(300);
    }

    return;
  }

  // ===== INPUT =====
  if (digitalRead(BTN_L) == LOW) paddleX -= 3;
  if (digitalRead(BTN_R) == LOW) paddleX += 3;

  if (paddleX < 0) paddleX = 0;
  if (paddleX > 128 - paddleW) paddleX = 128 - paddleW;

  // ===== BALL MOVE =====
  ballX += ballVX;
  ballY += ballVY;

  // walls
  if (ballX <= 0) { ballX = 0; ballVX *= -1; }
  if (ballX >= 126) { ballX = 126; ballVX *= -1; }
  if (ballY <= 0) { ballY = 0; ballVY *= -1; }

  // paddle
  if (ballY >= 56 && ballY <= 60 &&
      ballX >= paddleX && ballX <= paddleX + paddleW) {

    ballY = 56;
    ballVY *= -1;

    float hit = (ballX - paddleX) / (float)paddleW - 0.5;
    ballVX = hit * 4;

    if (abs(ballVX) < 0.5) {
      ballVX = (ballVX < 0) ? -0.5 : 0.5;
    }
  }

  // bricks
  bool hitBrick = false;

  for (int x = 0; x < BRICK_COLS && !hitBrick; x++) {
    for (int y = 0; y < BRICK_ROWS; y++) {
      if (!bricks[x][y]) continue;

      int bx = x * 12 + 2;
      int by = y * 6 + 2;

      if (ballX >= bx && ballX <= bx + 10 &&
          ballY >= by && ballY <= by + 4) {

        bricks[x][y] = false;
        ballVY *= -1;

        ballVX *= 1.02;
        ballVY *= 1.02;

        hitBrick = true;
        break;
      }
    }
  }

  // win / lose
  if (checkWin()) gameWin = true;
  if (ballY > 63) gameOver = true;

  // ===== DRAW =====
  display.clearDisplay();

  drawBricks();

  display.fillRect(paddleX, 58, paddleW, 4, WHITE);
  display.fillRect((int)ballX, (int)ballY, 2, 2, WHITE);

  display.display();
  delay(16);
}