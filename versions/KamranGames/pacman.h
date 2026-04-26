#pragma once

extern AppState appState;
extern int menuSel;

// -- Grid --
// 21×11 tile map, each tile = 6×6 px → 126×66 px (fits 128×64 with 1px margin)
const int PM_TW = 6, PM_TH = 6;
const int PM_COLS = 21, PM_ROWS = 11;
const int PM_YOFF = -1;  // shift whole game 1px up

// Tile types
const uint8_t PM_WALL  = 1;
const uint8_t PM_DOT   = 2;
const uint8_t PM_POWER = 3;
const uint8_t PM_EMPTY = 0;

// Base maze – mirrored left/right for symmetry
// W=wall, .=dot, P=power pellet, _=empty (ghost house)
static const uint8_t PM_MAZE_INIT[PM_ROWS][PM_COLS] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,3,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,3,1},
  {1,2,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,2,1},
  {1,2,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,2,1},
  {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
  {1,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,2,1,1,2,1},
  {1,2,2,2,2,1,2,2,2,1,0,1,2,2,2,1,2,2,2,2,1},
  {1,1,1,1,2,1,1,1,0,1,0,1,0,1,1,1,2,1,1,1,1},
  {0,0,0,1,2,1,0,0,0,0,0,0,0,0,0,1,2,1,0,0,0},
  {0,0,0,1,2,1,0,1,1,1,1,1,1,1,0,1,2,1,0,0,0},
  {1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1},
};

uint8_t pmMap[PM_ROWS][PM_COLS];
int     pmScore, pmLives, pmDots;
bool    pmGameOver = false;
bool    pmWon      = false;

// -- Pac-Man --
struct PacManState {
  int16_t x, y;      // pixel position (top-left of sprite)
  int8_t  dx, dy;    // current velocity
  int8_t  ndx, ndy;  // buffered next direction
  uint8_t frame;     // mouth animation frame 0-3
  uint8_t moveTimer;
};
PacManState pm;

// -- Ghosts --
const int PM_NGHOSTS = 3;
struct Ghost {
  int16_t x, y;
  int8_t  dx, dy;
  uint8_t moveTimer;
  bool    frightened;
  uint8_t frighTimer;
  bool    alive;
};
Ghost pmGhosts[PM_NGHOSTS];

uint8_t pmFrightTotal = 0;  // frames remaining for fright mode
uint32_t pmLastFrame  = 0;

// -- Helpers --
inline int pmTileX(int16_t px) { return px / PM_TW; }
inline int pmTileY(int16_t py) { return py / PM_TH; }
inline bool pmWallAt(int tx, int ty) {
  if (tx < 0 || tx >= PM_COLS || ty < 0 || ty >= PM_ROWS) return true;
  return pmMap[ty][tx] == PM_WALL;
}
inline bool pmCanMove(int16_t px, int16_t py, int8_t dx, int8_t dy) {
  int nx = px + dx * PM_TW;
  int ny = py + dy * PM_TH;
  return !pmWallAt(nx / PM_TW, ny / PM_TH);
}

// -- Initialise --
void pmInitGhosts() {
  // Start positions scattered around the ghost house (tile coords)
  int8_t sx[PM_NGHOSTS] = {9, 10, 11};
  int8_t sy[PM_NGHOSTS] = {8,  8,  8};
  int8_t sdx[PM_NGHOSTS] = {-1, 1, -1};
  for (int i = 0; i < PM_NGHOSTS; i++) {
    pmGhosts[i] = {
      (int16_t)(sx[i]*PM_TW), (int16_t)(sy[i]*PM_TH),
      sdx[i], 0,
      (uint8_t)(4 + i*2),
      false, 0, true
    };
  }
}

void pacmanStart() {
  memcpy(pmMap, PM_MAZE_INIT, sizeof(pmMap));
  pmScore = 0; pmLives = 3; pmGameOver = false; pmWon = false;
  pmFrightTotal = 0;
  // Count total dots
  pmDots = 0;
  for (int r = 0; r < PM_ROWS; r++)
    for (int c = 0; c < PM_COLS; c++)
      if (pmMap[r][c] == PM_DOT || pmMap[r][c] == PM_POWER) pmDots++;
  // Place Pac-Man at tile (10,4) – open corridor
  pm = {10*PM_TW, 4*PM_TH, 1, 0, 1, 0, 0, 3};
  pmInitGhosts();
  pmLastFrame = millis();
}

// -- Ghost AI --
void pmMoveGhost(Ghost &g) {
  if (!g.alive) return;
  if (--g.moveTimer > 0) return;
  g.moveTimer = g.frightened ? 8 : 5;

  // Try to continue, else pick random valid turn
  int8_t dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
  // Shuffle dirs for variety
  for (int i = 3; i > 0; i--) {
    int j = random(i+1);
    int8_t tx = dirs[i][0], ty = dirs[i][1];
    dirs[i][0] = dirs[j][0]; dirs[i][1] = dirs[j][1];
    dirs[j][0] = tx;         dirs[j][1] = ty;
  }
  // Prefer current direction if possible (and not reversing)
  bool moved = false;
  if (!pmWallAt(pmTileX(g.x)+g.dx, pmTileY(g.y)+g.dy)) {
    g.x += g.dx * PM_TW;
    g.y += g.dy * PM_TH;
    moved = true;
  }
  if (!moved) {
    for (auto &d : dirs) {
      if (d[0] == -g.dx && d[1] == -g.dy) continue; // no 180°
      if (!pmWallAt(pmTileX(g.x)+d[0], pmTileY(g.y)+d[1])) {
        g.dx = d[0]; g.dy = d[1];
        g.x += g.dx * PM_TW;
        g.y += g.dy * PM_TH;
        break;
      }
    }
  }
  // Wrap horizontal tunnel (rows 6,8 at x=-1 and x=PM_COLS)
  if (g.x < 0)              g.x = (PM_COLS-1)*PM_TW;
  if (g.x >= PM_COLS*PM_TW) g.x = 0;
}

// -- Draw --
void pmDraw() {
  display.clearDisplay();

  // Map
  for (int r = 0; r < PM_ROWS; r++) {
    for (int c = 0; c < PM_COLS; c++) {
      int px = c * PM_TW + 1;
      int py = r * PM_TH + PM_YOFF;
      switch (pmMap[r][c]) {
        case PM_WALL:
          display.fillRect(px, py, PM_TW-1, PM_TH-1, WHITE);
          break;
        case PM_DOT:
          display.drawPixel(px + PM_TW/2 - 1, py + PM_TH/2 - 1, WHITE);
          break;
        case PM_POWER:
          display.fillRect(px+1, py+1, PM_TW-3, PM_TH-3, WHITE);
          break;
      }
    }
  }

  // Pac-Man mouth animation
  uint8_t f = pm.frame % 4;
  int px = pm.x + 1;
  int py = pm.y + PM_YOFF;
  if (f == 0 || f == 3) {
    // Closed – full circle approximation (3×3 filled + corners)
    display.fillRect(px+1, py,   PM_TW-3, PM_TH,   WHITE);
    display.fillRect(px,   py+1, PM_TW-1, PM_TH-2, WHITE);
  } else {
    // Open mouth facing direction
    display.fillRect(px+1, py,   PM_TW-3, PM_TH,   WHITE);
    display.fillRect(px,   py+1, PM_TW-1, PM_TH-2, WHITE);
    // Cut mouth wedge based on direction
    if (pm.dx==1) {  // right
      display.fillRect(px+PM_TW/2, py+1,       PM_TW/2-1, PM_TH/2-1, BLACK);
      display.fillRect(px+PM_TW/2, py+PM_TH/2, PM_TW/2-1, PM_TH/2-1, BLACK);
    } else if (pm.dx==-1) {  // left
      display.fillRect(px,        py+1,       PM_TW/2,   PM_TH/2-1, BLACK);
      display.fillRect(px,        py+PM_TH/2, PM_TW/2,   PM_TH/2-1, BLACK);
    } else if (pm.dy==-1) {  // up
      display.fillRect(px+1, py,        PM_TW/2-1, PM_TH/2, BLACK);
      display.fillRect(px+PM_TW/2, py, PM_TW/2-1, PM_TH/2, BLACK);
    } else {  // down
      display.fillRect(px+1, py+PM_TH/2, PM_TW/2-1, PM_TH/2, BLACK);
      display.fillRect(px+PM_TW/2, py+PM_TH/2, PM_TW/2-1, PM_TH/2, BLACK);
    }
  }

  // Ghosts
  for (auto &g : pmGhosts) {
    if (!g.alive) continue;
    int gx = g.x + 1, gy = g.y + PM_YOFF;
    if (g.frightened) {
      // Frightened: inverted outline
      display.drawRect(gx, gy, PM_TW-1, PM_TH-1, WHITE);
      display.drawPixel(gx+1, gy+2, WHITE);
      display.drawPixel(gx+PM_TW-3, gy+2, WHITE);
    } else {
      // Body
      display.fillRect(gx+1, gy,   PM_TW-3, PM_TH,   WHITE);
      display.fillRect(gx,   gy+1, PM_TW-1, PM_TH-2, WHITE);
      // Wavy bottom
      display.drawPixel(gx,        gy+PM_TH-1, BLACK);
      display.drawPixel(gx+2,      gy+PM_TH-1, BLACK);
      display.drawPixel(gx+PM_TW-3,gy+PM_TH-1, BLACK);
      // Eyes (black dots = cut out of white body)
      display.drawPixel(gx+1, gy+2, BLACK);
      display.drawPixel(gx+PM_TW-3, gy+2, BLACK);
    }
  }

  // HUD
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print(F("SC:"));
  display.print(pmScore);
  display.setCursor(60, 55);
  for (int i = 0; i < pmLives; i++) display.print(F("*"));

  display.display();
}

// -- Main loop --
void pacmanLoop() {
  if (pmGameOver || pmWon) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(pmWon ? 22 : 12, 5);
    display.print(pmWon ? F("YOU WIN!") : F("GAME OVER"));
    display.setTextSize(1);
    display.setCursor(4, 28); display.print(F("Score: ")); display.print(pmScore);
    display.setCursor(4, 38); display.print(F("Right = Replay"));
    display.setCursor(4, 48); display.print(F("Left = Exit"));
    display.setCursor(4, 58); display.print(F("PACMAN by Kamran"));
    display.display();
    delay(100);
    if (digitalRead(BTN_R)   == LOW) { pacmanStart(); delay(300); }
    if (digitalRead(BTN_L) == LOW) { appState = MENU; menuSel = 0; delay(300); }
    return;
  }

  uint32_t now = millis();
  if (now - pmLastFrame < 80) return;   // ~12 fps
  pmLastFrame = now;

  // -- Input: buffer next direction --
  if (digitalRead(BTN_L) == LOW) { pm.ndx = -1; pm.ndy =  0; }
  if (digitalRead(BTN_R) == LOW) { pm.ndx =  1; pm.ndy =  0; }
  static bool pmRotLast = false;
  bool pmRotNow = digitalRead(BTN_ROT) == LOW;
  if (pmRotNow && !pmRotLast) {
    // Tap ROT: cycles up -> down -> up
    pm.ndy = (pm.ndy == -1) ? 1 : -1;
    pm.ndx = 0;
  }
  pmRotLast = pmRotNow;

  // -- Move Pac-Man --
  if (pm.moveTimer > 0) pm.moveTimer--;
  if (pm.moveTimer == 0) {
    pm.moveTimer = 4;
    // Apply buffered direction if passable
    if ((pm.ndx != 0 || pm.ndy != 0) && pmCanMove(pm.x, pm.y, pm.ndx, pm.ndy)) {
      pm.dx = pm.ndx;
      pm.dy = pm.ndy;
    }
    // Move in current direction
    if (pmCanMove(pm.x, pm.y, pm.dx, pm.dy)) {
      pm.x += pm.dx * PM_TW;
      pm.y += pm.dy * PM_TH;
      pm.frame++;
    }
    // Horizontal tunnel wrap
    if (pm.x < 0)              pm.x = (PM_COLS - 1) * PM_TW;
    if (pm.x >= PM_COLS*PM_TW) pm.x = 0;
  }

  // -- Eat dot/power --
  int tc = pmTileX(pm.x), tr = pmTileY(pm.y);
  if (tc >= 0 && tc < PM_COLS && tr >= 0 && tr < PM_ROWS) {
    uint8_t t = pmMap[tr][tc];
    if (t == PM_DOT) {
      pmMap[tr][tc] = PM_EMPTY;
      pmScore += 10;
      if (--pmDots <= 0) { pmWon = true; return; }
    } else if (t == PM_POWER) {
      pmMap[tr][tc] = PM_EMPTY;
      pmScore += 50;
      if (--pmDots <= 0) { pmWon = true; return; }
      pmFrightTotal = 50;
      for (auto &g : pmGhosts) { g.frightened = true; g.frighTimer = 50; }
    }
  }

  // -- Fright countdown --
  if (pmFrightTotal > 0) {
    pmFrightTotal--;
    for (auto &g : pmGhosts)
      if (g.frightened && --g.frighTimer == 0) g.frightened = false;
  }

  // -- Move ghosts --
  for (auto &g : pmGhosts) pmMoveGhost(g);

  // --Collisions --
  for (auto &g : pmGhosts) {
    if (!g.alive) continue;
    if (abs(g.x - pm.x) < PM_TW && abs(g.y - pm.y) < PM_TH) {
      if (g.frightened) {
        g.alive = false;
        g.frightened = false;
        pmScore += 200;
      } else {
        // Lose a life
        if (--pmLives <= 0) { pmGameOver = true; return; }
        // Reset positions
        pm.x = 10*PM_TW; pm.y = 6*PM_TH;
        pm.dx = 1; pm.dy = 0; pm.ndx = 1; pm.ndy = 0;
        pmInitGhosts();
        delay(500);
        return;
      }
    }
  }

  pmDraw();
}