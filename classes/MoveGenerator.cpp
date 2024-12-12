#include "Board.h"
#include <vector>

const int directional_offsets[] = {8, -8, -1, 1, 7, -7, 9, -9};

constexpr std::array<int, 8> compute_square_to_edge(int file, int rank) {
  int num_north = 7 - rank;
  int num_south = rank;
  int num_west = file;
  int num_east = 7 - file;

  return {num_north,
          num_south,
          num_west,
          num_east,
          std::min(num_north, num_west),
          std::min(num_south, num_east),
          std::min(num_north, num_east),
          std::min(num_south, num_west)};
}

constexpr auto setupsquares_to_edge() {
  std::array<std::array<int, 8>, 64> edges{};
  for (int file = 0; file < 8; ++file) {
    for (int rank = 0; rank < 8; ++rank) {
      int index = rank * 8 + file;
      edges[index] = compute_square_to_edge(file, rank);
    }
  }
  return edges;
}

constexpr auto squaresToEdge = setupsquares_to_edge();

// overall this class might not be needed, you could just handle all of this
// directly on the board

std::vector<Move> Board::GenerateLegalMoves() {
  std::vector<Move> legalMoves;
  auto pseudoLegalMoves = GenerateMoves();

  for (auto move : pseudoLegalMoves) {
    Board tempBoard = *this;
    tempBoard.makeMove(move);

    auto opponentMoves = tempBoard.GenerateMoves();

    bool illegal = false;
    for (auto opponentMove : opponentMoves) {
      if (charToPiece(tempBoard.state[opponentMove.EndSquare]) ==
          ChessPiece::King) {
        illegal = true;
        break;
      }
    }
    if (!illegal)
      legalMoves.push_back(move);
  }

  return legalMoves;
}

// questionable behaviour
std::vector<Move> operator+=(std::vector<Move> &a, const std::vector<Move> &b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}

// this should be refactored to remove dependance on the moves member variable
// (which should be removed)
std::vector<Move> Board::GenerateMoves() {
  std::vector<Move> moves;
  for (int i = 0; i < 64; i++) {
    if (!isFriendly(i))
      continue;

    ChessPiece piece = charToPiece(state[i]);
    switch (piece) {
    case NoPiece:
      continue;
    case Pawn:
      moves += GeneratePawnMoves(i);
      break;
    case Knight:
      moves += GenerateKnightMoves(i);
      break;
    case Bishop:
    case Rook:
    case Queen:
      moves += GenerateSlidingMoves(piece, i);
      break;
    case King:
      moves += GenerateKingMoves(i);
      break;
    default:
      break;
    }
  }

  return moves;
}

std::vector<Move> Board::GeneratePawnMoves(int index) {
  std::vector<Move> moves;

  int north_free = squaresToEdge[index][0];
  int south_free = squaresToEdge[index][1];
  int west_free = squaresToEdge[index][2];
  int east_free = squaresToEdge[index][3];

  int baseMovementOffset = isWhiteTurn ? 8 : -8;
  int extendedMovementOffset = isWhiteTurn ? 16 : -16;
  int captureOffsetWest = isWhiteTurn ? 7 : -9;
  int captureOffsetEast = isWhiteTurn ? 9 : -7;
  int freeMoveSquares = isWhiteTurn ? north_free : south_free;

  int startingRank = isWhiteTurn ? 1 : 6;
  bool inStartingRank = index / 8 == startingRank;

  if (freeMoveSquares < 1)
    return moves;

  int target_index = index + baseMovementOffset;
  if (isEmpty(target_index)) {
    moves.push_back(Move{index, target_index});
    if (inStartingRank && freeMoveSquares >= 2) {
      target_index = index + extendedMovementOffset;
      if (isEmpty(target_index)) {
        moves.push_back(Move{index, target_index});
      }
    }
  }

  if (west_free >= 1) {
    target_index = index + captureOffsetWest;
    if (isOpponent(target_index) || (target_index == enPassantIndex))
      moves.push_back(Move{index, target_index});
  }
  if (east_free >= 1) {
    target_index = index + captureOffsetEast;
    if (isOpponent(target_index) || (target_index == enPassantIndex))
      moves.push_back(Move{index, target_index});
  }

  return moves;
}

std::vector<Move> Board::GenerateSlidingMoves(ChessPiece piece, int index) {
  std::vector<Move> moves;

  int start_dir_index = (piece == Bishop) ? 4 : 0;
  int end_dir_index = (piece == Rook) ? 4 : 8;

  for (int dir_index = start_dir_index; dir_index < end_dir_index;
       dir_index++) {
    for (int n = 0; n < squaresToEdge[index][dir_index]; n++) {
      int target_index = index + directional_offsets[dir_index] * (n + 1);
      ChessPiece target_piece = charToPiece(state[target_index]);
      if (isFriendly(target_index)) {
        break;
      } else if (target_piece != NoPiece) {
        moves.push_back(Move{index, target_index});
        break;
      }

      moves.push_back(Move{index, target_index});
    }
  }

  return moves;
}

std::vector<Move> Board::GenerateKnightMoves(int index) {
  std::vector<Move> moves;

  int north_free = squaresToEdge[index][0];
  int south_free = squaresToEdge[index][1];
  int west_free = squaresToEdge[index][2];
  int east_free = squaresToEdge[index][3];

  auto tryAddMove = [&](int offset) {
    int target_index = index + offset;
    int target_piece = charToPiece(state[target_index]);
    if (!isFriendly(target_index))
      moves.push_back(Move{index, target_index});
  };

  if (south_free >= 2) {
    if (east_free >= 1) {
      tryAddMove(-15); // south-south-east
    }
    if (west_free >= 1) {
      tryAddMove(-17); // south-south-west
    }
  }
  if (north_free >= 2) {
    if (east_free >= 1) {
      tryAddMove(17); // north-north-east
    }
    if (west_free >= 1) {
      tryAddMove(15); // north-north-west
    }
  }
  if (east_free >= 2) {
    if (north_free >= 1) {
      tryAddMove(10); // east-east-north
    }
    if (south_free >= 1) {
      tryAddMove(-6); // east-east-south
    }
  }
  if (west_free >= 2) {
    if (north_free >= 1) {
      tryAddMove(6); // west-west-north
    }
    if (south_free >= 1) {
      tryAddMove(-10); // west-west-south
    }
  }

  return moves;
}

std::vector<Move> Board::GenerateKingMoves(int index) {
  std::vector<Move> moves;
  auto tryAddMove = [&](int offset) {
    int target_index = index + offset;
    if (!isFriendly(target_index)) {
      moves.push_back(Move{index, target_index});
    }
  };

  for (int dir = 0; dir < 8; ++dir) {
    if (squaresToEdge[index][dir] >= 1) {
      tryAddMove(directional_offsets[dir]);
    }
  }

  moves += GenerateCastlingMoves(index);
  return moves;
}

std::vector<Move> Board::GenerateCastlingMoves(int index) {
  std::vector<Move> moves;

  // TODO check pieceiswhite here
  auto tryAddMove = [&](bool isRight) {
    int limit = isRight ? squaresToEdge[index][3] : squaresToEdge[index][2];
    for (int i = 1; i <= limit; i++) {
      int target_index = isRight ? index + i : index - i;
      if (charToPiece(state[target_index]) == Rook)
        moves.push_back(Move{index, isRight ? index + 2 : index - 2});
      else if (!isEmpty(target_index))
        return;
    }
  };

  if (isWhiteTurn) {
    if (castleStatus & CastleStatus::K) {
      tryAddMove(true);
    }
    if (castleStatus & CastleStatus::Q) {
      tryAddMove(false);
    }
  } else {
    if (castleStatus & CastleStatus::k) {
      tryAddMove(true);
    }
    if (castleStatus & CastleStatus::q) {
      tryAddMove(false);
    }
  }

  return moves;
}

bool Board::isEmpty(int index) const {
  return charToPiece(state[index]) == NoPiece;
}

bool Board::isFriendly(int index) const {
  return charToPiece(state[index]) != NoPiece &&
         isWhite(state[index]) == isWhiteTurn;
}

bool Board::isOpponent(int index) const {
  return charToPiece(state[index]) != NoPiece &&
         isWhite(state[index]) != isWhiteTurn;
}
