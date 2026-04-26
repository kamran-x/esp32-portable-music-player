#pragma once

extern AppState appState;
extern int menuSel;

const int S_BS = 4;
const int S_COLS = (128 / S_BS);
const int S_ROWS = (64  / S_BS);
const int S_MAX  = S_COLS * S_ROWS;

struct SnakePt { int8_t x, y; };

SnakePt sBody[S_MAX];
int     sLen;
SnakePt sFood;
int8_t  sDx, sDy, sNdx, sNdy;
uint32_t sLastMove;
int     sSpeed;
bool    sGameOver = false;

void sPlaceFood() {
  SnakePt p;
  bool ok;
  do {
    p = { (int8_t)random(S_COLS), (int8_t)random(S_ROWS) };
    ok = true;
    for (int i = 0; i < sLen; i++)
      if (sBody[i].x == p.x && sBody[i].y == p.y) { ok = false; break; }
  } while (!ok);
  sFood = p;
}

void snakeStart() {
  sLen = 4; sSpeed = 200; sGameOver = false;
  int sx = S_COLS/2, sy = S_ROWS/2;
  for (int i = 0; i < sLen; i++) sBody[i] = {(int8_t)(sx-i),(int8_t)sy};
  sDx=1; sDy=0; sNdx=1; sNdy=0;
  sLastMove = millis();
  sPlaceFood();
}

void snakeLoop() {
  if (sGameOver) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(12, 5); display.print(F("GAME OVER"));
    display.setTextSize(1);
    display.setCursor(4,  28); display.print(F("R-btn > Play again"));
    display.setCursor(4,  40); display.print(F("C-btn > Main menu"));
    display.setCursor(4,  52); display.print(F("SNAKE by Kamran"));
    display.display();
    delay(340);

    if (digitalRead(BTN_R) == LOW)   { snakeStart(); delay(300); }
    if (digitalRead(BTN_ROT) == LOW) { appState = MENU; menuSel = 0; delay(300); }
    return;
  }

  if (digitalRead(BTN_L) == LOW && !(sNdx==1))  { sNdx=-1; sNdy=0; }
  if (digitalRead(BTN_R) == LOW && !(sNdx==-1)) { sNdx= 1; sNdy=0; }
  static bool sRotLast = false;
  bool sRotNow = digitalRead(BTN_ROT) == LOW;
  if (sRotNow && !sRotLast) {
    int8_t tmp = sNdx; sNdx = sNdy; sNdy = -tmp;
  }
  sRotLast = sRotNow;

  if (millis() - sLastMove > (uint32_t)sSpeed) {
    sLastMove = millis();
    if (!(sNdx==-sDx && sNdy==-sDy)) { sDx=sNdx; sDy=sNdy; }
    SnakePt head = {(int8_t)(sBody[0].x+sDx),(int8_t)(sBody[0].y+sDy)};
    bool dead = head.x<0||head.x>=S_COLS||head.y<0||head.y>=S_ROWS;
    if (!dead) for (int i=0;i<sLen;i++) if(sBody[i].x==head.x&&sBody[i].y==head.y){dead=true;break;}
    if (dead) { sGameOver = true; return; }
    for (int i=sLen;i>0;i--) sBody[i]=sBody[i-1];
    sBody[0]=head;
    if (head.x==sFood.x && head.y==sFood.y) {
      sLen++;
      if (sLen%5==0 && sSpeed>80) sSpeed-=20;
      sPlaceFood();
    }
  }

  display.clearDisplay();
  display.drawRect(0,0,S_COLS*S_BS+1,S_ROWS*S_BS+1,WHITE);
  for (int i=0;i<sLen;i++) {
    int px=sBody[i].x*S_BS+1, py=sBody[i].y*S_BS+1;
    if(i==0) display.fillRect(px,py,S_BS-1,S_BS-1,WHITE);
    else     display.drawRect(px,py,S_BS-2,S_BS-2,WHITE);
  }
  display.drawRect(sFood.x*S_BS+1,sFood.y*S_BS+1,S_BS-2,S_BS-2,WHITE);
  display.fillRect(sFood.x*S_BS+2,sFood.y*S_BS+2,S_BS-4,S_BS-4,WHITE);
  display.display();
}