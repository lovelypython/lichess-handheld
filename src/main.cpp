#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "config.h"
#include "display.h"
#include "touch.h"
#include "wifi_manager.h"

SPIClass displaySPI(FSPI);
ST7796Display tft(displaySPI);
XPT2046Touch touch(displaySPI);
WiFiManagerLite wifiMgr;

static const uint16_t C_BG    = ST7796Display::rgb565(25,27,31);
static const uint16_t C_PANEL = ST7796Display::rgb565(37,39,44);
static const uint16_t C_BTN   = ST7796Display::rgb565(65,69,78);
static const uint16_t C_ON    = ST7796Display::rgb565(104,135,78);
static const uint16_t C_TEXT  = ST7796Display::rgb565(245,245,245);
static const uint16_t C_MUTED = ST7796Display::rgb565(184,184,184);
static const uint16_t C_GOOD  = ST7796Display::rgb565(102,187,106);
static const uint16_t C_BAD   = ST7796Display::rgb565(239,83,80);

struct Rect { int x,y,w,h; };
static bool contains(const Rect& r,int x,int y) {
  return x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h;
}

enum class Screen { HOME, ONLINE, AI, PUZZLE, NETWORKS, WIFI_PASSWORD };
Screen screen = Screen::HOME;
bool screenAsleep = false;
uint32_t lastActivityMs = 0;
uint32_t lastTouchMs = 0;
String statusText = "Ready";

int aiLevel = 3;
int aiTime = 1;
int onlineTime = 0;
bool onlineRated = false;
int puzzleDifficulty = 2;
int networkPage = 0;
int selectedNetwork = -1;
String selectedSSID;
String wifiPassword;
bool keyboardUpper = false;
bool wifiError = false;

static String clipped(const String& s,int chars) {
  return s.length()<=size_t(chars)?s:s.substring(0,max(0,chars-3))+"...";
}

static void textCentered(const Rect& r,const String& label,uint16_t fg,uint16_t bg,int scale=1) {
  int tw=int(label.length())*6*scale, th=7*scale;
  tft.drawText(r.x+max(0,(r.w-tw)/2),r.y+max(0,(r.h-th)/2),label,fg,bg,scale);
}

static void button(const Rect& r,const String& label,bool on=false,bool enabled=true,int scale=1) {
  uint16_t bg=!enabled?ST7796Display::rgb565(48,50,54):(on?C_ON:C_BTN);
  tft.fillRect(r.x,r.y,r.w,r.h,bg);
  tft.drawRect(r.x,r.y,r.w,r.h,C_PANEL);
  textCentered(r,label,enabled?C_TEXT:C_MUTED,bg,scale);
}

static void drawHeader(const String& title) {
  tft.fillScreen(C_BG);
  tft.drawText(18,15,title,C_TEXT,C_BG,2);
  button({390,12,72,30},"Home");
}

static void drawNetworkPill() {
  bool online=WiFi.status()==WL_CONNECTED;
  Rect r{348,14,114,32};
  tft.fillRect(r.x,r.y,r.w,r.h,C_PANEL);
  tft.fillCircle(360,30,5,online?C_GOOD:C_BAD);
  tft.drawText(370,22,online?"Online":"Offline",C_TEXT,C_PANEL,1);
}

static void drawHome() {
  tft.fillScreen(C_BG);
  tft.drawText(18,16,"Chess Handheld",C_TEXT,C_BG,2);
  tft.drawText(18,48,"ESP32-C5 touchscreen",C_MUTED,C_BG,1);
  drawNetworkPill();
  const Rect cards[3]={{18,88,140,110},{170,88,140,110},{322,88,140,110}};
  const char* titles[3]={"Online","AI","Puzzle"};
  const char* subs[3]={"Random player","Level 1-8","Lichess API"};
  for(int i=0;i<3;i++){
    const Rect& r=cards[i];
    tft.fillRect(r.x,r.y,r.w,r.h,C_PANEL);
    tft.drawText(r.x+12,r.y+18,titles[i],C_TEXT,C_PANEL,2);
    tft.drawText(r.x+12,r.y+49,subs[i],C_MUTED,C_PANEL,1);
  }
  button({322,216,140,34},"Network");
  tft.drawText(18,280,clipped(statusText,73),C_MUTED,C_BG,1);
  tft.drawText(18,300,WiFi.status()==WL_CONNECTED?"Account: not linked":"Account: offline",C_MUTED,C_BG,1);
}

static void drawSelector(Screen which) {
  const char* title=which==Screen::AI?"AI game":(which==Screen::ONLINE?"Online match":"Puzzle");
  drawHeader(title);
  if(which==Screen::AI){
    tft.drawText(18,58,"AI strength",C_MUTED,C_BG,1);
    for(int i=1;i<=8;i++) button({18+(i-1)*55,82,48,34},String(i),aiLevel==i);
    tft.drawText(18,138,"Time control",C_MUTED,C_BG,1);
    const char* opts[]={"3+0","5+3","10+0","15+10"};
    for(int i=0;i<4;i++) button({18+i*108,164,96,36},opts[i],aiTime==i);
    button({18,224,444,44},"Start AI game",true);
  }else if(which==Screen::ONLINE){
    tft.drawText(18,58,"Board API random seek",C_MUTED,C_BG,1);
    const char* opts[]={"10+0","10+5","15+10","30+0"};
    for(int i=0;i<4;i++) button({18+i*108,92,96,36},opts[i],onlineTime==i);
    button({18,151,210,38},"Casual",!onlineRated);
    button({252,151,210,38},"Rated",onlineRated);
    button({18,218,444,44},"Review & Continue",true);
  }else{
    tft.drawText(18,58,"Difficulty relative to your rating",C_MUTED,C_BG,1);
    const char* opts[]={"easiest","easier","normal","harder","hardest"};
    for(int i=0;i<5;i++) button({18+i*89,90,82,36},opts[i],puzzleDifficulty==i);
    button({18,160,444,44},"Get next Lichess puzzle",true);
    tft.drawText(18,225,"Works anonymously when API is enabled.",C_MUTED,C_BG,1);
  }
  tft.drawText(18,292,clipped(statusText,73),C_MUTED,C_BG,1);
}

static void drawSignalBars(int x,int y,int rssi,uint16_t c,uint16_t bg) {
  int bars=rssi>=-55?4:(rssi>=-67?3:(rssi>=-78?2:1));
  for(int i=0;i<4;i++) tft.fillRect(x+i*5,y+(3-i)*3,3,(i+1)*3,i<bars?c:bg);
}

static void drawNetworks() {
  tft.fillScreen(C_BG);
  tft.drawText(18,14,"Choose Wi-Fi",C_TEXT,C_BG,2);
  button({390,12,72,30},"Home");
  if(WiFi.status()==WL_CONNECTED){
    tft.fillCircle(24,55,5,C_GOOD);
    tft.drawText(36,48,clipped(String("Connected: ")+WiFi.SSID(),45),C_GOOD,C_BG,1);
  }else{
    tft.fillCircle(24,55,5,C_BAD);
Wi-Fi credentials are configured locally and are not stored in this repository.
  }
  int count=wifiMgr.scanCount(),start=networkPage*5;
  for(int row=0;row<5;row++){
    int index=start+row;
    Rect r{18,72+row*38,444,34};
    if(index<count){
      tft.fillRect(r.x,r.y,r.w,r.h,C_PANEL);
      drawSignalBars(r.x+12,r.y+10,wifiMgr.scanRSSI(index),C_GOOD,C_PANEL);
      String lock=wifiMgr.scanSecure(index)?" [lock]":" [open]";
      tft.drawText(r.x+42,r.y+10,clipped(wifiMgr.scanSSID(index)+lock,48),C_TEXT,C_PANEL,1);
      tft.drawText(r.x+390,r.y+10,String(wifiMgr.scanRSSI(index))+"dBm",C_MUTED,C_PANEL,1);
    }else if(row==0&&count==0){
      tft.drawText(30,r.y+10,"No networks found. Tap Refresh.",C_MUTED,C_BG,1);
    }
  }
  button({18,274,112,34},"Refresh",true);
  button({142,274,72,34},"< Prev",false,networkPage>0);
  button({226,274,72,34},"Next >",false,(networkPage+1)*5<count);
  button({310,274,152,34},"Saved retry");
}

static const char* KEY_ROWS_LOWER[]={"1234567890","qwertyuiop","asdfghjkl.","zxcvbnm_-@"};
static const char* KEY_ROWS_UPPER[]={"1234567890","QWERTYUIOP","ASDFGHJKL.","ZXCVBNM_-@"};

static void drawPassword() {
  tft.fillScreen(C_BG);
  tft.drawText(14,9,"Wi-Fi password",C_TEXT,C_BG,2);
  tft.drawText(14,34,clipped(selectedSSID,68),C_MUTED,C_BG,1);
  uint16_t field=wifiError?C_BAD:C_PANEL;
  tft.fillRect(14,51,452,31,field);
  String masked;
  int shown=min(54,int(wifiPassword.length()));
  for(int i=0;i<shown;i++) masked+='*';
  tft.drawText(22,62,masked,C_TEXT,field,1);
  const char** rows=keyboardUpper?KEY_ROWS_UPPER:KEY_ROWS_LOWER;
  for(int row=0;row<4;row++) for(int col=0;col<10;col++){
    String label;label+=rows[row][col];
    button({14+col*45,88+row*39,41,34},label);
  }
  button({14,249,82,57},"Cancel");
  button({101,249,82,57},keyboardUpper?"lower":"SHIFT",keyboardUpper);
  button({188,249,82,57},"Delete");
  button({275,249,191,57},"Connect",true);
}

static void redraw() {
  switch(screen){
    case Screen::HOME: drawHome(); break;
    case Screen::ONLINE: case Screen::AI: case Screen::PUZZLE: drawSelector(screen); break;
    case Screen::NETWORKS: drawNetworks(); break;
    case Screen::WIFI_PASSWORD: drawPassword(); break;
  }
}

static void sleepScreen() {
  if(screenAsleep)return;
  tft.sleep();digitalWrite(PIN_TFT_BL,LOW);screenAsleep=true;
  Serial.println("[Display] sleep after 5 minutes idle");
}

static void wakeScreen() {
  if(screenAsleep){
    tft.wake();digitalWrite(PIN_TFT_BL,HIGH);screenAsleep=false;redraw();
    Serial.println("[Display] wake");
  }
  lastActivityMs=millis();
}

static void refreshNetworkList() {
  screen=Screen::NETWORKS;networkPage=0;
  tft.fillScreen(C_BG);tft.drawText(18,24,"Scanning Wi-Fi...",C_TEXT,C_BG,2);
  int count=wifiMgr.scan();
  statusText=count?String(count)+" Wi-Fi networks found":"No Wi-Fi networks found";
  redraw();
}

static void connectSelected(const String& password) {
  tft.fillScreen(C_BG);
  tft.drawText(18,90,"Connecting to",C_MUTED,C_BG,1);
  tft.drawText(18,116,clipped(selectedSSID,36),C_TEXT,C_BG,2);
  bool ok=wifiMgr.connectAndSave(selectedSSID,password);
  if(ok){
    statusText=String("Connected to ")+selectedSSID;screen=Screen::HOME;
    wifiError=false;wifiPassword="";
  }else{
    statusText="Connection failed. Check the password.";wifiError=true;
    screen=wifiMgr.scanSecure(selectedNetwork)?Screen::WIFI_PASSWORD:Screen::NETWORKS;
  }
  redraw();
}

static void homeClick(int x,int y) {
  if(contains({18,88,140,110},x,y))screen=Screen::ONLINE;
  else if(contains({170,88,140,110},x,y))screen=Screen::AI;
  else if(contains({322,88,140,110},x,y))screen=Screen::PUZZLE;
  else if(contains({322,216,140,34},x,y)||contains({348,14,114,32},x,y)){refreshNetworkList();return;}
  redraw();
}

static void selectorClick(int x,int y) {
  if(contains({390,12,72,30},x,y)){screen=Screen::HOME;redraw();return;}
  if(screen==Screen::AI){
    for(int i=1;i<=8;i++)if(contains({18+(i-1)*55,82,48,34},x,y))aiLevel=i;
    for(int i=0;i<4;i++)if(contains({18+i*108,164,96,36},x,y))aiTime=i;
    if(contains({18,224,444,44},x,y))statusText="AI game API is not linked yet.";
  }else if(screen==Screen::ONLINE){
    for(int i=0;i<4;i++)if(contains({18+i*108,92,96,36},x,y))onlineTime=i;
    if(contains({18,151,210,38},x,y))onlineRated=false;
    if(contains({252,151,210,38},x,y))onlineRated=true;
    if(contains({18,218,444,44},x,y))statusText="Link a Lichess token before matchmaking.";
  }else{
    for(int i=0;i<5;i++)if(contains({18+i*89,90,82,36},x,y))puzzleDifficulty=i;
    if(contains({18,160,444,44},x,y))statusText="Puzzle API port is not enabled yet.";
  }
  redraw();
}

static void networksClick(int x,int y) {
  if(contains({390,12,72,30},x,y)){screen=Screen::HOME;redraw();return;}
  if(contains({18,274,112,34},x,y)){refreshNetworkList();return;}
  if(contains({142,274,72,34},x,y)&&networkPage>0){networkPage--;redraw();return;}
  if(contains({226,274,72,34},x,y)&&(networkPage+1)*5<wifiMgr.scanCount()){networkPage++;redraw();return;}
  if(contains({310,274,152,34},x,y)){
Wi-Fi credentials are configured locally and are not stored in this repository.
    bool ok=wifiMgr.autoConnect();
Wi-Fi credentials are configured locally and are not stored in this repository.
    if(ok){screen=Screen::HOME;redraw();}else refreshNetworkList();
    return;
  }
  for(int row=0;row<5;row++){
    Rect r{18,72+row*38,444,34};int index=networkPage*5+row;
    if(index<wifiMgr.scanCount()&&contains(r,x,y)){
      selectedNetwork=index;selectedSSID=wifiMgr.scanSSID(index);
      wifiPassword="REPLACE_WITH_YOUR_WIFI_PASSWORD";wifiError=false;
      if(wifiMgr.scanSecure(index)){screen=Screen::WIFI_PASSWORD;redraw();}
      else connectSelected("");
      return;
    }
  }
}

static void passwordClick(int x,int y) {
  const char** rows=keyboardUpper?KEY_ROWS_UPPER:KEY_ROWS_LOWER;
  for(int row=0;row<4;row++)for(int col=0;col<10;col++){
    if(contains({14+col*45,88+row*39,41,34},x,y)&&wifiPassword.length()<63){
      wifiPassword+=rows[row][col];wifiError=false;drawPassword();return;
    }
  }
  if(contains({14,249,82,57},x,y)){screen=Screen::NETWORKS;redraw();return;}
  if(contains({101,249,82,57},x,y)){keyboardUpper=!keyboardUpper;drawPassword();return;}
  if(contains({188,249,82,57},x,y)){
    if(wifiPassword.length())wifiPassword.remove(wifiPassword.length()-1);
    wifiError=false;drawPassword();return;
  }
  if(contains({275,249,191,57},x,y))connectSelected(wifiPassword);
}

static void handleTouch(int x,int y) {
  lastActivityMs=millis();
  switch(screen){
    case Screen::HOME:homeClick(x,y);break;
    case Screen::ONLINE:case Screen::AI:case Screen::PUZZLE:selectorClick(x,y);break;
    case Screen::NETWORKS:networksClick(x,y);break;
    case Screen::WIFI_PASSWORD:passwordClick(x,y);break;
  }
}

String serialLine;
static void serialCommand(String s) {
  s.trim();
  if(s=="wifi scan"){refreshNetworkList();return;}
  if(s=="wifi reconnect"){
Wi-Fi credentials are configured locally and are not stored in this repository.
    if(ok){screen=Screen::HOME;redraw();}else refreshNetworkList();return;
  }
Wi-Fi credentials are configured locally and are not stored in this repository.
  if(s=="touch recalibrate"){wakeScreen();touch.clearCalibration();touch.runCalibration(tft);redraw();return;}
  if(s=="screen sleep"){sleepScreen();return;}
  if(s=="screen wake"){wakeScreen();return;}
  if(s=="status"){
    Serial.printf("screen=%d SSID=%s IP=%s RSSI=%d idle=%lu/%lu ms\n",int(screen),
      wifiMgr.currentSSID().c_str(),wifiMgr.ip().c_str(),wifiMgr.rssi(),
      millis()-lastActivityMs,SCREEN_IDLE_MS);return;
  }
  Serial.println("Commands: wifi scan | wifi reconnect | wifi clear | touch recalibrate | screen sleep | screen wake | status");
}

void setup(){
  Serial.begin(115200);delay(500);
  Serial.println("\nESP32-C5 Chess Handheld - UI/Wi-Fi parity v0.3");
  pinMode(PIN_TFT_CS,OUTPUT);digitalWrite(PIN_TFT_CS,HIGH);
  pinMode(PIN_TFT_BL,OUTPUT);digitalWrite(PIN_TFT_BL,HIGH);
  pinMode(PIN_TOUCH_CS,OUTPUT);digitalWrite(PIN_TOUCH_CS,HIGH);
  displaySPI.begin(PIN_SPI_SCK,PIN_SPI_MISO,PIN_SPI_MOSI,-1);
  tft.begin();touch.begin();
  if(!touch.calibrated())touch.runCalibration(tft);
  tft.fillScreen(C_BG);
  tft.drawText(18,84,"Chess Handheld",C_TEXT,C_BG,2);
Wi-Fi credentials are configured locally and are not stored in this repository.
  wifiMgr.begin();
  bool connected=wifiMgr.autoConnect();
  if(connected){statusText=String("Connected to ")+WiFi.SSID();screen=Screen::HOME;redraw();}
  else{statusText="Choose a Wi-Fi network";refreshNetworkList();}
  lastActivityMs=millis();
  Serial.println("Ready. Screen sleep timeout: 300 seconds.");
}

void loop(){
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\n'||c=='\r'){
      if(serialLine.length()){
        String cmd=serialLine;serialLine="";
        if(cmd!="screen sleep")wakeScreen();
        serialCommand(cmd);lastActivityMs=millis();
      }
    }else serialLine+=c;
  }
  if(screenAsleep){
    if(digitalRead(PIN_TOUCH_IRQ)==LOW){
      wakeScreen();while(digitalRead(PIN_TOUCH_IRQ)==LOW)delay(5);lastTouchMs=millis();
    }
    delay(5);return;
  }
  if(millis()-lastTouchMs>120&&digitalRead(PIN_TOUCH_IRQ)==LOW){
    TouchPoint p=touch.read();
    if(p.pressed){
      handleTouch(p.x,p.y);while(digitalRead(PIN_TOUCH_IRQ)==LOW)delay(5);lastTouchMs=millis();
    }
  }
  if(SCREEN_IDLE_MS>0&&millis()-lastActivityMs>=SCREEN_IDLE_MS){sleepScreen();return;}
  delay(3);
}
