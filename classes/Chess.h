#pragma once
#include "ChessSquare.h"
#include "Game.h"
#include <string>

const int pieceSize = 64;

const int White = 0;
const int Black = 128;

enum CastleStatus {
  K = 1,
  Q = 2,
  k = 4,
  q = 8,
};

enum ChessPiece {
  NoPiece = 0,
  Pawn = 1,
  Knight = 2,
  Bishop = 3,
  Rook = 4,
  Queen = 5,
  King = 6,
};

struct Move {
  int StartSquare;
  int EndSquare;

  bool operator==(const Move &other) const {
    return StartSquare == other.StartSquare && EndSquare == other.EndSquare;
  }
};

std::string movesToString(const std::vector<Move> &moves);

//
// the main game class
//
class Chess : public Game {
public:
  Chess();
  ~Chess();

  // set up the board
  void setUpBoard() override;

  Player *checkForWinner() override;
  bool checkForDraw() override;
  std::string initialStateString() override;
  std::string stateString() override;
  void setStateString(const std::string &s) override;
  bool actionForEmptyHolder(BitHolder &holder) override;
  bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
  bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
  void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

  void stopGame() override;
  BitHolder &getHolderAt(const int x, const int y) override {
    return _grid[y][x];
  }

  std::vector<Move> getMoves() { return _moves; };
  int getCS() { return _castle_status; };
  int getEnpasIndex() { return _enpas_index; };

  void updateAI() override;
  bool gameHasAI() override { return true; }

  std::string stateStringPretty();

private:
  Bit *PieceForPlayer(const int playerNumber, ChessPiece piece);
  const char bitToPieceNotation(int row, int column) const;
  void setPieceAt(int rank, int file, int playerNumber, ChessPiece piece);
  void setGameFromFEN(const std::string &string);
  void setBoardFromFEN(const std::string &string);
  void GenerateMoves(int playerNumber);
  void GenerateSlidingMoves(ChessPiece piece, int index);
  void GenerateKnightMoves(int index);
  void GeneratePawnMoves(int index);
  void GenerateKingMoves(int index);
  void GenerateCastlingMoves(int index);
  void updateExtrinsicState(Move move);
  bool canCastle(bool kingside);
  void disableCastlability(bool is_white, bool kingside);
  void disableCastlability(bool is_white);
  bool isFriendly(int index);
  bool isOpponent(int index);
  bool isEmpty(int index);
  void handleCastling(int startSquare, int endSquare, bool isWhiteTurn);
  void setEnpasSquare(int startSquare, bool isWhiteTurn);
  void handleEnpas(int endSquare, bool isWhiteTurn);
  void handlePromotion(int endSquare, bool isWhiteTurn);

  ChessSquare _grid[8][8];
  std::string _state;
  std::vector<Move> _moves;
  int _castle_status = 15;
  int _enpas_index = 64;
  bool _is_white_turn = true;
};
