#include "Board.h"

// these correlate to values from ChessPiece
const int PIECE_VALUES[] = {
    0,    // NoPiece
    100,  // Pawn
    320,  // Knight
    330,  // Bishop
    500,  // Rook
    900,  // Queen
    20000 // King
};

// autoformat ruined both of these for me gg
const int PAWN_TABLE[64] = {
    0,  0,  0,  0,   0,   0,  0,  0,  50, 50, 50,  50, 50, 50,  50, 50,
    10, 10, 20, 30,  30,  20, 10, 10, 5,  5,  10,  25, 25, 10,  5,  5,
    0,  0,  0,  20,  20,  0,  0,  0,  5,  -5, -10, 0,  0,  -10, -5, 5,
    5,  10, 10, -20, -20, 10, 10, 5,  0,  0,  0,   0,  0,  0,   0,  0};

const int KNIGHT_TABLE[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,
    0,   -20, -40, -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,
    15,  20,  20,  15,  5,   -30, -30, 0,   15,  20,  20,  15,  0,
    -30, -30, 5,   10,  15,  15,  10,  5,   -30, -40, -20, 0,   5,
    5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};

const int INF = 99999;

int Board::evaluate(Board *board) {
  int score = 0;
  int multiplier = board->isWhiteTurn ? 1 : -1;

  for (int i = 0; i < 64; ++i) {
    char piece = board->state[i];
    if (piece == '0')
      continue;

    ChessPiece chessPiece = charToPiece(piece);
    bool isWhite = isupper(piece);
    int pieceValue = PIECE_VALUES[chessPiece];

    int positionBonus = 0;
    switch (chessPiece) {
    case Pawn:
      positionBonus = isWhite ? PAWN_TABLE[i] : PAWN_TABLE[63 - i];
      break;
    case Knight:
      positionBonus = isWhite ? KNIGHT_TABLE[i] : KNIGHT_TABLE[63 - i];
      break;
    default:
      break;
    }

    int colorMultiplier = isWhite ? 1 : -1;
    score += colorMultiplier * (pieceValue + positionBonus);
  }

  return score * multiplier;
}

int Board::negamax(Board *board, int depth, int alpha, int beta,
                   int playerColor) {
  std::vector<Move> legalMoves = board->GenerateLegalMoves();

  // base case: depth reached or no legal moves
  if (depth == 0 || legalMoves.empty()) {
    return board->evaluate(board);
  }

  int bestScore = -99999;

  for (const auto &move : legalMoves) {
    Board newBoard = *board;

    newBoard.makeMove(move);

    int score =
        -newBoard.negamax(&newBoard, depth - 1, -beta, -alpha, -playerColor);

    bestScore = std::max(bestScore, score);
    alpha = std::max(alpha, score);

    if (alpha >= beta)
      break;
  }

  return bestScore;
}

Move selectBestMove(Board *board, int depth) {
  std::vector<Move> legalMoves = board->GenerateLegalMoves();

  if (legalMoves.empty()) {
    return Move{-1, -1};
  }

  Move bestMove = legalMoves[0];
  int bestScore = -INF;
  int alpha = -INF;
  int beta = INF;

  for (const auto &move : legalMoves) {
    Board newBoard = *board;

    newBoard.makeMove(move);

    int score = -newBoard.negamax(&newBoard, depth - 1, -beta, -alpha,
                                  board->isWhiteTurn ? -1 : 1);

    if (score > bestScore) {
      bestScore = score;
      bestMove = move;
    }

    alpha = std::max(alpha, score);
  }

  return bestMove;
}
