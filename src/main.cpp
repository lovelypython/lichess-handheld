#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "display.h"
#include "touch.h"
#include "wifi_manager.h"
#include "pieces_user.h"

SPIClass displaySPI(FSPI);
ST7796Display tft(displaySPI);
XPT2046Touch touch(displaySPI);
WiFiManagerLite wifiMgr;

static const uint16_t C_BG    = ST7796Display::rgb565(28,30,34);
static const uint16_t C_PANEL = ST7796Display::rgb565(43,45,51);
static const uint16_t C_TEXT  = ST7796Display::rgb565(240,240,240);
static const uint16_t C_MUTED = ST7796Display::rgb565(170,174,180);
static const uint16_t C_LIGHT = ST7796Display::rgb565(238,238,210);
static const uint16_t C_DARK  = ST7796Display::rgb565(118,150,86);
static const uint16_t C_SEL   = ST7796Display::rgb565(246,246,105);
static const uint16_t C_GREEN = ST7796Display::rgb565(75,200,110);
static const uint16_t C_BLACK = ST7796Display::rgb565(25,25,25);
static const uint16_t C_WHITE = ST7796Display::rgb565(244,244,244);

char boardState[8][8] = {
  {'r','n','b','q','k','b','n','r'},
  {'p','p','p','p','p','p','p','p'},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {'P','P','P','P','P','P','P','P'},
  {'R','N','B','Q','K','B','N','R'}
};

int selectedX=-1, selectedY=-1;
bool animating=false;
char animPiece=0;
int ax0=0,ay0=0,ax1=0,ay1=0;
uint32_t animStart=0;
static constexpr uint32_t ANIM_MS=100;

void redrawAll();

bool screenAsleep=false;
uint32_t lastActivityMs=0;

void sleepScreen() {
  if (screenAsleep) return;
  tft.sleep();
  digitalWrite(PIN_TFT_BL, LOW);
  screenAsleep=true;
  Serial.println("[Display] sleep");
}

void wakeScreen() {
  if (!screenAsleep) {
    lastActivityMs=millis();
    return;
  }
  tft.wake();
  digitalWrite(PIN_TFT_BL, HIGH);
  screenAsleep=false;
  lastActivityMs=millis();
  redrawAll();
  Serial.println("[Display] wake");
}

void drawFallbackPiece(int cx,int cy,char p) {
  bool white = isupper(p);
  char u=toupper(p);
  uint16_t fill=white?C_WHITE:C_BLACK;
  uint16_t edge=white?C_BLACK:C_WHITE;

  if(u=='P'){
    tft.fillCircle(cx,cy-7,6,fill); tft.drawCircle(cx,cy-7,6,edge);
    tft.fillRect(cx-6,cy-1,12,12,fill); tft.drawRect(cx-6,cy-1,12,12,edge);
    tft.fillRect(cx-10,cy+11,20,4,fill);
  } else if(u=='R'){
    tft.fillRect(cx-10,cy-10,20,22,fill); tft.drawRect(cx-10,cy-10,20,22,edge);
    for(int i=-8;i<=8;i+=8)tft.fillRect(cx+i-2,cy-14,5,6,fill);
  } else if(u=='N'){
    tft.fillCircle(cx-2,cy-3,11,fill); tft.drawCircle(cx-2,cy-3,11,edge);
    tft.fillRect(cx-7,cy+4,16,11,fill);
    tft.fillRect(cx+4,cy-10,8,5,fill);
  } else if(u=='B'){
    tft.fillCircle(cx,cy-5,9,fill); tft.drawCircle(cx,cy-5,9,edge);
    tft.drawLine(cx-4,cy-11,cx+5,cy-2,edge);
    tft.fillRect(cx-7,cy+3,14,12,fill);
  } else if(u=='Q'){
    for(int i=-9;i<=9;i+=9)tft.fillCircle(cx+i,cy-11,3,fill);
    tft.fillRect(cx-10,cy-7,20,20,fill); tft.drawRect(cx-10,cy-7,20,20,edge);
  } else {
    tft.fillRect(cx-9,cy-8,18,22,fill); tft.drawRect(cx-9,cy-8,18,22,edge);
    tft.drawLine(cx,cy-18,cx,cy-8,fill); tft.drawLine(cx-5,cy-14,cx+5,cy-14,fill);
  }
}

#if USER_PIECES_AVAILABLE
const uint16_t* pixelsFor(char p);
const uint8_t* maskFor(char p);
#endif

void drawPieceAt(int x,int y,char p) {
  if(!p)return;
  int px=x*40+2, py=y*40+2;
#if USER_PIECES_AVAILABLE
  tft.drawRGB565Masked(px,py,pixelsFor(p),maskFor(p),PIECE_W,PIECE_H);
#else
  drawFallbackPiece(x*40+20,y*40+20,p);
#endif
}

void drawBoard() {
  for(int y=0;y<8;y++)for(int x=0;x<8;x++){
    uint16_t c=((x+y)&1)?C_DARK:C_LIGHT;
    if(x==selectedX&&y==selectedY)c=C_SEL;
    tft.fillRect(x*40,y*40,40,40,c);
    if(!(animating && x==ax1 && y==ay1)) drawPieceAt(x,y,boardState[y][x]);
  }
}

void drawSidebar() {
  tft.fillRect(320,0,160,320,C_PANEL);
  tft.drawText(330,12,"ESP32-C5",C_TEXT,C_PANEL,2);
  tft.drawText(330,42,"ST7796S 480x320",C_MUTED,C_PANEL,1);
  tft.drawText(330,65,"WiFi:",C_MUTED,C_PANEL,1);
  tft.drawText(330,78,wifiMgr.currentSSID(),WiFi.status()==WL_CONNECTED?C_GREEN:C_TEXT,C_PANEL,1);
  tft.drawText(330,96,wifiMgr.ip(),C_MUTED,C_PANEL,1);
#if USER_PIECES_AVAILABLE
  tft.drawText(330,122,"Pieces: user Neo",C_TEXT,C_PANEL,1);
#else
  tft.drawText(330,122,"Pieces: fallback",C_TEXT,C_PANEL,1);
#endif
  tft.drawText(330,145,"Tap board:",C_MUTED,C_PANEL,1);
  tft.drawText(330,158,"select -> move",C_MUTED,C_PANEL,1);
  tft.drawText(330,188,"100 ms animation",C_MUTED,C_PANEL,1);
  tft.drawText(330,216,"Serial commands:",C_MUTED,C_PANEL,1);
  tft.drawText(330,230,"wifi scan",C_TEXT,C_PANEL,1);
  tft.drawText(330,243,"wifi add S|P",C_TEXT,C_PANEL,1);
  tft.drawText(330,256,"wifi reconnect",C_TEXT,C_PANEL,1);
  tft.drawText(330,269,"touch recalibrate",C_TEXT,C_PANEL,1);
  tft.drawText(330,295,"HW bring-up v0.2",C_MUTED,C_PANEL,1);
}

void redrawAll(){drawBoard();drawSidebar();}

void startMove(int x0,int y0,int x1,int y1) {
  animating=true; animStart=millis();
  ax0=x0;ay0=y0;ax1=x1;ay1=y1;
  animPiece=boardState[y0][x0];
  boardState[y1][x1]=animPiece;
  boardState[y0][x0]=0;
  selectedX=selectedY=-1;
  drawBoard();
}

void updateAnimation(){
  if(!animating)return;
  uint32_t e=millis()-animStart;
  if(e>=ANIM_MS){animating=false;drawBoard();return;}
  float t=float(e)/ANIM_MS; t=t*t*(3-2*t);
  int cx=int((ax0+(ax1-ax0)*t)*40+20);
  int cy=int((ay0+(ay1-ay0)*t)*40+20);
  int dx=int((ax0+(ax1-ax0)*max(0.0f,t-0.15f))*40+20);
  int dy=int((ay0+(ay1-ay0)*max(0.0f,t-0.15f))*40+20);
  // Small redraw around the motion path is cheap enough for the first hardware test.
  drawBoard();
  drawFallbackPiece(cx,cy,animPiece);
}

String serialLine;

void handleSerialCommand(String s){
  s.trim();
  if(s=="wifi scan"){wifiMgr.scanToSerial();return;}
  if(s=="wifi reconnect"){
    bool ok=wifiMgr.autoConnect();
    Serial.println(ok?"WiFi connected":"No known WiFi available");
    drawSidebar();return;
  }
  if(s.startsWith("wifi add ")){
    String p=s.substring(9);
    int sep=p.indexOf('|');
    if(sep<1){Serial.println("Usage: wifi add SSID|PASSWORD");return;}
    wifiMgr.addOrReplaceExtra(p.substring(0,sep),p.substring(sep+1));
Wi-Fi credentials are configured locally and are not stored in this repository.
    return;
  }
  if(s=="wifi clear"){wifiMgr.clearExtra();Serial.println("Extra network cleared");return;}
  if(s=="touch recalibrate"){wakeScreen();touch.clearCalibration();touch.runCalibration(tft);redrawAll();lastActivityMs=millis();return;}
  if(s=="screen sleep"){sleepScreen();return;}
  if(s=="screen wake"){wakeScreen();return;}
  if(s=="status"){
    Serial.printf("SSID=%s IP=%s RSSI=%d touch=%s\n",
      wifiMgr.currentSSID().c_str(),wifiMgr.ip().c_str(),wifiMgr.rssi(),
      touch.calibrated()?"calibrated":"not calibrated");
    return;
  }
  Serial.println("Commands: wifi scan | wifi add SSID|PASSWORD | wifi reconnect | wifi clear | touch recalibrate | screen sleep | screen wake | status");
}

void setup(){
  Serial.begin(115200);
  delay(600);
  Serial.println("\nESP32-C5 Lichess Handheld - hardware bring-up v0.2");

  pinMode(PIN_TFT_CS,OUTPUT); digitalWrite(PIN_TFT_CS,HIGH);
  pinMode(PIN_TFT_BL,OUTPUT); digitalWrite(PIN_TFT_BL,HIGH);
  pinMode(PIN_TOUCH_CS,OUTPUT); digitalWrite(PIN_TOUCH_CS,HIGH);
  displaySPI.begin(PIN_SPI_SCK,PIN_SPI_MISO,PIN_SPI_MOSI,-1);

  tft.begin();
  touch.begin();

  tft.fillScreen(C_BG);
  tft.drawText(20,35,"ESP32-C5 Lichess Handheld",C_TEXT,C_BG,2);
  tft.drawText(20,70,"Display OK - connecting WiFi...",C_MUTED,C_BG,1);

  wifiMgr.begin();
  wifiMgr.autoConnect();

  if(!touch.calibrated()) touch.runCalibration(tft);

  redrawAll();
  lastActivityMs=millis();
  Serial.println("Ready. Type 'status' for diagnostics.");
}

uint32_t lastTouch=0;
void loop(){
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\n'||c=='\r'){
      if(serialLine.length()){
        // Any serial interaction wakes the UI first, except an explicit sleep request.
        String cmd=serialLine; cmd.trim();
        if(cmd!="screen sleep") wakeScreen();
        handleSerialCommand(serialLine);
        serialLine="";
        lastActivityMs=millis();
      }
    } else serialLine+=c;
  }

  // Touch controller stays powered while ST7796S + backlight are asleep.
  if(screenAsleep){
    if(digitalRead(PIN_TOUCH_IRQ)==LOW){
      wakeScreen();
      while(digitalRead(PIN_TOUCH_IRQ)==LOW) delay(5); // first tap only wakes
      lastTouch=millis();
    }
    delay(5);
    return;
  }

  updateAnimation();

  if(!animating && millis()-lastTouch>120){
    TouchPoint p=touch.read();
    if(p.pressed){
      lastActivityMs=millis();
      lastTouch=millis();
      if(p.x<320){
        int x=p.x/40,y=p.y/40;
        if(selectedX<0){
          if(boardState[y][x]){selectedX=x;selectedY=y;drawBoard();}
        }else{
          if(x==selectedX&&y==selectedY){selectedX=selectedY=-1;drawBoard();}
          else startMove(selectedX,selectedY,x,y);
        }
        while(digitalRead(PIN_TOUCH_IRQ)==LOW)delay(5);
      }
    }
  }

  if(SCREEN_IDLE_MS>0 && millis()-lastActivityMs>=SCREEN_IDLE_MS){
    sleepScreen();
    return;
  }

  static uint32_t lastWifiPaint=0;
  if(millis()-lastWifiPaint>3000){lastWifiPaint=millis();drawSidebar();}
  delay(2);
}
