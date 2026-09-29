#include "lichess_client.h"
#include "secrets.h"
#include "tls_root.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>

static const char* BASE="https://lichess.org";
static const char* UA="ESP32-C5-Lichess-Handheld/1.0";
static SemaphoreHandle_t tlsHandshakeMutex=nullptr;

class TlsHandshakeGuard {
 public:
  explicit TlsHandshakeGuard(uint32_t timeoutMs=30000):locked_(
      tlsHandshakeMutex && xSemaphoreTake(tlsHandshakeMutex,pdMS_TO_TICKS(timeoutMs))==pdTRUE){}
  ~TlsHandshakeGuard(){if(locked_)xSemaphoreGive(tlsHandshakeMutex);}
  bool locked() const{return locked_;}
 private:
  bool locked_;
};

class DecodedStream {
 public:
  DecodedStream(WiFiClient* s,bool chunked):s_(s),chunked_(chunked){}
  bool readLine(String& out,uint32_t timeoutMs=65000){
    out="";uint32_t last=millis();
    while(millis()-last<timeoutMs){
      int c=readChar();
      if(c<0){delay(2);continue;}
      last=millis();
      if(c=='\n')return true;
      if(c!='\r'&&out.length()<16384)out+=char(c);
    }
    return false;
  }
 private:
  WiFiClient* s_;bool chunked_;int32_t remaining_=0;bool consumeCrlf_=false;bool ended_=false;
  int readChar(){
    if(ended_||!s_)return -1;
    if(!chunked_){if(!s_->available())return -1;return s_->read();}
    if(remaining_<=0){
      if(consumeCrlf_){while(s_->available()){int c=s_->read();if(c=='\n')break;}consumeCrlf_=false;}
      if(!s_->available())return -1;
      String sizeLine=s_->readStringUntil('\n');sizeLine.trim();
      int sem=sizeLine.indexOf(';');if(sem>=0)sizeLine=sizeLine.substring(0,sem);
      remaining_=strtol(sizeLine.c_str(),nullptr,16);
      if(remaining_<=0){ended_=true;return -1;}
    }
    if(!s_->available())return -1;
    int c=s_->read();remaining_--;if(remaining_==0)consumeCrlf_=true;return c;
  }
};

static void prepareClient(WiFiClientSecure& c){
  // Follow the certificate chain currently served by Lichess/Cloudflare
  // instead of pinning one CA that can change when the CDN renews certificates.
  c.setCACert(LICHESS_ROOT_CA);
  c.setHandshakeTimeout(25);
  c.setTimeout(15);
}

static int requestOnce(const char* method,const String& path,const String& body,String& response,const char* contentType="application/x-www-form-urlencoded"){
  if(WiFi.status()!=WL_CONNECTED)return -1000;
  WiFiClientSecure client;prepareClient(client);
  HTTPClient http;http.setTimeout(20000);
  if(!http.begin(client,String(BASE)+path))return -1001;
  http.addHeader("Authorization",String("Bearer ")+LICHESS_TOKEN);
  http.addHeader("User-Agent",UA);http.addHeader("Accept","application/json");
  TlsHandshakeGuard handshake;
  if(!handshake.locked()){http.end();return -1002;}
  int code;
  if(strcmp(method,"GET")==0)code=http.GET();
  else{http.addHeader("Content-Type",contentType);code=http.sendRequest(method,(uint8_t*)body.c_str(),body.length());}
  if(code>0)response=http.getString();else response=http.errorToString(code);
  http.end();return code;
}

void LichessClient::emit(NetEventType type,const String& text,int code,uint32_t elapsed){
  if(!queue_)return;NetEvent* e=new NetEvent{type,code,elapsed,text};
  if(xQueueSend(queue_,&e,0)!=pdTRUE)delete e;
}

void LichessClient::begin(QueueHandle_t queue){
  queue_=queue;stopEvent_=false;authenticated_=false;
  if(!tlsHandshakeMutex)tlsHandshakeMutex=xSemaphoreCreateMutex();
  xTaskCreate(eventTask,"lichess-events",12288,this,1,nullptr);
}

void LichessClient::launch(Op op,const String& a,const String& b,int x,int y,bool flag){
  Job* j=new Job{this,op,a,b,x,y,flag};
  if(xTaskCreate(jobTask,"lichess-job",14336,j,1,nullptr)!=pdPASS){delete j;emit(NetEventType::ERROR,"Could not start network task");}
}
void LichessClient::login(){launch(Op::LOGIN);}
void LichessClient::createAI(int level,int minutes,int increment){launch(Op::AI,"","",level,minutes*60+increment*10000);}
void LichessClient::move(const String& gameId,const String& uci){launch(Op::MOVE,gameId,uci);}
void LichessClient::resign(const String& gameId){launch(Op::RESIGN,gameId);}
void LichessClient::fetchPuzzle(const String& difficulty,const String& excludeId){launch(Op::PUZZLE,difficulty,excludeId);}
void LichessClient::finishPuzzleAndNext(const String& puzzleId,bool won,const String& difficulty){launch(Op::PUZZLE_NEXT,puzzleId,difficulty,0,0,won);}

static bool choosePuzzle(const String& response,const String& exclude,String& chosen){
  JsonDocument doc;if(deserializeJson(doc,response))return false;
  JsonArray a=doc["puzzles"].as<JsonArray>();
  if(a.isNull()){chosen=response;return true;}
  for(JsonVariant v:a){String id=v["puzzle"]["id"]|"";if(id!=exclude){serializeJson(v,chosen);return true;}}
  if(a.size()){serializeJson(a[0],chosen);return true;}return false;
}

void LichessClient::jobTask(void* arg){
  Job* j=(Job*)arg;LichessClient* self=j->self;String body,resp,path;int code=0;uint32_t t0=millis();
  switch(j->op){
    case Op::LOGIN:
      self->authenticated_=false;
      for(int attempt=0;attempt<3;attempt++){
        resp="";code=requestOnce("GET","/api/account","",resp);
        Serial.printf("[Lichess] account attempt %d HTTP %d, response bytes=%u\n",attempt+1,code,unsigned(resp.length()));
        if(code==200||code==401||code==403)break;
        delay(1000*(attempt+1));
      }
      self->authenticated_=(code==200);
      self->emit(code==200?NetEventType::ACCOUNT:NetEventType::ERROR,resp,code,millis()-t0);break;
    case Op::AI:{
      int seconds=j->y%10000,inc=j->y/10000;
      body="level="+String(j->x)+"&clock.limit="+String(seconds)+"&clock.increment="+String(inc)+"&color=random&variant=standard";
      code=requestOnce("POST","/api/challenge/ai",body,resp);
      self->emit(code>=200&&code<300?NetEventType::START_GAME:NetEventType::ERROR,resp,code,millis()-t0);break;}
    case Op::MOVE:
      code=requestOnce("POST",String("/api/board/game/")+j->a+"/move/"+j->b,"",resp);
      self->emit(NetEventType::MOVE_RESULT,j->b+"\n"+resp,code,millis()-t0);break;
    case Op::RESIGN:
      code=requestOnce("POST",String("/api/board/game/")+j->a+"/resign","",resp);
      self->emit(NetEventType::STATUS,code==200?"Game resigned":String("Resign failed ")+code,code,millis()-t0);break;
    case Op::PUZZLE:
      path=String("/api/puzzle/batch/mix?difficulty=")+j->a+"&nb=3";
      code=requestOnce("GET",path,"",resp);
      if(code==200){String chosen;if(choosePuzzle(resp,j->b,chosen))self->emit(NetEventType::PUZZLE_JSON,chosen,code,millis()-t0);else self->emit(NetEventType::ERROR,"No puzzle returned",code);}
      else self->emit(NetEventType::ERROR,resp,code,millis()-t0);break;
    case Op::PUZZLE_NEXT:{
      body=String("{\"solutions\":[{\"id\":\"")+j->a+"\",\"win\":"+(j->flag?"true":"false")+",\"rated\":false}]}";
      requestOnce("POST","/api/puzzle/batch/mix?nb=0",body,resp,"application/json");
      path=String("/api/puzzle/batch/mix?difficulty=")+j->b+"&nb=3";resp="";
      code=requestOnce("GET",path,"",resp);
      if(code==200){String chosen;if(choosePuzzle(resp,j->a,chosen))self->emit(NetEventType::PUZZLE_JSON,chosen,code,millis()-t0);else self->emit(NetEventType::ERROR,"No next puzzle returned",code);}
      else self->emit(NetEventType::ERROR,resp,code,millis()-t0);break;}
  }
  delete j;vTaskDelete(nullptr);
}

static bool startStream(const String& path,const String& method,const String& body,WiFiClientSecure& client,HTTPClient& http,int& code,bool& chunked){
  code=-1001;chunked=false;
  prepareClient(client);http.setTimeout(65000);const char* keys[]={"Transfer-Encoding"};http.collectHeaders(keys,1);
  if(!http.begin(client,String(BASE)+path))return false;
  http.addHeader("Authorization",String("Bearer ")+LICHESS_TOKEN);http.addHeader("User-Agent",UA);http.addHeader("Accept","application/x-ndjson");
  if(method=="POST")http.addHeader("Content-Type","application/x-www-form-urlencoded");
  TlsHandshakeGuard handshake;
  if(!handshake.locked()){code=-1002;return false;}
  code=method=="POST"?http.sendRequest("POST",(uint8_t*)body.c_str(),body.length()):http.GET();
  chunked=http.header("Transfer-Encoding").indexOf("chunked")>=0;return code>=200&&code<300;
}

void LichessClient::eventTask(void* arg){
  auto* self=(LichessClient*)arg;
  while(!self->stopEvent_){
    if(self->pauseEvent_||!self->authenticated_){delay(100);continue;}
    if(WiFi.status()!=WL_CONNECTED){delay(1000);continue;}
    WiFiClientSecure client;HTTPClient http;int code;bool chunked;
    if(!startStream("/api/stream/event","GET","",client,http,code,chunked)){self->emit(NetEventType::ERROR,String("Event stream HTTP ")+code,code);http.end();delay(2000);continue;}
    DecodedStream rd(http.getStreamPtr(),chunked);String line;
    while(!self->stopEvent_&&!self->pauseEvent_&&(client.connected()||client.available()))if(rd.readLine(line)){if(line.startsWith("{"))self->emit(NetEventType::EVENT_JSON,line);line="";}
    http.end();if(!self->stopEvent_)delay(1200);
  }
  vTaskDelete(nullptr);
}

struct GameArgs{LichessClient* self;String gameId;uint32_t generation;};
void LichessClient::startGameStream(const String& gameId){
  stopGameStream();delay(20);activeGameId_=gameId;stopGame_=false;pauseEvent_=true;
  gameGeneration_=gameGeneration_+1;uint32_t generation=gameGeneration_;GameArgs* a=new GameArgs{this,gameId,generation};
  if(xTaskCreate(gameTask,"lichess-game",16384,a,1,nullptr)!=pdPASS){delete a;pauseEvent_=false;emit(NetEventType::ERROR,"Could not start game stream");}
}
void LichessClient::stopGameStream(){stopGame_=true;activeGameId_="";gameGeneration_=gameGeneration_+1;pauseEvent_=false;}
void LichessClient::gameTask(void* arg){
  GameArgs* a=(GameArgs*)arg;auto* self=a->self;String gid=a->gameId;uint32_t generation=a->generation;delete a;
  while(!self->stopGame_&&generation==self->gameGeneration_){
    WiFiClientSecure client;HTTPClient http;int code;bool chunked;
    if(!startStream(String("/api/board/game/stream/")+gid,"GET","",client,http,code,chunked)){self->emit(NetEventType::ERROR,String("Game stream HTTP ")+code,code);http.end();delay(1500);continue;}
    DecodedStream rd(http.getStreamPtr(),chunked);String line;
    while(!self->stopGame_&&generation==self->gameGeneration_&&(client.connected()||client.available()))if(rd.readLine(line)){if(generation==self->gameGeneration_&&line.startsWith("{"))self->emit(NetEventType::GAME_JSON,line);line="";}
    http.end();if(!self->stopGame_)delay(800);
  }
  vTaskDelete(nullptr);
}

struct SeekArgs{LichessClient* self;int minutes,inc;bool rated;uint32_t generation;};
void LichessClient::seek(int minutes,int increment,bool rated){
  cancelSeek();delay(20);stopSeek_=false;seekGeneration_=seekGeneration_+1;uint32_t generation=seekGeneration_;SeekArgs* a=new SeekArgs{this,minutes,increment,rated,generation};
  if(xTaskCreate(seekTask,"lichess-seek",12288,a,1,nullptr)!=pdPASS){delete a;emit(NetEventType::ERROR,"Could not start seek task");}
}
void LichessClient::cancelSeek(){stopSeek_=true;seekGeneration_=seekGeneration_+1;}
void LichessClient::seekTask(void* arg){
  SeekArgs* a=(SeekArgs*)arg;auto* self=a->self;uint32_t generation=a->generation;
  String body="time="+String(a->minutes)+"&increment="+String(a->inc)+"&rated="+(a->rated?"true":"false")+"&variant=standard";
  delete a;WiFiClientSecure client;HTTPClient http;int code;bool chunked;
  self->emit(NetEventType::STATUS,"Searching for an opponent...");
  if(!startStream("/api/board/seek","POST",body,client,http,code,chunked))self->emit(NetEventType::ERROR,String("Seek failed HTTP ")+code,code);
  else{
    DecodedStream rd(http.getStreamPtr(),chunked);String line;
    while(!self->stopSeek_&&generation==self->seekGeneration_&&(client.connected()||client.available()))rd.readLine(line,5000);
  }
  http.end();vTaskDelete(nullptr);
}
