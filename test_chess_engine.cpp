#include "chess_engine.h"
#include <cassert>
#include <iostream>

static void uci(ChessBoard& b, const char* move) {
  if (!b.applyUCI(String(move))) {
    std::cerr << "illegal test move: " << move << "\n";
    std::abort();
  }
}

static unsigned long long perft(const ChessBoard& b, int depth) {
  if (!depth) return 1;
  ChessMove moves[256];
  int count=b.generateLegal(moves,256);
  unsigned long long nodes=0;
  for(int i=0;i<count;i++){
    ChessBoard next=b;
    assert(next.apply(moves[i]));
    nodes+=perft(next,depth-1);
  }
  return nodes;
}

int main() {
  ChessBoard b;
  ChessMove legal[256];
  int openingMoves=b.generateLegal(legal, 256);
  assert(openingMoves == 20);
  assert(perft(b,2)==400);
  assert(perft(b,3)==8902);

  // Scholar's mate: UCI legality, SAN parsing and checkmate detection.
  uci(b, "e2e4"); uci(b, "e7e5"); uci(b, "f1c4"); uci(b, "b8c6");
  uci(b, "d1h5"); uci(b, "g8f6");
  ChessMove mate;
  assert(b.parseSAN(String("Qxf7#"), mate));
  assert(b.san(mate) == String("Qxf7#"));
  assert(b.apply(mate));
  assert(b.checkmate());

  // Both castling sides are generated and move the rook correctly.
  assert(b.loadFEN(String("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1")));
  ChessMove castle;
  assert(b.findLegal(4, 6, 0, castle));
  assert(b.apply(castle));
  assert(b.pieceAt(6) == 'K' && b.pieceAt(5) == 'R');
  assert(b.findLegal(60, 58, 0, castle));
  assert(b.apply(castle));
  assert(b.pieceAt(58) == 'k' && b.pieceAt(59) == 'r');

  // En passant.
  assert(b.loadFEN(String("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1")));
  ChessMove ep;
  assert(b.findLegal(36, 43, 0, ep));
  assert(ep.flags & MF_EN_PASSANT);
  assert(b.apply(ep));
  assert(b.pieceAt(43) == 'P' && b.pieceAt(35) == 0);

  // Promotion (default queen and explicit knight).
  assert(b.loadFEN(String("4k3/P7/8/8/8/8/8/4K3 w - - 0 1")));
  ChessMove prom;
  assert(b.findLegal(48, 56, 'q', prom));
  assert(b.apply(prom));
  assert(b.pieceAt(56) == 'Q');
  assert(b.loadFEN(String("4k3/P7/8/8/8/8/8/4K3 w - - 0 1")));
  assert(b.applyUCI(String("a7a8n")) && b.pieceAt(56) == 'N');

  // A pinned piece may not expose its king.
  assert(b.loadFEN(String("4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1")));
  ChessMove pinned;
  assert(!b.findLegal(12, 11, 0, pinned));

  // Lichess puzzle PGN setup: every SAN token parses, then the API's UCI
  // solution starts from the resulting position.
  b.reset();
  const char* puzzlePgn[]={
    "e4","e5","d4","Nc6","d5","Nb8","Nc3","d6","Be3","Be7","Qf3","Nf6",
    "h3","O-O","g4","a6","g5","Ne8","h4","Nd7","Bh3","g6","h5","Bxg5",
    "Bxg5","Qxg5","h6","Ndf6","Nge2","Nh5","Bxc8","Rxc8","Nd1","Qxh6","Ng3","Nef6"
  };
  for(const char* token:puzzlePgn){ChessMove move;assert(b.parseSAN(String(token),move));assert(b.apply(move));}
  ChessMove solution;
  assert(b.findLegal(ChessBoard::squareFromName('f','3'),ChessBoard::squareFromName('f','6'),0,solution));

  std::cout << "chess_engine_tests=PASS\n";
}
