#include "touch.h"

static constexpr uint8_t TOUCH_CAL_VERSION = 2;

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
  prefs_.begin("touch", false);
  calibrated_ = prefs_.getBool("ok", false) &&
                prefs_.getUChar("ver", 0) == TOUCH_CAL_VERSION;
  a_=prefs_.getFloat("a",0); b_=prefs_.getFloat("b",0); c_=prefs_.getFloat("c",0);
  d_=prefs_.getFloat("d",0); e_=prefs_.getFloat("e",0); f_=prefs_.getFloat("f",0);
  calibrated_ = calibrated_ && isfinite(a_) && isfinite(b_) && isfinite(c_) &&
                isfinite(d_) && isfinite(e_) && isfinite(f_);
  prefs_.end();
}
void XPT2046Touch::save() {
  prefs_.begin("touch", false);
  prefs_.putBool("ok", true);
  prefs_.putUChar("ver", TOUCH_CAL_VERSION);
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

bool XPT2046Touch::solveAffine(const float raw[][2], const float scr[][2], size_t count) {
  if(count<3)return false;
  float A[3][3]={{0,0,0},{0,0,0},{0,0,0}},X[3]={0,0,0},Y[3]={0,0,0};
  for(size_t i=0;i<count;i++){
    const float v[3]={raw[i][0],raw[i][1],1.0f};
    for(int r=0;r<3;r++){
      X[r]+=v[r]*scr[i][0];Y[r]+=v[r]*scr[i][1];
      for(int c=0;c<3;c++)A[r][c]+=v[r]*v[c];
    }
  }
  float ox[3], oy[3];
  if(!solve3(A,X,ox)||!solve3(A,Y,oy)) return false;
  a_=ox[0]; b_=ox[1]; c_=ox[2]; d_=oy[0]; e_=oy[1]; f_=oy[2];
  float maxError=0;
  for(size_t i=0;i<count;i++){
    float px=a_*raw[i][0]+b_*raw[i][1]+c_;
    float py=d_*raw[i][0]+e_*raw[i][1]+f_;
    float dx=px-scr[i][0],dy=py-scr[i][1];
    maxError=max(maxError,sqrtf(dx*dx+dy*dy));
  }
  return isfinite(maxError)&&maxError<=70.0f;
}

static void drawTarget(ST7796Display& tft,int x,int y) {
  uint16_t bg=ST7796Display::rgb565(18,20,24), fg=ST7796Display::rgb565(255,176,32);
  tft.fillScreen(bg);
  tft.drawLine(x-26,y,x+26,y,fg);
  tft.drawLine(x,y-26,x,y+26,fg);
  tft.drawCircle(x,y,24,fg);
  tft.drawCircle(x,y,16,fg);
  tft.drawText(10,10,"Touch calibration",ST7796Display::rgb565(255,255,255),bg,2);
  tft.drawText(10,294,"Tap anywhere inside the orange circle",ST7796Display::rgb565(210,210,210),bg,1);
}

void XPT2046Touch::runCalibration(ST7796Display& tft) {
  const float scr[5][2]={{48,48},{SCREEN_W-48,48},{SCREEN_W-48,SCREEN_H-48},
                         {48,SCREEN_H-48},{SCREEN_W/2.0f,SCREEN_H/2.0f}};
  float raw[5][2];
  for(int i=0;i<5;i++){
    drawTarget(tft,int(scr[i][0]),int(scr[i][1]));
    while(digitalRead(PIN_TOUCH_IRQ)!=LOW) delay(5);
    uint16_t xs[11],ys[11];int n=0;
    while(digitalRead(PIN_TOUCH_IRQ)==LOW&&n<11){
      uint16_t x,y;
      if(readRaw(x,y)){xs[n]=x;ys[n]=y;n++;}
      delay(5);
    }
    while(digitalRead(PIN_TOUCH_IRQ)==LOW)delay(3);
    if(n==0){i--;continue;}
    for(int a=0;a<n;a++)for(int b=a+1;b<n;b++)if(xs[b]<xs[a]){uint16_t t=xs[a];xs[a]=xs[b];xs[b]=t;}
    for(int a=0;a<n;a++)for(int b=a+1;b<n;b++)if(ys[b]<ys[a]){uint16_t t=ys[a];ys[a]=ys[b];ys[b]=t;}
    raw[i][0]=xs[n/2];raw[i][1]=ys[n/2];
    delay(100);
  }
  calibrated_=solveAffine(raw,scr,5);
  if(calibrated_) save();
  tft.fillScreen(ST7796Display::rgb565(18,20,24));
  tft.drawText(24,140, calibrated_ ? "Touch calibrated" : "Calibration failed",
               calibrated_?ST7796Display::rgb565(80,220,120):ST7796Display::rgb565(255,80,80),
               ST7796Display::rgb565(18,20,24),2);
  delay(1000);
}
