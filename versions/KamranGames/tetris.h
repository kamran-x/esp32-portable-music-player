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
uint32_t lastDrop = 0;
bool tGameOver = false;

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
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (shapes[curType][r] & (0x8000 >> (i * 4 + j))) {
        int nx = x + j, ny = y + i;
        if (nx < 0 || nx >= COLS || ny >= ROWS || (ny >= 0 && field[nx][ny])) return true;
      }
  return false;
}

void lockPiece() {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (shapes[curType][curRot] & (0x8000 >> (i * 4 + j)))
        field[curX + j][curY + i] = true;
  for (int y = ROWS - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < COLS; x++) if (!field[x][y]) full = false;
    if (full) {
      for (int ty = y; ty > 0; ty--)
        for (int tx = 0; tx < COLS; tx++) field[tx][ty] = field[tx][ty-1];
      y++;
    }
  }
  spawn();
}

void spawn() {
  curX = 3; curY = 0; curType = random(7); curRot = 0;
  if (checkCollision(curX, curY, curRot)) {
    tGameOver = true;
  }
}

void tetrisStart() {
  memset(field, 0, sizeof(field));
  lastDrop = millis();
  tGameOver = false;
  spawn();
}

void tetrisLoop() {
  if (tGameOver) {
    display.clearDisplay();
   display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(12, 5); display.print(F("GAME OVER"));
    display.setTextSize(1);
    display.setCursor(4,  28); display.print(F("R-btn > Play again"));
    display.setCursor(4,  40); display.print(F("C-btn > Main menu"));
    display.setCursor(4,  52); display.print(F("TETRIS by Kamran"));
    display.display();
    delay(340);

    if (digitalRead(BTN_R) == LOW)   { tetrisStart(); delay(300); }
    if (digitalRead(BTN_ROT) == LOW) { appState = MENU; menuSel = 0; delay(300); }
    return;
  }

  if (digitalRead(BTN_L) == LOW) if (!checkCollision(curX - 1, curY, curRot)) curX--;
  if (digitalRead(BTN_R) == LOW) if (!checkCollision(curX + 1, curY, curRot)) curX++;
  if (digitalRead(BTN_ROT) == LOW) {
    int nextRot = (curRot + 1) % 4;
    if (!checkCollision(curX, curY, nextRot)) curRot = nextRot;
    delay(150);
  }

  if (millis() - lastDrop > 500) {
    if (!checkCollision(curX, curY + 1, curRot)) curY++;
    else lockPiece();
    lastDrop = millis();
  }

  display.clearDisplay();
  display.drawRect(OFFSET_X - 1, 0, COLS * BLOCK_SIZE + 2, ROWS * BLOCK_SIZE + 1, WHITE);
  for (int x = 0; x < COLS; x++)
    for (int y = 0; y < ROWS; y++)
      if (field[x][y]) display.fillRect(OFFSET_X + x*BLOCK_SIZE, y*BLOCK_SIZE, BLOCK_SIZE-1, BLOCK_SIZE-1, WHITE);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (shapes[curType][curRot] & (0x8000 >> (i*4+j)))
        display.fillRect(OFFSET_X + (curX+j)*BLOCK_SIZE, (curY+i)*BLOCK_SIZE, BLOCK_SIZE-1, BLOCK_SIZE-1, WHITE);
  display.setTextColor(WHITE);
  display.setCursor(15, 53);
  display.println(F("TETRIS by Kamran"));
  display.display();
  delay(50);
}