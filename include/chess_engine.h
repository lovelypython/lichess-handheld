#pragma once
#include <Arduino.h>

enum ChessMoveFlags : uint8_t {
  MF_NONE=0, MF_CAPTURE=1, MF_EN_PASSANT=2, MF_CASTLE=4, MF_PROMOTION=8
};

struct ChessMove {
  int8_t from=-1;
  int8_t to=-1;
  char promotion=0;  // lower-case q/r/b/n
  uint8_t flags=MF_NONE;
  bool valid() const { return from>=0 && to>=0; }
};

class ChessBoard {
 public:
  ChessBoard(){ reset(); }
  void reset();
  bool loadFEN(const String& fen);
  char pieceAt(int sq) const { return sq>=0&&sq<64?cells_[sq]:0; }
  bool whiteToMove() const { return whiteToMove_; }
  int fullmoveNumber() const { return fullmove_; }
  int enPassantSquare() const { return epSquare_; }
  uint8_t castlingRights() const { return castle_; }

  int generateLegal(ChessMove* out,int maxMoves) const;
  bool isLegal(const ChessMove& m) const;
  bool findLegal(int from,int to,char promotion,ChessMove& out) const;
  bool apply(const ChessMove& m);
  bool applyUCI(const String& uci,ChessMove* applied=nullptr);
  bool parseSAN(const String& san,ChessMove& out) const;
  String san(const ChessMove& move) const;
  bool inCheck(bool white) const;
  bool checkmate() const;
  String uci(const ChessMove& m) const;

  static int squareFromName(char file,char rank);
  static String squareName(int sq);
  static bool isWhite(char p){return p>='A'&&p<='Z';}
  static char typeOf(char p){return p>='a'&&p<='z'?p-'a'+'A':p;}

 private:
  char cells_[64]{};
  bool whiteToMove_=true;
  uint8_t castle_=0;
  int8_t epSquare_=-1;
  uint16_t halfmove_=0;
  uint16_t fullmove_=1;

  int generatePseudo(ChessMove* out,int maxMoves) const;
  bool squareAttacked(int sq,bool byWhite) const;
  void applyUnchecked(const ChessMove& m);
  static void add(ChessMove* out,int& n,int maxMoves,int from,int to,char promo=0,uint8_t flags=0);
};
