#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "display.h"
#include "touch.h"
#include "wifi_manager.h"
#include "chess_engine.h"
#include "lichess_client.h"
#include "pieces_user.h"

SPIClass displaySPI(FSPI);
ST7796Display tft(displaySPI);
XPT2046Touch touch(displaySPI);
WiFiManagerLite wifiMgr;
LichessClient api;
QueueHandle_t netQueue=nullptr;

static const uint16_t C_BG=ST7796Display::rgb565(25,27,31);
static const uint16_t C_PANEL=ST7796Display::rgb565(37,39,44);
static const uint16_t C_BTN=ST7796Display::rgb565(65,69,78);
static const uint16_t C_ON=ST7796Display::rgb565(104,135,78);
static const uint16_t C_TEXT=ST7796Display::rgb565(245,245,245);
static const uint16_t C_MUTED=ST7796Display::rgb565(184,184,184);
static const uint16_t C_LIGHT=ST7796Display::rgb565(238,238,210);
static const uint16_t C_DARK=ST7796Display::rgb565(118,150,86);
static const uint16_t C_LAST=ST7796Display::rgb565(205,210,106);
static const uint16_t C_SEL=ST7796Display::rgb565(246,246,105);
static const uint16_t C_HINT=ST7796Display::rgb565(255,183,64);
static const uint16_t C_GOOD=ST7796Display::rgb565(102,187,106);
static const uint16_t C_BAD=ST7796Display::rgb565(239,83,80);

struct Rect{int x,y,w,h;};
static bool hit(const Rect&r,int x,int y){return x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;}
static String clip(const String&s,int n){return s.length()<=size_t(n)?s:s.substring(0,max(0,n-3))+"...";}

enum class Screen{HOME,ONLINE,AI,PUZZLE,ONLINE_CONFIRM,ONLINE_WAIT,GAME,PUZZLE_GAME,PUZZLE_ANSWER,NETWORKS,WIFI_PASSWORD};
Screen screen=Screen::HOME;
bool screenAsleep=false,apiStarted=false,loggedIn=false;
uint32_t lastActivityMs=0,lastTouchMs=0;
String statusText="Starting...",accountId,accountName;

int aiLevel=3,aiTimeIndex=1,onlineTimeIndex=0,puzzleDiffIndex=2;
bool onlineRated=false;
const int AI_TIMES[4][2]={{3,0},{5,3},{10,0},{15,10}};
const int ONLINE_TIMES[4][2]={{10,0},{10,5},{15,10},{30,0}};
const char* PUZZLE_DIFFS[5]={"easiest","easier","normal","harder","hardest"};

int networkPage=0,selectedNetwork=-1;String selectedSSID,wifiPassword;int keyboardMode=0;bool wifiError=false;
static const char* KEY_LOWER[]={"1234567890","qwertyuiop","asdfghjkl.","zxcvbnm_-@"};
static const char* KEY_UPPER[]={"1234567890","QWERTYUIOP","ASDFGHJKL.","ZXCVBNM_-@"};
static const char* KEY_SYMBOL[]={"!\"#$%&'()*","+,-./:;<=>","?@[\\]^_`{|"," }~1234567"};

ChessBoard board;
int selectedSq=-1;
ChessMove lastMove;
bool viewWhite=true;

struct Animation{bool active=false;char piece=0;int from=-1,to=-1;uint32_t start=0;};
Animation anim;
static constexpr uint32_t ANIM_MS=100;

String gameId,opponent="-",gameStatus="idle",pendingUci;
bool myWhite=true;
int serverMoveCount=0;
int64_t wtime=-1,btime=-1;int winc=0,binc=0;uint32_t clockSyncMs=0;
uint32_t movePostMs=0,moveStreamMs=0,pendingSentMs=0;
ChessBoard pendingBoard;ChessMove pendingLast;
int64_t pendingWtime=-1,pendingBtime=-1;uint32_t pendingClockSync=0;

String puzzleId,puzzleThemes;
int puzzleRating=0,puzzleSolutionCount=0,puzzleIndex=0,puzzlePlayedCount=0,puzzleHistoryPly=0;
String puzzleSolution[32],puzzlePlayed[64],puzzleAnswer[32];
ChessBoard puzzleStart;
bool puzzleWhite=true,puzzleLoading=false,puzzleReview=false,puzzleMistake=false,puzzleAnswerShown=false;
int puzzleHintLevel=0,puzzleHintFrom=-1,puzzleHintTo=-1;
bool puzzleReplyPending=false;uint32_t puzzleReplyDue=0;

static void redraw();

static void centerText(const Rect&r,const String&s,uint16_t fg,uint16_t bg,int scale=1){
  int tw=int(s.length())*6*scale,th=7*scale;tft.drawText(r.x+max(0,(r.w-tw)/2),r.y+max(0,(r.h-th)/2),s,fg,bg,scale);
}
static void button(const Rect&r,const String&s,bool on=false,bool enabled=true,int scale=1){
  uint16_t bg=!enabled?ST7796Display::rgb565(48,50,54):(on?C_ON:C_BTN);
  tft.fillRect(r.x,r.y,r.w,r.h,bg);tft.drawRect(r.x,r.y,r.w,r.h,C_PANEL);centerText(r,s,enabled?C_TEXT:C_MUTED,bg,scale);
}
static void header(const String&title){tft.fillScreen(C_BG);tft.drawText(18,15,title,C_TEXT,C_BG,2);button({390,12,72,30},"Home");}

static void ensureApi(){
  if(!apiStarted){netQueue=xQueueCreate(10,sizeof(NetEvent*));api.begin(netQueue);apiStarted=true;}
  if(WiFi.status()==WL_CONNECTED)api.login();
}

static int dispToSq(int dx,int dy){return viewWhite?dx+(7-dy)*8:(7-dx)+dy*8;}
static void sqToDisp(int sq,int&dx,int&dy){int f=sq&7,r=sq>>3;if(viewWhite){dx=f;dy=7-r;}else{dx=7-f;dy=r;}}

static void drawPiecePixel(int px,int py,char p){
  if(!p)return;
#if USER_PIECES_AVAILABLE
  tft.drawRGB565Masked(px,py,pixelsFor(p),maskFor(p),USER_PIECE_W,USER_PIECE_H);
#else
  uint16_t c=ChessBoard::isWhite(p)?C_TEXT:ST7796Display::rgb565(25,25,25);
  tft.fillCircle(px+18,py+18,12,c);tft.drawText(px+15,py+14,String(ChessBoard::typeOf(p)),C_BAD,c,1);
#endif
}

static void drawBoard(){
  for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++){
    int sq=dispToSq(dx,dy);uint16_t c=((dx+dy)&1)?C_DARK:C_LIGHT;
    if(lastMove.valid()&&(sq==lastMove.from||sq==lastMove.to))c=C_LAST;
    if(sq==selectedSq)c=C_SEL;
    tft.fillRect(dx*40,dy*40,40,40,c);
    if(puzzleHintLevel&&sq==puzzleHintFrom)tft.drawRect(dx*40+2,dy*40+2,36,36,C_HINT);
    if(!(anim.active&&sq==anim.to))drawPiecePixel(dx*40+2,dy*40+2,board.pieceAt(sq));
  }
  if(puzzleHintLevel>=2&&puzzleHintFrom>=0&&puzzleHintTo>=0){
    int x0,y0,x1,y1;sqToDisp(puzzleHintFrom,x0,y0);sqToDisp(puzzleHintTo,x1,y1);
    x0=x0*40+20;y0=y0*40+20;x1=x1*40+20;y1=y1*40+20;
    tft.drawLine(x0,y0,x1,y1,C_HINT);tft.fillCircle(x1,y1,4,C_HINT);
  }
}

static void startAnimation(const ChessBoard&before,const ChessMove&m){
  anim.active=true;anim.from=m.from;anim.to=m.to;anim.piece=before.pieceAt(m.from);anim.start=millis();drawBoard();
}
static void updateAnimation(){
  if(!anim.active)return;uint32_t e=millis()-anim.start;
  if(e>=ANIM_MS){anim.active=false;drawBoard();return;}
  float t=float(e)/ANIM_MS;t=t*t*(3-2*t);int fx,fy,tx,ty;sqToDisp(anim.from,fx,fy);sqToDisp(anim.to,tx,ty);
  drawBoard();int px=int((fx+(tx-fx)*t)*40)+2,py=int((fy+(ty-fy)*t)*40)+2;drawPiecePixel(px,py,anim.piece);
}

static String fmtClock(int64_t base,bool white){
  if(base<0)return "--:--";int64_t ms=base;
  if(board.whiteToMove()==white)ms=max<int64_t>(0,ms-int64_t(millis()-clockSyncMs));
  int sec=ms/1000;char b[16];if(ms<20000)snprintf(b,sizeof(b),"%02d:%02d.%d",sec/60,sec%60,int((ms%1000)/100));
  else snprintf(b,sizeof(b),"%02d:%02d",sec/60,sec%60);return String(b);
}

static void drawHome(){
  tft.fillScreen(C_BG);tft.drawText(18,16,"Chess Handheld",C_TEXT,C_BG,2);tft.drawText(18,48,"ESP32-C5 - Lichess",C_MUTED,C_BG,1);
  Rect nr{348,14,114,32};tft.fillRect(nr.x,nr.y,nr.w,nr.h,C_PANEL);
  uint16_t nc=loggedIn?C_GOOD:(WiFi.status()==WL_CONNECTED?C_HINT:C_BAD);tft.fillCircle(360,30,5,nc);
  tft.drawText(370,22,loggedIn?"Online":(WiFi.status()==WL_CONNECTED?"Wi-Fi":"Offline"),C_TEXT,C_PANEL,1);
  const Rect cards[3]={{18,88,140,110},{170,88,140,110},{322,88,140,110}};const char* a[]={"Online","AI","Puzzle"};const char*b[]={"Random player","Level 1-8","Lichess API"};
  for(int i=0;i<3;i++){tft.fillRect(cards[i].x,cards[i].y,cards[i].w,cards[i].h,C_PANEL);tft.drawText(cards[i].x+12,cards[i].y+18,a[i],C_TEXT,C_PANEL,2);tft.drawText(cards[i].x+12,cards[i].y+49,b[i],C_MUTED,C_PANEL,1);}
  button({322,216,140,34},"Network");tft.drawText(18,280,clip(statusText,73),C_MUTED,C_BG,1);
  tft.drawText(18,300,clip(String("Account: ")+(accountName.length()?accountName:"offline"),73),C_MUTED,C_BG,1);
}

static void drawSelector(Screen which){
  header(which==Screen::AI?"AI game":(which==Screen::ONLINE?"Online match":"Puzzle"));
  if(which==Screen::AI){
    tft.drawText(18,58,"AI strength",C_MUTED,C_BG,1);for(int i=1;i<=8;i++)button({18+(i-1)*55,82,48,34},String(i),aiLevel==i);
    tft.drawText(18,138,"Time control",C_MUTED,C_BG,1);for(int i=0;i<4;i++)button({18+i*108,164,96,36},String(AI_TIMES[i][0])+"+"+AI_TIMES[i][1],aiTimeIndex==i);
    button({18,224,444,44},"Start AI game",true,loggedIn);
  }else if(which==Screen::ONLINE){
    tft.drawText(18,58,"Board API random seek",C_MUTED,C_BG,1);for(int i=0;i<4;i++)button({18+i*108,92,96,36},String(ONLINE_TIMES[i][0])+"+"+ONLINE_TIMES[i][1],onlineTimeIndex==i);
    button({18,151,210,38},"Casual",!onlineRated);button({252,151,210,38},"Rated",onlineRated);button({18,218,444,44},"Review & Continue",true,loggedIn);
  }else{
    tft.drawText(18,58,"Difficulty relative to your rating",C_MUTED,C_BG,1);for(int i=0;i<5;i++)button({18+i*89,90,82,36},PUZZLE_DIFFS[i],puzzleDiffIndex==i);
    button({18,160,444,44},"Get next Lichess puzzle",true,WiFi.status()==WL_CONNECTED);tft.drawText(18,225,"Hints, answer and played-history review",C_MUTED,C_BG,1);
  }
  tft.drawText(18,292,clip(statusText,73),C_MUTED,C_BG,1);
}

static void drawOnlineConfirm(){
  tft.fillScreen(C_BG);tft.drawText(18,15,"Confirm online match",C_TEXT,C_BG,2);tft.drawText(18,50,"Check these settings before matchmaking.",C_MUTED,C_BG,1);
  tft.fillRect(18,80,444,144,C_PANEL);tft.drawText(36,98,"Time control",C_MUTED,C_PANEL,1);tft.drawText(36,119,String(ONLINE_TIMES[onlineTimeIndex][0])+"+"+ONLINE_TIMES[onlineTimeIndex][1],C_TEXT,C_PANEL,2);
  tft.drawText(220,98,"Game type",C_MUTED,C_PANEL,1);tft.drawText(220,119,onlineRated?"Rated":"Casual",C_TEXT,C_PANEL,2);tft.drawText(36,166,"Network",C_MUTED,C_PANEL,1);tft.drawText(36,187,loggedIn?"Connected":"Not connected",loggedIn?C_GOOD:C_BAD,C_PANEL,2);
  tft.drawText(220,177,clip(String("Account: ")+accountName,37),C_TEXT,C_PANEL,1);button({18,250,210,44},"Back");button({252,250,210,44},"Confirm & Search",true,loggedIn);
}
static void drawOnlineWait(){tft.fillScreen(C_BG);tft.drawText(18,28,"Finding opponent...",C_TEXT,C_BG,2);tft.drawText(18,74,String(ONLINE_TIMES[onlineTimeIndex][0])+"+"+ONLINE_TIMES[onlineTimeIndex][1]+(onlineRated?" - Rated":" - Casual"),C_TEXT,C_BG,2);tft.drawText(18,112,"Waiting for Lichess Board API",C_MUTED,C_BG,1);tft.drawText(18,150,clip(statusText,73),C_MUTED,C_BG,1);button({18,232,444,46},"Cancel search");}

static void drawGameSide(){
  tft.fillRect(320,0,160,320,C_PANEL);int64_t wt=wtime,bt=btime;String top=myWhite?opponent:accountName,bot=myWhite?accountName:opponent;
  tft.drawText(330,10,"Online game",C_TEXT,C_PANEL,2);tft.drawText(330,48,clip(top,23),C_MUTED,C_PANEL,1);tft.drawText(330,68,myWhite?fmtClock(bt,false):fmtClock(wt,true),C_TEXT,C_PANEL,2);
  tft.drawText(330,126,clip(bot,23),C_MUTED,C_PANEL,1);tft.drawText(330,146,myWhite?fmtClock(wt,true):fmtClock(bt,false),C_TEXT,C_PANEL,2);
  tft.drawText(330,184,String("API ")+(movePostMs?String(movePostMs)+"ms":"-"),C_MUTED,C_PANEL,1);tft.drawText(330,199,String("Sync ")+(moveStreamMs?String(moveStreamMs)+"ms":"-"),C_MUTED,C_PANEL,1);
  button({330,226,140,32},"Home");button({330,264,140,32},"Resign");tft.drawText(330,308,clip(statusText,24),C_MUTED,C_PANEL,1);
}
static void drawPuzzleSide(){
  tft.fillRect(320,0,160,320,C_PANEL);tft.drawText(330,10,"Puzzle",C_TEXT,C_PANEL,2);tft.drawText(330,45,clip(String("#")+puzzleId,23),C_MUTED,C_PANEL,1);tft.drawText(330,66,String("Rating ")+puzzleRating,C_TEXT,C_PANEL,2);tft.drawText(330,92,puzzleWhite?"White to move":"Black to move",C_TEXT,C_PANEL,1);
  tft.drawText(330,116,clip(puzzleThemes,23),C_MUTED,C_PANEL,1);tft.drawText(330,181,String(puzzleReview?"History ":"Played ")+(puzzleReview?puzzleHistoryPly:puzzlePlayedCount)+"/"+puzzlePlayedCount,C_TEXT,C_PANEL,1);
  if(puzzleReview){button({330,210,66,28},"< Prev",false,puzzleHistoryPly>0);button({404,210,66,28},"Next >",false,puzzleHistoryPly<puzzlePlayedCount);button({330,244,66,28},"Resume");button({404,244,66,28},"Answer");}
  else{button({330,210,66,28},puzzleHintLevel==0?"Hint":(puzzleHintLevel==1?"Hint 2":"Arrow"));button({404,210,66,28},"Answer");button({330,244,66,28},"< Prev",false,puzzlePlayedCount>0);button({404,244,66,28},"Next >",false,false);}
  button({330,278,66,28},"Home");button({404,278,66,28},"Next Puz",false,!puzzleLoading);tft.drawText(330,308,clip(statusText,24),C_MUTED,C_PANEL,1);
}
static void drawGame(){drawBoard();if(screen==Screen::GAME)drawGameSide();else drawPuzzleSide();}

static void drawPuzzleAnswer(){
  tft.fillScreen(C_BG);tft.drawText(18,14,"Puzzle answer",C_TEXT,C_BG,2);tft.drawText(18,47,clip(String("#")+puzzleId+" - rating "+puzzleRating,72),C_MUTED,C_BG,1);tft.drawText(18,78,"Full solution steps",C_TEXT,C_BG,2);
  for(int i=0;i<puzzleSolutionCount&&i<12;i++){int col=i<6?0:1,row=i<6?i:i-6;tft.drawText(18+col*225,112+row*25,String(i+1)+". "+clip(puzzleAnswer[i],31),C_TEXT,C_BG,1);}
  button({18,276,210,32},"Back to board");button({252,276,210,32},"Next puzzle",true);
}

static void signalBars(int x,int y,int rssi,uint16_t bg){int n=rssi>=-55?4:(rssi>=-67?3:(rssi>=-78?2:1));for(int i=0;i<4;i++)tft.fillRect(x+i*5,y+(3-i)*3,3,(i+1)*3,i<n?C_GOOD:bg);}
static void drawNetworks(){
  tft.fillScreen(C_BG);tft.drawText(18,14,"Choose Wi-Fi",C_TEXT,C_BG,2);button({390,12,72,30},"Home");
Wi-Fi credentials are configured locally and are not stored in this repository.
  int count=wifiMgr.scanCount(),start=networkPage*5;for(int row=0;row<5;row++){int i=start+row;Rect r{18,72+row*38,444,34};if(i<count){tft.fillRect(r.x,r.y,r.w,r.h,C_PANEL);signalBars(r.x+12,r.y+10,wifiMgr.scanRSSI(i),C_PANEL);tft.drawText(r.x+42,r.y+10,clip(wifiMgr.scanSSID(i)+(wifiMgr.scanSecure(i)?" [lock]":" [open]"),48),C_TEXT,C_PANEL,1);tft.drawText(r.x+390,r.y+10,String(wifiMgr.scanRSSI(i))+"dBm",C_MUTED,C_PANEL,1);}else if(row==0&&count==0)tft.drawText(30,r.y+10,"No networks found. Tap Refresh.",C_MUTED,C_BG,1);}
  button({18,274,112,34},"Refresh",true);button({142,274,72,34},"< Prev",false,networkPage>0);button({226,274,72,34},"Next >",false,(networkPage+1)*5<count);button({310,274,152,34},"Saved retry");
}
static void drawPassword(){
  tft.fillScreen(C_BG);tft.drawText(14,9,"Wi-Fi password",C_TEXT,C_BG,2);tft.drawText(14,34,clip(selectedSSID,68),C_MUTED,C_BG,1);uint16_t f=wifiError?C_BAD:C_PANEL;tft.fillRect(14,51,452,31,f);String m;for(int i=0;i<min(54,int(wifiPassword.length()));i++)m+='*';tft.drawText(22,62,m,C_TEXT,f,1);
  const char**rows=keyboardMode==0?KEY_LOWER:(keyboardMode==1?KEY_UPPER:KEY_SYMBOL);for(int r=0;r<4;r++)for(int c=0;c<10;c++){String s=rows[r][c]==' '?"SP":String(rows[r][c]);button({14+c*45,88+r*39,41,34},s);}button({14,249,82,57},"Cancel");button({101,249,82,57},keyboardMode==0?"ABC":(keyboardMode==1?"SYM":"abc"),keyboardMode!=0);button({188,249,82,57},"Delete");button({275,249,191,57},"Connect",true);
}

static void redraw(){
  if(screenAsleep)return;
  switch(screen){
    case Screen::HOME:drawHome();break;case Screen::ONLINE:case Screen::AI:case Screen::PUZZLE:drawSelector(screen);break;
    case Screen::ONLINE_CONFIRM:drawOnlineConfirm();break;case Screen::ONLINE_WAIT:drawOnlineWait();break;case Screen::GAME:case Screen::PUZZLE_GAME:drawGame();break;case Screen::PUZZLE_ANSWER:drawPuzzleAnswer();break;case Screen::NETWORKS:drawNetworks();break;case Screen::WIFI_PASSWORD:drawPassword();break;
  }
}

static void sleepScreen(){if(screenAsleep)return;tft.sleep();digitalWrite(PIN_TFT_BL,LOW);screenAsleep=true;}
static void wakeScreen(){if(screenAsleep){tft.wake();digitalWrite(PIN_TFT_BL,HIGH);screenAsleep=false;redraw();}lastActivityMs=millis();}
static void refreshNetworks(){screen=Screen::NETWORKS;networkPage=0;tft.fillScreen(C_BG);tft.drawText(18,24,"Scanning Wi-Fi...",C_TEXT,C_BG,2);wifiMgr.scan();redraw();}
static void connectSelected(){
  tft.fillScreen(C_BG);tft.drawText(18,90,"Connecting to",C_MUTED,C_BG,1);tft.drawText(18,116,clip(selectedSSID,36),C_TEXT,C_BG,2);
  if(wifiMgr.connectAndSave(selectedSSID,wifiPassword)){statusText=String("Connected to ")+selectedSSID;screen=Screen::HOME;wifiPassword="";wifiError=false;ensureApi();}
  else{statusText="Connection failed";wifiError=true;screen=wifiMgr.scanSecure(selectedNetwork)?Screen::WIFI_PASSWORD:Screen::NETWORKS;}redraw();
}

static bool replayPGN(const String&pgn,int wanted,ChessBoard&out){
  out.reset();String tok;bool headerSkip=false,comment=false;int variation=0,done=0;
  auto process=[&](String s)->bool{
    s.trim();if(!s.length()||s[0]=='$'||s=="1-0"||s=="0-1"||s=="1/2-1/2"||s=="*")return true;
    int dot=-1;for(int i=0;i<int(s.length());i++)if(s[i]=='.')dot=i;if(dot>=0){s=s.substring(dot+1);if(!s.length())return true;}
    ChessMove m;if(!out.parseSAN(s,m)||!out.apply(m))return false;done++;return true;
  };
  for(size_t i=0;i<=pgn.length()&&done<wanted;i++){
    char c=i<pgn.length()?pgn[i]:' ';
    if(headerSkip){if(c==']')headerSkip=false;continue;}if(comment){if(c=='}')comment=false;continue;}
    if(c=='['&&tok.length()==0){headerSkip=true;continue;}if(c=='{'){comment=true;continue;}if(c=='('){variation++;continue;}if(c==')'){variation=max(0,variation-1);continue;}if(variation)continue;
    if(isspace((unsigned char)c)){if(tok.length()){if(!process(tok))return false;tok="";}}else tok+=c;
  }
  return done==wanted;
}

static void rebuildPuzzle(int ply){board=puzzleStart;lastMove={};for(int i=0;i<ply;i++){ChessMove m;if(board.applyUCI(puzzlePlayed[i],&m))lastMove=m;}selectedSq=-1;}
static void resetHint(){puzzleHintLevel=0;puzzleHintFrom=puzzleHintTo=-1;}
static bool loadPuzzle(const String&json){
  JsonDocument doc;if(deserializeJson(doc,json)){statusText="Puzzle JSON parse failed";return false;}JsonObject p=doc["puzzle"],g=doc["game"];
  puzzleId=String((const char*)(p["id"]|""));puzzleRating=p["rating"]|0;puzzleThemes="";for(JsonVariant v:p["themes"].as<JsonArray>()){if(puzzleThemes.length())puzzleThemes+=",";puzzleThemes+=String((const char*)v);if(puzzleThemes.length()>40)break;}
  puzzleSolutionCount=0;for(JsonVariant v:p["solution"].as<JsonArray>())if(puzzleSolutionCount<32)puzzleSolution[puzzleSolutionCount++]=String((const char*)v);
  bool ok=false;const char*fen=p["fen"]|nullptr;if(fen&&*fen)ok=board.loadFEN(String(fen));else ok=replayPGN(String((const char*)(g["pgn"]|"")),int(p["initialPly"]|0)+1,board);
  if(!ok||!puzzleSolutionCount){statusText="Puzzle position parse failed";return false;}ChessMove first;if(!board.findLegal(ChessBoard::squareFromName(puzzleSolution[0][0],puzzleSolution[0][1]),ChessBoard::squareFromName(puzzleSolution[0][2],puzzleSolution[0][3]),puzzleSolution[0].length()>4?puzzleSolution[0][4]:0,first)){statusText="Puzzle position mismatch";return false;}
  puzzleStart=board;puzzleWhite=board.whiteToMove();viewWhite=puzzleWhite;puzzleIndex=0;puzzlePlayedCount=0;puzzleHistoryPly=0;puzzleReview=false;puzzleMistake=false;puzzleAnswerShown=false;puzzleLoading=false;puzzleReplyPending=false;selectedSq=-1;lastMove={};resetHint();
  ChessBoard a=board;for(int i=0;i<puzzleSolutionCount;i++){ChessMove m;if(!a.findLegal(ChessBoard::squareFromName(puzzleSolution[i][0],puzzleSolution[i][1]),ChessBoard::squareFromName(puzzleSolution[i][2],puzzleSolution[i][3]),puzzleSolution[i].length()>4?puzzleSolution[i][4]:0,m)){puzzleAnswer[i]="? "+puzzleSolution[i];break;}puzzleAnswer[i]=String(a.fullmoveNumber())+(a.whiteToMove()?". ":"... ")+a.san(m);a.apply(m);}
  screen=Screen::PUZZLE_GAME;statusText=String("Puzzle ")+puzzleId+" - "+(puzzleWhite?"White":"Black")+" to move";return true;
}

static void requestPuzzle(bool finish=false){if(puzzleLoading)return;puzzleLoading=true;statusText="Loading puzzle...";redraw();if(finish&&puzzleId.length())api.finishPuzzleAndNext(puzzleId,!puzzleMistake&&!puzzleAnswerShown&&puzzleIndex>=puzzleSolutionCount,PUZZLE_DIFFS[puzzleDiffIndex]);else api.fetchPuzzle(PUZZLE_DIFFS[puzzleDiffIndex],finish?puzzleId:"");}

static int countMoves(const String&s){int n=0;bool in=false;for(char c:s){if(c==' '){if(in){n++;in=false;}}else in=true;}if(in)n++;return n;}
static bool applyServerMoves(const String&moves){
  ChessBoard b;ChessMove lm;String tok,lastToken;int n=0;ChessBoard beforeLast;bool pendingConfirmed=false;
  for(size_t i=0;i<=moves.length();i++){char c=i<moves.length()?moves[i]:' ';if(c==' '){if(tok.length()){beforeLast=b;ChessMove m;if(!b.applyUCI(tok,&m))return false;if(pendingUci.length()&&n==serverMoveCount&&tok==pendingUci)pendingConfirmed=true;lm=m;lastToken=tok;n++;tok="";}}else tok+=c;}
  board=b;lastMove=lm;
  if(n>serverMoveCount&&(!pendingUci.length()||lastToken!=pendingUci))startAnimation(beforeLast,lm);
  if(pendingConfirmed){moveStreamMs=millis()-pendingSentMs;pendingUci="";}
  serverMoveCount=n;return true;
}

static void startGame(const String&id){
  gameId=id;board.reset();selectedSq=-1;lastMove={};viewWhite=true;myWhite=true;serverMoveCount=0;wtime=btime=-1;pendingUci="";opponent="-";screen=Screen::GAME;statusText="Opening game stream...";api.cancelSeek();api.startGameStream(id);redraw();
}

static void processGameJson(const String&json){
  JsonDocument doc;if(deserializeJson(doc,json))return;String type=String((const char*)(doc["type"]|""));JsonObject state;
  if(type=="gameFull"){
    JsonObject w=doc["white"],b=doc["black"];String wid=String((const char*)(w["id"]|"")),bid=String((const char*)(b["id"]|""));wid.toLowerCase();bid.toLowerCase();String aid=accountId;aid.toLowerCase();
    myWhite=aid.length()?aid==wid:w["aiLevel"].isNull();viewWhite=myWhite;JsonObject other=myWhite?b:w;const char* otherName=other["name"]|nullptr;if(!otherName||!*otherName)otherName=other["id"]|"Lichess AI";opponent=String(otherName);state=doc["state"];
  }else if(type=="gameState")state=doc.as<JsonObject>();else return;
  String moves=String((const char*)(state["moves"]|""));if(!applyServerMoves(moves)){statusText="Game move stream mismatch";return;}
  gameStatus=String((const char*)(state["status"]|"started"));wtime=state["wtime"]|-1;btime=state["btime"]|-1;winc=state["winc"]|0;binc=state["binc"]|0;clockSyncMs=millis();
  if(gameStatus!="started"){statusText=String("Game finished: ")+gameStatus;api.stopGameStream();}else statusText=board.whiteToMove()==myWhite?"Your move":"Opponent thinking";redraw();
}

static void processNetEvents(){
  if(!netQueue)return;NetEvent*e=nullptr;while(xQueueReceive(netQueue,&e,0)==pdTRUE){if(!e)continue;
    switch(e->type){
      case NetEventType::ACCOUNT:{JsonDocument d;if(!deserializeJson(d,e->text)){accountId=String((const char*)(d["id"]|""));accountName=String((const char*)(d["username"]|accountId.c_str()));loggedIn=true;statusText=String("Logged in: ")+accountName;}break;}
      case NetEventType::EVENT_JSON:{JsonDocument d;if(!deserializeJson(d,e->text)&&String((const char*)(d["type"]|""))=="gameStart")startGame(String((const char*)(d["game"]["id"]|"")));break;}
      case NetEventType::START_GAME:{JsonDocument d;if(!deserializeJson(d,e->text))startGame(String((const char*)(d["id"]|"")));else statusText="AI response parse failed";break;}
      case NetEventType::GAME_JSON:processGameJson(e->text);break;
      case NetEventType::MOVE_RESULT:{movePostMs=e->elapsedMs;int nl=e->text.indexOf('\n');String u=nl>=0?e->text.substring(0,nl):e->text;if(e->code<200||e->code>=300){board=pendingBoard;lastMove=pendingLast;wtime=pendingWtime;btime=pendingBtime;clockSyncMs=pendingClockSync;pendingUci="";statusText=String("Move rejected HTTP ")+e->code;redraw();}else statusText=String("Move sent - ")+movePostMs+"ms";break;}
      case NetEventType::PUZZLE_JSON:loadPuzzle(e->text);redraw();break;
      case NetEventType::STATUS:statusText=e->text;redraw();break;
      case NetEventType::ERROR:statusText=String("Network/API error ")+e->code+": "+clip(e->text,52);if(e->code==401||e->code==403)loggedIn=false;redraw();break;
    }delete e;e=nullptr;
  }
}

static void showHint(){
  if(puzzleReview){statusText="Resume before using Hint";return;}if(puzzleReplyPending||puzzleIndex>=puzzleSolutionCount){statusText="No hint available";return;}
  String u=puzzleSolution[puzzleIndex];int f=ChessBoard::squareFromName(u[0],u[1]),t=ChessBoard::squareFromName(u[2],u[3]);if(puzzleHintFrom!=f||puzzleHintTo!=t)puzzleHintLevel=0;puzzleHintFrom=f;puzzleHintTo=t;puzzleHintLevel=min(2,puzzleHintLevel+1);statusText=puzzleHintLevel==1?String("Hint: move piece on ")+ChessBoard::squareName(f):String("Hint: ")+ChessBoard::squareName(f)+" -> "+ChessBoard::squareName(t);redraw();
}

static void clickBoard(int x,int y){
  if(anim.active)return;if(screen==Screen::PUZZLE_GAME&&puzzleReview){statusText="History is read-only";drawPuzzleSide();return;}int sq=dispToSq(x/40,y/40);
  if(selectedSq<0){char p=board.pieceAt(sq);if(p&&ChessBoard::isWhite(p)==board.whiteToMove()&&(screen!=Screen::GAME||ChessBoard::isWhite(p)==myWhite)&&(screen!=Screen::PUZZLE_GAME||ChessBoard::isWhite(p)==puzzleWhite)){selectedSq=sq;drawBoard();}return;}
  if(sq==selectedSq){selectedSq=-1;drawBoard();return;}ChessMove m;if(!board.findLegal(selectedSq,sq,'q',m)){char p=board.pieceAt(sq);selectedSq=(p&&ChessBoard::isWhite(p)==board.whiteToMove())?sq:-1;drawBoard();return;}selectedSq=-1;
  if(screen==Screen::GAME){if(board.whiteToMove()!=myWhite||pendingUci.length())return;pendingBoard=board;pendingLast=lastMove;pendingWtime=wtime;pendingBtime=btime;pendingClockSync=clockSyncMs;pendingUci=board.uci(m);pendingSentMs=millis();ChessBoard before=board;if(wtime>=0&&btime>=0){uint32_t elapsed=millis()-clockSyncMs;if(board.whiteToMove())wtime=max<int64_t>(0,wtime-int64_t(elapsed))+winc;else btime=max<int64_t>(0,btime-int64_t(elapsed))+binc;clockSyncMs=millis();}board.apply(m);lastMove=m;startAnimation(before,m);statusText=String("Sending ")+pendingUci;api.move(gameId,pendingUci);drawGameSide();return;}
  if(puzzleReplyPending||puzzleIndex>=puzzleSolutionCount)return;String u=board.uci(m),expected=puzzleSolution[puzzleIndex];bool altMate=false;if(u!=expected&&puzzleThemes.indexOf("mateIn1")>=0){ChessBoard p=board;p.apply(m);altMate=p.checkmate();}
  if(u!=expected&&!altMate){puzzleMistake=true;resetHint();statusText="Not the puzzle move";drawPuzzleSide();return;}
  ChessBoard before=board;board.apply(m);lastMove=m;startAnimation(before,m);puzzlePlayed[puzzlePlayedCount++]=u;resetHint();puzzleIndex++;
  if(altMate){puzzleIndex=puzzleSolutionCount;statusText="Solved! Checkmate.";drawPuzzleSide();return;}
  if(puzzleIndex<puzzleSolutionCount){puzzleReplyPending=true;puzzleReplyDue=millis()+ANIM_MS+20;statusText="Correct...";}else statusText="Solved!";drawPuzzleSide();
}

static void puzzlePrev(){if(!puzzlePlayedCount){statusText="No previous move";return;}if(!puzzleReview){puzzleReview=true;puzzleHistoryPly=puzzlePlayedCount-1;}else puzzleHistoryPly=max(0,puzzleHistoryPly-1);rebuildPuzzle(puzzleHistoryPly);redraw();}
static void puzzleNext(){if(!puzzleReview||puzzleHistoryPly>=puzzlePlayedCount){statusText="No later played move";return;}puzzleHistoryPly++;rebuildPuzzle(puzzleHistoryPly);redraw();}
static void puzzleResume(){puzzleReview=false;puzzleHistoryPly=puzzlePlayedCount;rebuildPuzzle(puzzlePlayedCount);statusText="Returned to live puzzle";redraw();}

static void homeClick(int x,int y){
  if(hit({18,88,140,110},x,y)){screen=Screen::ONLINE;redraw();}
  else if(hit({170,88,140,110},x,y)){screen=Screen::AI;redraw();}
  else if(hit({322,88,140,110},x,y)){screen=Screen::PUZZLE;redraw();}
  else if(hit({322,216,140,34},x,y)||hit({348,14,114,32},x,y))refreshNetworks();
}
static void selectorClick(int x,int y){
  if(hit({390,12,72,30},x,y)){screen=Screen::HOME;redraw();return;}
  bool handled=false;
  if(screen==Screen::AI){for(int i=1;i<=8;i++)if(hit({18+(i-1)*55,82,48,34},x,y)){aiLevel=i;handled=true;break;}for(int i=0;i<4;i++)if(hit({18+i*108,164,96,36},x,y)){aiTimeIndex=i;handled=true;break;}if(hit({18,224,444,44},x,y)&&loggedIn){statusText="Starting Lichess AI...";api.createAI(aiLevel,AI_TIMES[aiTimeIndex][0],AI_TIMES[aiTimeIndex][1]);handled=true;}}
  else if(screen==Screen::ONLINE){for(int i=0;i<4;i++)if(hit({18+i*108,92,96,36},x,y)){onlineTimeIndex=i;handled=true;break;}if(hit({18,151,210,38},x,y)){onlineRated=false;handled=true;}if(hit({252,151,210,38},x,y)){onlineRated=true;handled=true;}if(hit({18,218,444,44},x,y)&&loggedIn){screen=Screen::ONLINE_CONFIRM;handled=true;}}
  else{for(int i=0;i<5;i++)if(hit({18+i*89,90,82,36},x,y)){puzzleDiffIndex=i;handled=true;break;}if(hit({18,160,444,44},x,y)&&WiFi.status()==WL_CONNECTED){requestPuzzle(false);return;}}
  if(handled)redraw();
}
static void networksClick(int x,int y){
  if(hit({390,12,72,30},x,y)){screen=Screen::HOME;redraw();return;}if(hit({18,274,112,34},x,y)){refreshNetworks();return;}if(hit({142,274,72,34},x,y)&&networkPage>0){networkPage--;redraw();return;}if(hit({226,274,72,34},x,y)&&(networkPage+1)*5<wifiMgr.scanCount()){networkPage++;redraw();return;}
  if(hit({310,274,152,34},x,y)){if(wifiMgr.autoConnect()){screen=Screen::HOME;ensureApi();redraw();}else refreshNetworks();return;}
  for(int r=0;r<5;r++){int i=networkPage*5+r;if(i<wifiMgr.scanCount()&&hit({18,72+r*38,444,34},x,y)){selectedNetwork=i;selectedSSID=wifiMgr.scanSSID(i);wifiPassword="";wifiError=false;if(wifiMgr.scanSecure(i)){screen=Screen::WIFI_PASSWORD;redraw();}else connectSelected();return;}}
}
static void passwordClick(int x,int y){const char**rows=keyboardMode==0?KEY_LOWER:(keyboardMode==1?KEY_UPPER:KEY_SYMBOL);for(int r=0;r<4;r++)for(int c=0;c<10;c++)if(hit({14+c*45,88+r*39,41,34},x,y)&&wifiPassword.length()<63){wifiPassword+=rows[r][c];wifiError=false;drawPassword();return;}if(hit({14,249,82,57},x,y)){screen=Screen::NETWORKS;redraw();}else if(hit({101,249,82,57},x,y)){keyboardMode=(keyboardMode+1)%3;drawPassword();}else if(hit({188,249,82,57},x,y)){if(wifiPassword.length())wifiPassword.remove(wifiPassword.length()-1);drawPassword();}else if(hit({275,249,191,57},x,y))connectSelected();}

static void handleTouch(int x,int y){lastActivityMs=millis();
  if(screen==Screen::HOME){homeClick(x,y);return;}if(screen==Screen::ONLINE||screen==Screen::AI||screen==Screen::PUZZLE){selectorClick(x,y);return;}if(screen==Screen::NETWORKS){networksClick(x,y);return;}if(screen==Screen::WIFI_PASSWORD){passwordClick(x,y);return;}
  if(screen==Screen::ONLINE_CONFIRM){if(hit({18,250,210,44},x,y)){screen=Screen::ONLINE;redraw();}else if(hit({252,250,210,44},x,y)&&loggedIn){screen=Screen::ONLINE_WAIT;api.seek(ONLINE_TIMES[onlineTimeIndex][0],ONLINE_TIMES[onlineTimeIndex][1],onlineRated);redraw();}return;}
  if(screen==Screen::ONLINE_WAIT){if(hit({18,232,444,46},x,y)){api.cancelSeek();screen=Screen::ONLINE;statusText="Search cancelled";redraw();}return;}
  if(screen==Screen::PUZZLE_ANSWER){if(hit({18,276,210,32},x,y)){screen=Screen::PUZZLE_GAME;redraw();}else if(hit({252,276,210,32},x,y))requestPuzzle(true);return;}
  if((screen==Screen::GAME||screen==Screen::PUZZLE_GAME)&&x<320){clickBoard(x,y);return;}
  if(screen==Screen::GAME){if(hit({330,226,140,32},x,y)){api.stopGameStream();screen=Screen::HOME;redraw();}else if(hit({330,264,140,32},x,y)){api.resign(gameId);redraw();}return;}
  if(screen==Screen::PUZZLE_GAME){if(puzzleReview){if(hit({330,210,66,28},x,y))puzzlePrev();else if(hit({404,210,66,28},x,y))puzzleNext();else if(hit({330,244,66,28},x,y))puzzleResume();else if(hit({404,244,66,28},x,y)){puzzleAnswerShown=true;screen=Screen::PUZZLE_ANSWER;redraw();}}else{if(hit({330,210,66,28},x,y))showHint();else if(hit({404,210,66,28},x,y)){puzzleAnswerShown=true;screen=Screen::PUZZLE_ANSWER;redraw();}else if(hit({330,244,66,28},x,y))puzzlePrev();}if(hit({330,278,66,28},x,y)){screen=Screen::HOME;redraw();}else if(hit({404,278,66,28},x,y))requestPuzzle(true);}
}

String serialLine;
static void serialCommand(String s){s.trim();if(s=="wifi scan")refreshNetworks();else if(s=="wifi reconnect"){if(wifiMgr.autoConnect()){ensureApi();screen=Screen::HOME;redraw();}else refreshNetworks();}else if(s=="wifi clear"){wifiMgr.clearExtra();refreshNetworks();}else if(s=="touch recalibrate"){wakeScreen();touch.clearCalibration();touch.runCalibration(tft);redraw();}else if(s=="screen sleep")sleepScreen();else if(s=="screen wake")wakeScreen();else if(s=="status")Serial.printf("screen=%d wifi=%s account=%s game=%s heap=%u\n",int(screen),wifiMgr.currentSSID().c_str(),accountName.c_str(),gameId.c_str(),ESP.getFreeHeap());else Serial.println("Commands: wifi scan | wifi reconnect | wifi clear | touch recalibrate | screen sleep | screen wake | status");}

void setup(){
  Serial.begin(115200);delay(500);Serial.println("\nESP32-C5 Lichess Handheld full firmware v1.0.2");pinMode(PIN_TFT_CS,OUTPUT);digitalWrite(PIN_TFT_CS,HIGH);pinMode(PIN_TFT_BL,OUTPUT);digitalWrite(PIN_TFT_BL,HIGH);pinMode(PIN_TOUCH_CS,OUTPUT);digitalWrite(PIN_TOUCH_CS,HIGH);displaySPI.begin(PIN_SPI_SCK,PIN_SPI_MISO,PIN_SPI_MOSI,-1);tft.begin();touch.begin();if(!touch.calibrated())touch.runCalibration(tft);
Wi-Fi credentials are configured locally and are not stored in this repository.
  if(connected){statusText=String("Connected to ")+WiFi.SSID();screen=Screen::HOME;ensureApi();redraw();}else{statusText="Choose a Wi-Fi network";refreshNetworks();}lastActivityMs=millis();
}

void loop(){
  processNetEvents();
  while(Serial.available()){char c=Serial.read();if(c=='\n'||c=='\r'){if(serialLine.length()){String cmd=serialLine;serialLine="";if(cmd!="screen sleep")wakeScreen();serialCommand(cmd);lastActivityMs=millis();}}else serialLine+=c;}
  if(screenAsleep){if(digitalRead(PIN_TOUCH_IRQ)==LOW){wakeScreen();while(digitalRead(PIN_TOUCH_IRQ)==LOW)delay(5);lastTouchMs=millis();}delay(5);return;}
  if(puzzleReplyPending&&millis()>=puzzleReplyDue){String u=puzzleSolution[puzzleIndex];ChessMove m;ChessBoard before=board;if(board.applyUCI(u,&m)){lastMove=m;puzzlePlayed[puzzlePlayedCount++]=u;puzzleIndex++;startAnimation(before,m);statusText=puzzleIndex>=puzzleSolutionCount?"Solved!":"Your move";}else statusText="Puzzle reply mismatch";puzzleReplyPending=false;drawPuzzleSide();}
  updateAnimation();
  if(millis()-lastTouchMs>120&&digitalRead(PIN_TOUCH_IRQ)==LOW){TouchPoint p=touch.read();if(p.pressed){Serial.printf("[Touch] raw=%u,%u screen=%d,%d page=%d\n",p.rawX,p.rawY,p.x,p.y,int(screen));handleTouch(p.x,p.y);while(digitalRead(PIN_TOUCH_IRQ)==LOW)delay(5);lastTouchMs=millis();}}
  if(SCREEN_IDLE_MS&&millis()-lastActivityMs>=SCREEN_IDLE_MS){sleepScreen();return;}
  static uint32_t lastClock=0;if(screen==Screen::GAME&&millis()-lastClock>500){lastClock=millis();drawGameSide();}
  delay(3);
}
