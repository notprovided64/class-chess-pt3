#pragma once
#include "Board.h"
#include "ChessSquare.h"
#include "Game.h"
#include <string>
#include <vector>

const int pieceSize = 64;

const int White = 0;
const int Black = 128;

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
  void updateGrid();

  void stopGame() override;
  BitHolder &getHolderAt(const int x, const int y) override {
    return _grid[y][x];
  }

  void updateAI() override;
  bool gameHasAI() override { return true; }

  std::string stateStringPretty();
  std::vector<Move> getCurrentMoves() { return _moves; }
  int getCastlingStatus() { return _board.castleStatus; }
  int getEnPassantIndex() { return _board.enPassantIndex; }

private:
  Bit *PieceForPlayer(const int playerNumber, ChessPiece piece);
  const char bitToPieceNotation(int row, int column) const;
  void setPieceAt(int rank, int file, int playerNumber, ChessPiece piece);
  void setGameFromFEN(const std::string &string);
  void setBoardFromFEN(const std::string &string);
  void generateMoves();
  void makeMove(Move move);

  ChessSquare _grid[8][8];
  Board _board;
  std::vector<Move> _moves;
};
