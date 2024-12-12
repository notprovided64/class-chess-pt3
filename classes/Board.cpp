#include "Board.h"

bool isWhite(char piece) {
  const char *wpieces = "?PNBRQK";

  for (int i = 0; wpieces[i] != '\0'; i++) {
    if (wpieces[i] == piece) {
      return true;
    }
  }

  return false;
}

bool isSlidingPiece(ChessPiece piece) {
  return piece == Bishop || piece == Rook || piece || Queen;
}

ChessPiece charToPiece(char piece) {
  const char *wpieces = "?PNBRQK";
  const char *bpieces = "?pnbrqk";

  for (int i = 0; wpieces[i] != '\0'; i++) {
    if (wpieces[i] == piece) {
      return (enum ChessPiece)i;
    }
  }

  for (int i = 0; bpieces[i] != '\0'; i++) {
    if (bpieces[i] == piece) {
      return (enum ChessPiece)i;
    }
  }

  return NoPiece;
}

bool Board::canCastle(bool kingside) {
  if (castleStatus == 0)
    return false;
  else if (isWhiteTurn) {
    if (castleStatus & CastleStatus::K && kingside)
      return true;
    if (castleStatus & CastleStatus::Q && !kingside)
      return true;
  } else {
    if (castleStatus & CastleStatus::k && kingside)
      return true;
    if (castleStatus & CastleStatus::q && !kingside)
      return true;
  }
  return false;
}

void Board::disableCastlability(bool is_white) {
  // fmkcl -- trueeeeeeee
  castleStatus &= is_white ? ~(K | Q) : ~(k | q);
}

void Board::disableCastlability(bool is_white, bool kingside) {
  if (is_white) {
    castleStatus ^= kingside ? K : Q;
  } else {
    castleStatus ^= kingside ? k : q;
  }
}

void Board::setEnpasSquare(int startSquare, bool isWhiteTurn) {
  int offset = isWhiteTurn ? 8 : -8;

  enPassantIndex = startSquare + offset;
}

void Board::handleEnpas(int endSquare, bool isWhiteTurn) {
  int offset, captureIndex;

  offset = isWhiteTurn ? -8 : 8;
  captureIndex = endSquare + offset;

  state[captureIndex] = '0';
}

void Board::handleCastling(int startSquare, int endSquare, bool isWhiteTurn) {
  int rookIndex, rookStartColumn, rookEndColumn;
  char rookChar;

  if (startSquare - endSquare == -2) { // kingside
    rookIndex = endSquare - 1;
    rookStartColumn = 7;
    rookEndColumn = 5;
  } else if (startSquare - endSquare == 2) { // queenside
    rookIndex = endSquare + 1;
    rookStartColumn = 0;
    rookEndColumn = 3;
  } else {
    return;
  }

  int row = isWhiteTurn ? 0 : 7;
  rookChar = isWhiteTurn ? 'R' : 'r';

  state[rookIndex] = rookChar;
  state[8 * row + rookStartColumn] = '0';
}

void Board::handlePromotion(int startSquare, int endSquare, bool isWhiteTurn) {
  int finalRow = isWhiteTurn ? 7 : 0;
  int squareRow = endSquare / 8;

  if (squareRow != finalRow)
    return;

  char queenChar = isWhiteTurn ? 'Q' : 'q';

  state[startSquare] = queenChar;
}

// this is where we handle castling and en passant
void Board::updateExtrinsicState(Move move) {

  // disable ability to castle once rook has been moved
  ChessPiece piece = charToPiece(state[move.StartSquare]);

  bool can_castle_kingside = canCastle(true);
  bool can_castle_queenside = canCastle(false);
  bool can_castle = can_castle_kingside || can_castle_queenside;

  if (can_castle) {
    if (piece == Rook) {
      if (can_castle_kingside && (move.StartSquare == (isWhiteTurn ? 7 : 63)))
        disableCastlability(isWhiteTurn, true);
      if (can_castle_queenside && (move.StartSquare == (isWhiteTurn ? 0 : 56)))
        disableCastlability(isWhiteTurn, false);
    }
    if (piece == King) {
      disableCastlability(isWhiteTurn);

      // if king moves 2 squares we castling
      if (std::abs(move.StartSquare - move.EndSquare) == 2) {
        handleCastling(move.StartSquare, move.EndSquare, isWhiteTurn);
      }
    }
  }

  if (piece == Pawn) {
    // if pawn moved two spaces (16 indices) we set enpassant, and then from
    // there if it didn't move 8 we know we captured something enpassant-wise
    if (std::abs(move.StartSquare - move.EndSquare) == 16) {
      setEnpasSquare(move.StartSquare, isWhiteTurn);
    } else if (std::abs(move.StartSquare - move.EndSquare) != 8) {
      // we captured something
      ChessPiece capture = charToPiece(state[move.EndSquare]);
      if (capture == NoPiece)
        handleEnpas(move.EndSquare, isWhiteTurn);
    }

    handlePromotion(move.StartSquare, move.EndSquare, isWhiteTurn);
  }
}

void Board::makeMove(Move move) {
  updateExtrinsicState(move);

  char piece = state[move.StartSquare];
  state[move.StartSquare] = '0';
  state[move.EndSquare] = piece;
  // does all that extra stuff not directly related to immediate piece movement
  isWhiteTurn = !isWhiteTurn;
}
