#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class NetEventType : uint8_t {
  ACCOUNT, EVENT_JSON, START_GAME, GAME_JSON, MOVE_RESULT,
  PUZZLE_JSON, STATUS, ERROR
};

struct NetEvent {
  NetEventType type;
  int code=0;
  uint32_t elapsedMs=0;
  String text;
};

class LichessClient {
 public:
  void begin(QueueHandle_t queue);
  void login();
  void createAI(int level,int minutes,int increment);
  void seek(int minutes,int increment,bool rated);
  void cancelSeek();
  void startGameStream(const String& gameId);
  void stopGameStream();
  void move(const String& gameId,const String& uci);
  void resign(const String& gameId);
  void fetchPuzzle(const String& difficulty,const String& excludeId="");
  void finishPuzzleAndNext(const String& puzzleId,bool won,const String& difficulty);

 private:
  QueueHandle_t queue_=nullptr;
  volatile bool stopEvent_=false;
  volatile bool pauseEvent_=false;
  volatile bool stopGame_=false;
  volatile bool stopSeek_=false;
  volatile uint32_t gameGeneration_=0;
  volatile uint32_t seekGeneration_=0;
  String activeGameId_;

  enum class Op : uint8_t { LOGIN,AI,MOVE,RESIGN,PUZZLE,PUZZLE_NEXT };
  struct Job { LichessClient* self;Op op;String a,b;int x=0,y=0;bool flag=false; };
  void launch(Op op,const String& a="",const String& b="",int x=0,int y=0,bool flag=false);
  void emit(NetEventType type,const String& text,int code=0,uint32_t elapsed=0);
  static void jobTask(void* arg);
  static void eventTask(void* arg);
  static void gameTask(void* arg);
  static void seekTask(void* arg);
};
