#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define BTN_UP    4   // Reuse ROT as UP
#define BTN_R     3
#define BTN_L     1
#define SDA_PIN   5
#define SCL_PIN   6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int BS = 4;                        // block size in pixels
const int COLS = SCREEN_WIDTH  / BS;     // 32
const int ROWS = SCREEN_HEIGHT / BS;     // 16
const int MAX_LEN = COLS * ROWS;

struct Pt { int8_t x, y; };

Pt snake[COLS * ROWS];
int snakeLen;
Pt food;
int8_t dx, dy;       // current direction
int8_t ndx, ndy;     // buffered next direction
bool alive;
int score, hi;
uint32_t lastMove;
int speed = 200;      // ms per step

void placeFood() {
  bool ok;
  do {
    food = {(int8_t)random(COLS), (int8_t)random(ROWS)};
    ok = true;
    for (int i = 0; i < snakeLen; i++)
      if (snake[i].x == food.x && snake[i].y == food.y) { ok = false; break; }
  } while (!ok);
}

void spawnGame() {
  snakeLen = 4;
  int sx = COLS / 2, sy = ROWS / 2;
  for (int i = 0; i < snakeLen; i++) snake[i] = {(int8_t)(sx - i), (int8_t)sy};
  dx = 1; dy = 0; ndx = 1; ndy = 0;
  score = 0; alive = true;
  placeFood();
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_L,  INPUT_PULLUP);
  pinMode(BTN_R,  INPUT_PULLUP);
  randomSeed(analogRead(0));
  spawnGame();
}

void drawScreen() {
  display.clearDisplay();

  // Board border
  display.drawRect(0, 0, COLS * BS + 1, ROWS * BS + 1, WHITE);

  // Snake
  for (int i = 0; i < snakeLen; i++) {
    int px = snake[i].x * BS + 1;
    int py = snake[i].y * BS + 1;
    if (i == 0) display.fillRect(px, py, BS - 1, BS - 1, WHITE);   // head solid
    else        display.drawRect(px, py, BS - 2, BS - 2, WHITE);   // body outline
  }

  // Food (blinking dot with border)
  display.drawRect(food.x * BS + 1, food.y * BS + 1, BS - 2, BS - 2, WHITE);
  display.fillRect(food.x * BS + 2, food.y * BS + 2, BS - 4, BS - 4, WHITE);

  // Score sidebar
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(COLS * BS + 4, 2);
  display.println(F("SC"));
  display.setCursor(COLS * BS + 4, 12);
  display.println(score);
  display.setCursor(COLS * BS + 4, 28);
  display.println(F("HI"));
  display.setCursor(COLS * BS + 4, 38);
  display.println(hi);

  display.display();
}

void gameOverScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(18, 20);
  display.println(F("GAME OVER"));
  display.setCursor(18, 32);
  display.print(F("Score: ")); display.println(score);
  display.setCursor(10, 44);
  display.println(F("Hold L+R restart"));
  display.display();
}

void loop() {
  // --- Input ---
  // Rotation/Up cycles direction: right → up → left → down
  if (digitalRead(BTN_UP) == LOW) {
    // Rotate direction 90° CCW: (dx,dy)->(dy,-dx)
    int8_t tmp = ndx;
    ndx = ndy; ndy = -tmp;
    delay(150);
  }
  if (digitalRead(BTN_L) == LOW && !(ndx == 1 && ndy == 0))  { ndx = -1; ndy = 0; }
  if (digitalRead(BTN_R) == LOW && !(ndx == -1 && ndy == 0)) { ndx =  1; ndy = 0; }

  // Hold both L+R on game-over to restart
  if (!alive) {
    if (digitalRead(BTN_L) == LOW && digitalRead(BTN_R) == LOW) {
      delay(300);
      spawnGame();
    }
    gameOverScreen();
    return;
  }

  // --- Move ---
  if (millis() - lastMove > speed) {
    lastMove = millis();

    // Apply buffered direction (prevent 180° reversal)
    if (!(ndx == -dx && ndy == -dy)) { dx = ndx; dy = ndy; }

    Pt head = {(int8_t)(snake[0].x + dx), (int8_t)(snake[0].y + dy)};

    // Wall collision
    if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
      alive = false; return;
    }
    // Self collision
    for (int i = 0; i < snakeLen; i++)
      if (snake[i].x == head.x && snake[i].y == head.y) { alive = false; return; }

    // Shift body
    for (int i = snakeLen; i > 0; i--) snake[i] = snake[i - 1];
    snake[0] = head;

    // Eat food?
    if (head.x == food.x && head.y == food.y) {
      snakeLen++;
      score++;
      if (score > hi) hi = score;
      if (score % 5 == 0 && speed > 80) speed -= 20; // speed up every 5 pts
      placeFood();
    }
  }

  drawScreen();
}