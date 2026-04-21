#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define BTN_FIRE  4   // was ROT — now shoots
#define BTN_R     3
#define BTN_L     1
#define SDA_PIN   5
#define SCL_PIN   6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Game config ---
const int W = 128, H = 64;
const int E_COLS = 8, E_ROWS = 3;
const int E_W = 7, E_H = 5, E_GAP_X = 13, E_GAP_Y = 8;
const int MAX_BULLETS = 4, MAX_EBULLETS = 6, MAX_PARTICLES = 20;

struct Bullet  { int x, y; bool alive; };
struct Enemy   { int x, y, type, tick; bool alive; };
struct Particle{ int x, y; int8_t vx, vy; uint8_t life; };

Enemy    enemies[E_COLS * E_ROWS];
Bullet   bullets[MAX_BULLETS];
Bullet   eBullets[MAX_EBULLETS];
Particle parts[MAX_PARTICLES];

int playerX;
int score, hi, lives, wave;
bool gameOver;

int  enemyDir;
uint32_t lastEnemyMove, lastEShoot, lastFrame;
int  eMoveInterval;

uint8_t shootCooldown;

void spawnParticle(int x, int y) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (parts[i].life == 0) {
      parts[i] = {x, y, (int8_t)(random(-2,3)), (int8_t)(random(-2,3)), 10};
      return;
    }
  }
}

void spawnWave() {
  playerX = 60;
  for (auto &b : bullets)  b.alive = false;
  for (auto &b : eBullets) b.alive = false;
  for (auto &p : parts)    p.life  = 0;
  enemyDir = 1;
  eMoveInterval = max(150, 700 - wave * 70);
  lastEnemyMove = lastEShoot = millis();
  shootCooldown = 0;

  int idx = 0;
  int rows = min(wave + 1, 3);
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < E_COLS; c++)
      enemies[idx++] = {8 + c * E_GAP_X, 6 + r * E_GAP_Y, r, 0, true};
  for (; idx < E_COLS * E_ROWS; idx++) enemies[idx].alive = false;
}

void startGame() {
  score = 0; lives = 3; wave = 1; gameOver = false;
  spawnWave();
}

void drawEnemy(int x, int y, int type, int t) {
  t %= 2;
  if (type == 0) {
    display.drawPixel(x+3, y,   WHITE);
    display.drawPixel(x+3, y,   WHITE); display.drawFastHLine(x+1, y,   5, WHITE);
    display.drawFastHLine(x,   y+1, 7, WHITE); display.drawFastHLine(x, y+2, 7, WHITE);
    display.drawPixel(x+(t?1:2), y+3, WHITE); display.drawPixel(x+(t?1:2)+1, y+3, WHITE);
    display.drawPixel(x+(t?4:3), y+3, WHITE); display.drawPixel(x+(t?4:3)+1, y+3, WHITE);
  } else if (type == 1) {
    display.drawFastHLine(x+2, y,   3, WHITE);
    display.drawFastHLine(x+(t?0:1), y+1, 7-(t?0:2), WHITE);
    display.drawFastHLine(x,   y+2, 7, WHITE); display.drawFastHLine(x, y+3, 7, WHITE);
    display.drawPixel(x+(t?1:0), y+4, WHITE); display.drawPixel(x+(t?1:0)+1, y+4, WHITE);
    display.drawPixel(x+(t?4:5), y+4, WHITE); display.drawPixel(x+(t?4:5)+1, y+4, WHITE);
  } else {
    display.drawFastHLine(x+1, y,   5, WHITE); display.drawFastHLine(x+1, y+1, 5, WHITE);
    display.drawFastHLine(x,   y+2, 7, WHITE); display.drawFastHLine(x,   y+3, 7, WHITE);
    display.drawFastHLine(x+(t?0:1), y+4, 3, WHITE);
    display.drawFastHLine(x+(t?4:3), y+4, 3, WHITE);
  }
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  pinMode(BTN_FIRE, INPUT_PULLUP);
  pinMode(BTN_L,    INPUT_PULLUP);
  pinMode(BTN_R,    INPUT_PULLUP);
  randomSeed(analogRead(0));
  startGame();
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrame < 33) return;   // ~30 fps
  lastFrame = now;

  if (gameOver) {
    if (digitalRead(BTN_FIRE) == LOW && digitalRead(BTN_L) == LOW) {
      delay(300); startGame();
    }
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(20, 20); display.println(F("GAME OVER"));
    display.setCursor(20, 32); display.print(F("Score:")); display.println(score);
    display.setCursor(10, 44); display.println(F("FIRE+L to restart"));
    display.display();
    return;
  }

  // --- Input ---
  if (digitalRead(BTN_L) == LOW && playerX > 1)          playerX -= 2;
  if (digitalRead(BTN_R) == LOW && playerX < W - 9)      playerX += 2;
  if (digitalRead(BTN_FIRE) == LOW && shootCooldown == 0) {
    for (auto &b : bullets) {
      if (!b.alive) { b = {playerX + 3, 55, true}; break; }
    }
    shootCooldown = 12;
  }
  if (shootCooldown) shootCooldown--;

  // --- Move player bullets ---
  for (auto &b : bullets)  { if (b.alive) { b.y -= 3; if (b.y < 0) b.alive = false; } }
  for (auto &b : eBullets) { if (b.alive) { b.y++;    if (b.y > H) b.alive = false; } }

  // --- Move enemies ---
  if (now - lastEnemyMove > (uint32_t)eMoveInterval) {
    lastEnemyMove = now;
    int minX = W, maxX = 0;
    for (auto &e : enemies) if (e.alive) {
      minX = min(minX, e.x);
      maxX = max(maxX, e.x + E_W);
      e.tick++;
    }
    bool flip = (enemyDir == 1 && maxX >= W - 2) || (enemyDir == -1 && minX <= 2);
    for (auto &e : enemies) if (e.alive) {
      if (flip) e.y += 4; else e.x += enemyDir * 3;
    }
    if (flip) enemyDir = -enemyDir;
  }

  // --- Enemy shoot ---
  if (now - lastEShoot > (uint32_t)(900 - wave * 60)) {
    lastEShoot = now;
    // Find bottom enemy in a random column
    int col = random(E_COLS);
    Enemy *shooter = nullptr;
    for (auto &e : enemies)
      if (e.alive && (e.x - 8) / E_GAP_X == col)
        if (!shooter || e.y > shooter->y) shooter = &e;
    if (shooter) {
      for (auto &b : eBullets) {
        if (!b.alive) { b = {shooter->x + 3, shooter->y + E_H, true}; break; }
      }
    }
  }

  // --- Collision: player bullets vs enemies ---
  for (auto &b : bullets) {
    if (!b.alive) continue;
    for (auto &e : enemies) {
      if (!e.alive) continue;
      if (b.x >= e.x && b.x <= e.x + E_W && b.y >= e.y && b.y <= e.y + E_H) {
        e.alive = false; b.alive = false;
        for (int p = 0; p < 4; p++) spawnParticle(e.x + 3, e.y + 2);
        score += (3 - e.type) * 10 + 10;
        if (score > hi) hi = score;
      }
    }
  }

  // --- Collision: enemy bullets vs player ---
  for (auto &b : eBullets) {
    if (!b.alive) continue;
    if (b.x >= playerX && b.x <= playerX + 7 && b.y >= 57 && b.y <= 61) {
      b.alive = false;
      for (int p = 0; p < 6; p++) spawnParticle(playerX + 3, 58);
      if (--lives <= 0) gameOver = true;
    }
  }

  // --- Enemies reaching bottom ---
  for (auto &e : enemies)
    if (e.alive && e.y + E_H >= 57) { lives = 0; gameOver = true; }

  // --- Particles ---
  for (auto &p : parts) if (p.life) {
    p.x += p.vx; p.y += p.vy; p.life--;
  }

  // --- Wave clear? ---
  bool anyAlive = false;
  for (auto &e : enemies) if (e.alive) { anyAlive = true; break; }
  if (!anyAlive) { wave++; spawnWave(); }

  // --- Draw ---
  display.clearDisplay();
  for (auto &e : enemies) if (e.alive) drawEnemy(e.x, e.y, e.type, e.tick);
  for (auto &b : bullets)  if (b.alive) display.drawFastVLine(b.x, b.y, 2, WHITE);
  for (auto &b : eBullets) if (b.alive) {
    display.drawPixel(b.x, b.y, WHITE);
    display.drawPixel(b.x, b.y + 2, WHITE);
  }
  for (auto &p : parts) if (p.life) display.drawPixel(p.x, p.y, WHITE);

  // Player ship
  display.drawPixel(playerX + 3, 57, WHITE);
  display.drawFastHLine(playerX + 2, 58, 3, WHITE);
  display.drawFastHLine(playerX,     59, 7, WHITE);
  display.drawFastHLine(playerX,     60, 7, WHITE);

  // HUD
  display.drawFastHLine(0, H - 1, W, WHITE);
  display.setTextSize(1); display.setTextColor(WHITE);
  display.setCursor(0,  2); display.print(score);
  display.setCursor(90, 2); display.print(F("W:")); display.print(wave);
  display.setCursor(50, 2);
  for (int i = 0; i < lives; i++) display.print(F("*"));

  display.display();
}
