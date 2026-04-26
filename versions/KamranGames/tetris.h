#pragma once

void tetrisStart();
void spawn();

extern AppState appState;
extern int menuSel;

const int ROWS = 12;
const int COLS = 10;
const int BLOCK_SIZE = 4;
const int OFFSET_X = 40;

bool field[COLS][ROWS] = {0};
int curX = 4, curY = 0, curRot = 0, curType = 0;
int nextType = 0;

uint32_t lastDrop = 0;
bool tGameOver = false;

// speed system
int linesClearedTotal = 0;
int dropSpeed = 500;

extern int exitPressCount;
extern bool lastLeftState;
extern uint32_t exitLastPressTime;

const uint16_t shapes[7][4] = {
  {0x0F00, 0x4444, 0x0F00, 0x4444},
  {0x4460, 0x0E80, 0xC440, 0x2E00},
  {0x44C0, 0x8E00, 0x6440, 0x0E20},
  {0x0660, 0x0660, 0x0660, 0x0660},
  {0x06C0, 0x8C40, 0x06C0, 0x8C40},
  {0x0C60, 0x4C80, 0x0C60, 0x4C80},
  {0x4E00, 0x4640, 0x0E40, 0x4C40}
};

bool checkCollision(int x, int y, int r) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (shapes[curType][r] & (0x8000 >> (i * 4 + j))) {
        int nx = x + j;
        int ny = y + i;

        if (nx < 0 || nx >= COLS || ny >= ROWS)
          return true;

        if (ny >= 0 && field[nx][ny])
          return true;
      }
    }
  }
  return false;
}

void lockPiece() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (shapes[curType][curRot] & (0x8000 >> (i * 4 + j))) {
        field[curX + j][curY + i] = true;
      }
    }
  }

  int clearedNow = 0;

  for (int y = ROWS - 1; y >= 0; y--) {
    bool full = true;

    for (int x = 0; x < COLS; x++) {
      if (!field[x][y]) full = false;
    }

    if (full) {
      clearedNow++;

      for (int ty = y; ty > 0; ty--) {
        for (int tx = 0; tx < COLS; tx++) {
          field[tx][ty] = field[tx][ty - 1];
        }
      }

      for (int tx = 0; tx < COLS; tx++) {
        field[tx][0] = false;
      }

      y++;
    }
  }

  linesClearedTotal += clearedNow;

  int level = linesClearedTotal / 5;
  dropSpeed = 500 - (level * 40);

  if (dropSpeed < 100)
    dropSpeed = 100;

  spawn();
}

void spawn() {
  curX = 3;
  curY = 0;

  curType = nextType;
  nextType = random(7);

  curRot = 0;

  if (checkCollision(curX, curY, curRot)) {
    tGameOver = true;
  }
}

void tetrisStart() {
  memset(field, 0, sizeof(field));

  lastDrop = millis();
  tGameOver = false;

  linesClearedTotal = 0;
  dropSpeed = 500;

  extern int exitPressCount;
  extern bool lastLeftState;
  extern uint32_t exitLastPressTime;

  nextType = random(7);

  spawn();
}

void drawExitProgress() {
  int x = OFFSET_X + COLS * BLOCK_SIZE + 6;
  int y = 10;

  for (int i = 0; i < 5; i++) {
    if (i < exitPressCount)
      display.fillRect(x, y + i * 6, 4, 4, WHITE);
    else
      display.drawRect(x, y + i * 6, 4, 4, WHITE);
  }
}

void drawNextPiece() {
  int previewX = OFFSET_X - 20;
  int previewY = 10;

  display.setTextSize(1);
  display.setCursor(previewX, previewY - 8);
  display.print("N");

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (shapes[nextType][0] & (0x8000 >> (i * 4 + j))) {
        display.fillRect(
          previewX + j * 3,
          previewY + i * 3,
          2,
          2,
          WHITE
        );
      }
    }
  }
}

void tetrisLoop() {

  if (tGameOver) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(12, 5);
    display.print(F("GAME OVER"));

    display.setTextSize(1);
    display.setCursor(4, 28);
    display.print(F("Right = Replay"));

    display.setCursor(4, 40);
    display.print(F("Left = Exit"));

    display.display();
    delay(100);

    if (digitalRead(BTN_R) == LOW) {
      tetrisStart();
      delay(300);
    }

    if (digitalRead(BTN_L) == LOW) {
      appState = MENU;
      menuSel = 0;
      delay(300);
    }

    return;
  }

  // timeout reset
  if (exitPressCount > 0 && millis() - exitLastPressTime > 1500) {
    exitPressCount = 0;
  }

  // exit logic
  bool currentLeft = digitalRead(BTN_L);

  if (lastLeftState == HIGH && currentLeft == LOW) {
    exitPressCount++;
    exitLastPressTime = millis();

    if (exitPressCount >= 5) {
      appState = MENU;
      menuSel = 0;
      exitPressCount = 0;
      delay(300);
      return;
    }
  }

  if (digitalRead(BTN_R) == LOW || digitalRead(BTN_ROT) == LOW) {
    exitPressCount = 0;
  }

  lastLeftState = currentLeft;

  // movement
  if (digitalRead(BTN_L) == LOW) {
    if (!checkCollision(curX - 1, curY, curRot))
      curX--;
  }

  if (digitalRead(BTN_R) == LOW) {
    if (!checkCollision(curX + 1, curY, curRot))
      curX++;
  }

  if (digitalRead(BTN_ROT) == LOW) {
    int nextRot = (curRot + 1) % 4;

    if (!checkCollision(curX, curY, nextRot))
      curRot = nextRot;

    delay(150);
  }

  // falling
  if (millis() - lastDrop > dropSpeed) {
    if (!checkCollision(curX, curY + 1, curRot))
      curY++;
    else
      lockPiece();

    lastDrop = millis();
  }

  // draw
  display.clearDisplay();

  display.drawRect(
    OFFSET_X - 1,
    0,
    COLS * BLOCK_SIZE + 2,
    ROWS * BLOCK_SIZE + 1,
    WHITE
  );

  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      if (field[x][y]) {
        display.fillRect(
          OFFSET_X + x * BLOCK_SIZE,
          y * BLOCK_SIZE,
          BLOCK_SIZE - 1,
          BLOCK_SIZE - 1,
          WHITE
        );
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (shapes[curType][curRot] & (0x8000 >> (i * 4 + j))) {
        display.fillRect(
          OFFSET_X + (curX + j) * BLOCK_SIZE,
          (curY + i) * BLOCK_SIZE,
          BLOCK_SIZE - 1,
          BLOCK_SIZE - 1,
          WHITE
        );
      }
    }
  }

  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print(F("L:"));
  display.print(linesClearedTotal);

  display.setCursor(15, 53);
  display.print(F("TETRIS"));

  // side UI
  drawNextPiece();
  drawExitProgress();

  display.display();

  delay(50);
}