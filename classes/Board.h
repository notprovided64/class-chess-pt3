#pragma once
#include <string>

enum ChessPiece {
  NoPiece = 0,
  Pawn = 1,
  Knight = 2,
  Bishop = 3,
  Rook = 4,
  Queen = 5,
  King = 6,
};

enum CastleStatus {
  K = 1,
  Q = 2,
  k = 4,
  q = 8,
};

enum MoveFlag { None = 0, Promotion = 1, Castling, EnPassant };

struct Move {
  int StartSquare;
  int EndSquare;
  MoveFlag flag = None;

  bool operator==(const Move &other) const {
    return StartSquare == other.StartSquare && EndSquare == other.EndSquare;
  }
};

bool isWhite(char piece);
ChessPiece charToPiece(char piece);
bool isSlidingPiece(ChessPiece piece);

// contains underlying state of chess game
//
// hopefully this can be used to replace extrinsic state bs by just updating
// differences between _state here and the bit board
//
class Board {
private:
  std::vector<Move> GeneratePawnMoves(int index);
  std::vector<Move> GenerateSlidingMoves(ChessPiece piece, int index);
  std::vector<Move> GenerateKnightMoves(int index);
  std::vector<Move> GenerateKingMoves(int index);
  std::vector<Move> GenerateCastlingMoves(int index);

  bool isEmpty(int index) const;
  bool isFriendly(int index) const;
  bool isOpponent(int index) const;

  void updateExtrinsicState(Move move);
  bool canCastle(bool kingside);
  void disableCastlability(bool is_white, bool kingside);
  void disableCastlability(bool is_white);

  void handleCastling(int startSquare, int endSquare, bool isWhiteTurn);
  void setEnpasSquare(int startSquare, bool isWhiteTurn);
  void handleEnpas(int endSquare, bool isWhiteTurn);
  void handlePromotion(int startSquare, int endSquare, bool isWhiteTurn);

public:
  std::vector<Move> GenerateLegalMoves();
  std::vector<Move> GenerateMoves(); // make priv later

  void makeMove(Move move);

  std::string state;
  int castleStatus = 15;
  int enPassantIndex = 64;
  bool isWhiteTurn = true;
};
