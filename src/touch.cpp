#include "touch.h"

static uint16_t median5(uint16_t* v) {
  for (int i=0;i<5;i++) for (int j=i+1;j<5;j++) if (v[j]<v[i]) { auto t=v[i];v[i]=v[j];v[j]=t; }
  return v[2];
}

void XPT2046Touch::begin() {
  pinMode(PIN_TOUCH_CS, OUTPUT);
  pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  load();
}

uint16_t XPT2046Touch::read12(uint8_t command) {
  spi_.transfer(command);
  uint16_t v = (uint16_t(spi_.transfer(0)) << 8) | spi_.transfer(0);
  return (v >> 3) & 0x0FFF;
}

bool XPT2046Touch::readRaw(uint16_t& x, uint16_t& y) {
  if (digitalRead(PIN_TOUCH_IRQ) != LOW) return false;

  uint16_t xs[5], ys[5];
  spi_.beginTransaction(SPISettings(TOUCH_SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_TFT_CS, HIGH);
  digitalWrite(PIN_TOUCH_CS, LOW);
  for (int i=0;i<5;i++) {
    xs[i] = read12(0xD0);
    ys[i] = read12(0x90);
  }
  digitalWrite(PIN_TOUCH_CS, HIGH);
  spi_.endTransaction();

  x=median5(xs); y=median5(ys);
  return true;
}

TouchPoint XPT2046Touch::read() {
  TouchPoint p;
  uint16_t rx, ry;
  if (!readRaw(rx,ry)) return p;
  p.pressed=true; p.rawX=rx; p.rawY=ry;
  if (calibrated_) {
    p.x = constrain(int(a_*rx + b_*ry + c_ + 0.5f), 0, SCREEN_W-1);
    p.y = constrain(int(d_*rx + e_*ry + f_ + 0.5f), 0, SCREEN_H-1);
  }
  return p;
}

void XPT2046Touch::load() {
  prefs_.begin("touch", true);
  calibrated_ = prefs_.getBool("ok", false);
  a_=prefs_.getFloat("a",0); b_=prefs_.getFloat("b",0); c_=prefs_.getFloat("c",0);
  d_=prefs_.getFloat("d",0); e_=prefs_.getFloat("e",0); f_=prefs_.getFloat("f",0);
  prefs_.end();
}
void XPT2046Touch::save() {
  prefs_.begin("touch", false);
  prefs_.putBool("ok", true);
  prefs_.putFloat("a",a_); prefs_.putFloat("b",b_); prefs_.putFloat("c",c_);
  prefs_.putFloat("d",d_); prefs_.putFloat("e",e_); prefs_.putFloat("f",f_);
  prefs_.end();
}
void XPT2046Touch::clearCalibration() {
  prefs_.begin("touch", false); prefs_.clear(); prefs_.end();
  calibrated_=false;
}

static float det3(const float m[3][3]) {
  return m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])
       - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])
       + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
}
static bool solve3(const float A[3][3], const float Y[3], float out[3]) {
  float det=det3(A); if (fabs(det)<1e-6f) return false;
  for(int col=0;col<3;col++){
    float M[3][3];
    for(int r=0;r<3;r++)for(int c=0;c<3;c++)M[r][c]=(c==col)?Y[r]:A[r][c];
    out[col]=det3(M)/det;
  }
  return true;
}

bool XPT2046Touch::solveAffine(const float raw[3][2], const float scr[3][2]) {
  float A[3][3]={{raw[0][0],raw[0][1],1},{raw[1][0],raw[1][1],1},{raw[2][0],raw[2][1],1}};
  float X[3]={scr[0][0],scr[1][0],scr[2][0]}, Y[3]={scr[0][1],scr[1][1],scr[2][1]};
  float ox[3], oy[3];
  if(!solve3(A,X,ox)||!solve3(A,Y,oy)) return false;
  a_=ox[0]; b_=ox[1]; c_=ox[2]; d_=oy[0]; e_=oy[1]; f_=oy[2];
  return true;
}

static void drawTarget(ST7796Display& tft,int x,int y) {
  uint16_t bg=ST7796Display::rgb565(18,20,24), fg=ST7796Display::rgb565(255,176,32);
  tft.fillScreen(bg);
  tft.drawLine(x-18,y,x+18,y,fg);
  tft.drawLine(x,y-18,x,y+18,fg);
  tft.drawCircle(x,y,12,fg);
  tft.drawText(10,10,"Touch calibration",ST7796Display::rgb565(255,255,255),bg,2);
}

void XPT2046Touch::runCalibration(ST7796Display& tft) {
  const float scr[3][2]={{35,35},{SCREEN_W-35,35},{SCREEN_W-35,SCREEN_H-35}};
  float raw[3][2];
  for(int i=0;i<3;i++){
    drawTarget(tft,int(scr[i][0]),int(scr[i][1]));
    while(digitalRead(PIN_TOUCH_IRQ)!=LOW) delay(5);
    uint32_t sx=0,sy=0,n=0;
    while(digitalRead(PIN_TOUCH_IRQ)==LOW){
      uint16_t x,y;
      if(readRaw(x,y)){sx+=x;sy+=y;n++;}
      delay(8);
    }
    if(n==0){i--;continue;}
    raw[i][0]=float(sx)/n; raw[i][1]=float(sy)/n;
    delay(150);
  }
  calibrated_=solveAffine(raw,scr);
  if(calibrated_) save();
  tft.fillScreen(ST7796Display::rgb565(18,20,24));
  tft.drawText(24,140, calibrated_ ? "Touch calibrated" : "Calibration failed",
               calibrated_?ST7796Display::rgb565(80,220,120):ST7796Display::rgb565(255,80,80),
               ST7796Display::rgb565(18,20,24),2);
  delay(900);
}
