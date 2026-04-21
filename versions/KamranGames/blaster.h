#pragma once

extern AppState appState;
extern int menuSel;

const int B_ECOLS=8, B_EROWS=3, B_EW=7, B_EH=5, B_EGAPX=13, B_EGAPY=8;
const int B_MAXB=4, B_MAXEB=6, B_MAXP=20;

struct BulletB   { int x,y; bool alive; };
struct EnemyB    { int x,y,type,tick; bool alive; };
struct ParticleB { int x,y; int8_t vx,vy; uint8_t life; };

EnemyB    bEnemies[B_ECOLS*B_EROWS];
BulletB   bBullets[B_MAXB];
BulletB   bEBullets[B_MAXEB];
ParticleB bParts[B_MAXP];

int      bPlayerX, bLives, bWave;
int8_t   bEDir;
uint32_t bLastEMove, bLastEShoot, bLastFrame;
int      bEMoveInterval;
uint8_t  bShootCD;
bool     bGameOver = false;

void bSpawnParticle(int x,int y) {
  for (auto &p:bParts) if(!p.life){p={x,y,(int8_t)random(-2,3),(int8_t)random(-2,3),10};return;}
}

void bSpawnWave() {
  bPlayerX=60;
  for(auto &b:bBullets)  b.alive=false;
  for(auto &b:bEBullets) b.alive=false;
  for(auto &p:bParts)    p.life=0;
  bEDir=1; bEMoveInterval=max(150,700-bWave*70);
  bLastEMove=bLastEShoot=millis(); bShootCD=0;
  int idx=0, rows=min(bWave+1,3);
  for(int r=0;r<rows;r++)
    for(int c=0;c<B_ECOLS;c++)
      bEnemies[idx++]={8+c*B_EGAPX,6+r*B_EGAPY,r,0,true};
  for(;idx<B_ECOLS*B_EROWS;idx++) bEnemies[idx].alive=false;
}

void blasterStart() {
  bLives=3; bWave=1; bGameOver=false;
  bSpawnWave();
}

void bDrawEnemy(int x,int y,int type,int t) {
  t%=2;
  if(type==0){
    display.drawFastHLine(x+1,y,5,WHITE);
    display.drawFastHLine(x,y+1,7,WHITE); display.drawFastHLine(x,y+2,7,WHITE);
    display.drawPixel(x+(t?1:2),y+3,WHITE); display.drawPixel(x+(t?1:2)+1,y+3,WHITE);
    display.drawPixel(x+(t?4:3),y+3,WHITE); display.drawPixel(x+(t?4:3)+1,y+3,WHITE);
  } else if(type==1){
    display.drawFastHLine(x+2,y,3,WHITE);
    display.drawFastHLine(x+(t?0:1),y+1,7-(t?0:2),WHITE);
    display.drawFastHLine(x,y+2,7,WHITE); display.drawFastHLine(x,y+3,7,WHITE);
    display.drawPixel(x+(t?1:0),y+4,WHITE); display.drawPixel(x+(t?1:0)+1,y+4,WHITE);
    display.drawPixel(x+(t?4:5),y+4,WHITE); display.drawPixel(x+(t?4:5)+1,y+4,WHITE);
  } else {
    display.drawFastHLine(x+1,y,5,WHITE); display.drawFastHLine(x+1,y+1,5,WHITE);
    display.drawFastHLine(x,y+2,7,WHITE); display.drawFastHLine(x,y+3,7,WHITE);
    display.drawFastHLine(x+(t?0:1),y+4,3,WHITE);
    display.drawFastHLine(x+(t?4:3),y+4,3,WHITE);
  }
}

void blasterLoop() {
  if (bGameOver) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(12, 5); display.print(F("GAME OVER"));
    display.setTextSize(1);
    display.setCursor(4,  28); display.print(F("R-btn > Play again"));
    display.setCursor(4,  40); display.print(F("C-btn > Main menu"));
    display.setCursor(4,  52); display.print(F("BLASTER by Kamran"));

    display.display();
    delay(550);
    if (digitalRead(BTN_R) == LOW)   { blasterStart(); delay(300); }
    if (digitalRead(BTN_ROT) == LOW) { appState = MENU; menuSel = 0; delay(300); }
    return;
  }

  uint32_t now=millis();
  if(now-bLastFrame<33) return;
  bLastFrame=now;

  if(digitalRead(BTN_L)==LOW && bPlayerX>1)   bPlayerX-=2;
  if(digitalRead(BTN_R)==LOW && bPlayerX<119) bPlayerX+=2;

  bool bRotNow = digitalRead(BTN_ROT)==LOW;
  if(bRotNow && bShootCD==0) {
    for(auto &b:bBullets) if(!b.alive){b={bPlayerX+3,55,true};break;}
    bShootCD=12;
  }
  if(bShootCD) bShootCD--;

  for(auto &b:bBullets)  {if(b.alive){b.y-=3;if(b.y<0)b.alive=false;}}
  for(auto &b:bEBullets) {if(b.alive){b.y++;  if(b.y>64)b.alive=false;}}

  if(now-bLastEMove>(uint32_t)bEMoveInterval){
    bLastEMove=now;
    int minX=128,maxX=0;
    for(auto &e:bEnemies) if(e.alive){minX=min(minX,e.x);maxX=max(maxX,e.x+B_EW);e.tick++;}
    bool flip=(bEDir==1&&maxX>=126)||(bEDir==-1&&minX<=2);
    for(auto &e:bEnemies) if(e.alive){if(flip)e.y+=4;else e.x+=bEDir*3;}
    if(flip) bEDir=-bEDir;
  }

  if(now-bLastEShoot>(uint32_t)(900-bWave*60)){
    bLastEShoot=now;
    int col=random(B_ECOLS);
    EnemyB *sh=nullptr;
    for(auto &e:bEnemies)
      if(e.alive&&(e.x-8)/B_EGAPX==col&&(!sh||e.y>sh->y)) sh=&e;
    if(sh) for(auto &b:bEBullets) if(!b.alive){b={sh->x+3,sh->y+B_EH,true};break;}
  }

  for(auto &b:bBullets) {
    if(!b.alive) continue;
    for(auto &e:bEnemies) {
      if(!e.alive) continue;
      if(b.x>=e.x&&b.x<=e.x+B_EW&&b.y>=e.y&&b.y<=e.y+B_EH){
        e.alive=false; b.alive=false;
        for(int p=0;p<4;p++) bSpawnParticle(e.x+3,e.y+2);
      }
    }
  }

  for(auto &b:bEBullets) {
    if(!b.alive) continue;
    if(b.x>=bPlayerX&&b.x<=bPlayerX+7&&b.y>=57&&b.y<=61){
      b.alive=false;
      for(int p=0;p<6;p++) bSpawnParticle(bPlayerX+3,58);
      if(--bLives<=0){ bGameOver=true; return; }
    }
  }

  for(auto &e:bEnemies)
    if(e.alive&&e.y+B_EH>=57){ bGameOver=true; return; }

  for(auto &p:bParts) if(p.life){p.x+=p.vx;p.y+=p.vy;p.life--;}

  bool any=false;
  for(auto &e:bEnemies) if(e.alive){any=true;break;}
  if(!any){bWave++;bSpawnWave();}

  display.clearDisplay();
  for(auto &e:bEnemies) if(e.alive) bDrawEnemy(e.x,e.y,e.type,e.tick);
  for(auto &b:bBullets)  if(b.alive) display.drawFastVLine(b.x,b.y,2,WHITE);
  for(auto &b:bEBullets) if(b.alive){display.drawPixel(b.x,b.y,WHITE);display.drawPixel(b.x,b.y+2,WHITE);}
  for(auto &p:bParts)    if(p.life)  display.drawPixel(p.x,p.y,WHITE);
  display.drawPixel(bPlayerX+3,57,WHITE);
  display.drawFastHLine(bPlayerX+2,58,3,WHITE);
  display.drawFastHLine(bPlayerX,59,7,WHITE);
  display.drawFastHLine(bPlayerX,60,7,WHITE);
  display.drawFastHLine(0,63,128,WHITE);
  display.setTextColor(WHITE); display.setTextSize(1);
  display.setCursor(50,2);
  for(int i=0;i<bLives;i++) display.print(F("*"));
  display.setCursor(90,2); display.print(F("W:")); display.print(bWave);
  display.display();
}