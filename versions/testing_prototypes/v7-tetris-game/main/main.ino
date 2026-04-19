#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// Your Specific Pins
#define BTN_ROT  4  // Rotate
#define BTN_R    3  // Right
#define BTN_L    1  // Left
#define SDA_PIN  5
#define SCL_PIN  6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Game constants
const int ROWS = 12;
const int COLS = 10;
const int BLOCK_SIZE = 4;
const int OFFSET_X = 40; // Center the game on 128px screen

bool field[COLS][ROWS] = {0};
int curX = 4, curY = 0, curRot = 0, curType = 0;
uint32_t lastDrop = 0;

// Tetromino Shapes (Simplified 4x4 matrix)
const uint16_t shapes[7][4] = {
  {0x0F00, 0x4444, 0x0F00, 0x4444}, // I
  {0x4460, 0x0E80, 0xC440, 0x2E00}, // L
  {0x44C0, 0x8E00, 0x6440, 0x0E20}, // J
  {0x0660, 0x0660, 0x0660, 0x0660}, // O
  {0x06C0, 0x8C40, 0x06C0, 0x8C40}, // S
  {0x0C60, 0x4C80, 0x0C60, 0x4C80}, // Z
  {0x4E00, 0x4640, 0x0E40, 0x4C40}  // T
};

bool checkCollision(int x, int y, int r) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (shapes[curType][r] & (0x8000 >> (i * 4 + j))) {
        int nx = x + j;
        int ny = y + i;
        if (nx < 0 || nx >= COLS || ny >= ROWS || (ny >= 0 && field[nx][ny])) return true;
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
  // Check for lines
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
  if (checkCollision(curX, curY, curRot)) { // Game Over
    memset(field, 0, sizeof(field));
  }
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  pinMode(BTN_ROT, INPUT_PULLUP);
  pinMode(BTN_L, INPUT_PULLUP);
  pinMode(BTN_R, INPUT_PULLUP);
  spawn();
}

void loop() {
  // Input Handling
  if (digitalRead(BTN_L) == LOW) if (!checkCollision(curX - 1, curY, curRot)) curX--;
  if (digitalRead(BTN_R) == LOW) if (!checkCollision(curX + 1, curY, curRot)) curX++;
  if (digitalRead(BTN_ROT) == LOW) {
    int nextRot = (curRot + 1) % 4;
    if (!checkCollision(curX, curY, nextRot)) curRot = nextRot;
    delay(150); // Debounce for rotation
  }

  // Gravity
  if (millis() - lastDrop > 500) {
    if (!checkCollision(curX, curY + 1, curRot)) curY++;
    else lockPiece();
    lastDrop = millis();
  }

  // Draw
  display.clearDisplay();
  display.drawRect(OFFSET_X - 1, 0, COLS * BLOCK_SIZE + 2, ROWS * BLOCK_SIZE + 1, WHITE);
  
  for(int x=0; x<COLS; x++)
    for(int y=0; y<ROWS; y++)
      if(field[x][y]) display.fillRect(OFFSET_X + x*BLOCK_SIZE, y*BLOCK_SIZE, BLOCK_SIZE-1, BLOCK_SIZE-1, WHITE);

  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++)
      if(shapes[curType][curRot] & (0x8000 >> (i*4+j)))
        display.fillRect(OFFSET_X + (curX+j)*BLOCK_SIZE, (curY+i)*BLOCK_SIZE, BLOCK_SIZE-1, BLOCK_SIZE-1, WHITE);

  display.display();
  delay(50);
}

