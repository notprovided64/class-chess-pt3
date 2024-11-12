#include "Chess.h"
#include <array>
#include <cstdio>
#include <ctype.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

const int AI_PLAYER = 1;
const int HUMAN_PLAYER = -1;

std::string movesToString(const std::vector<Move> &moves) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < moves.size(); ++i) {
    oss << "Move(Start: " << moves[i].StartSquare
        << ", End: " << moves[i].EndSquare << ")";
    if (i != moves.size() - 1) {
      oss << ",\n";
    }
  }
  oss << "]";
  return oss.str();
}

Chess::Chess() {}

Chess::~Chess() {}

//
// make a chess piece for the player
//
Bit *Chess::PieceForPlayer(const int playerNumber, ChessPiece piece) {
  const char *pieces[] = {"pawn.png", "knight.png", "bishop.png",
                          "rook.png", "queen.png",  "king.png"};

  Bit *bit = new Bit();
  const char *pieceName = pieces[piece - 1];
  std::string spritePath =
      std::string("chess/") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
  bit->LoadTextureFromFile(spritePath.c_str());
  bit->setOwner(getPlayerAt(playerNumber));
  bit->setSize(pieceSize, pieceSize);

  return bit;
}

void Chess::setUpBoard() {
  setNumberOfPlayers(2);
  _gameOptions.rowX = 8;
  _gameOptions.rowY = 8;
  //
  // we want white to be at the bottom of the screen so we need to reverse the
  // board
  //
  char piece[2];
  piece[1] = 0;
  for (int y = 0; y < _gameOptions.rowY; y++) {
    for (int x = 0; x < _gameOptions.rowX; x++) {
      ImVec2 position((float)(pieceSize * x + pieceSize),
                      (float)(pieceSize * (_gameOptions.rowY - y) + pieceSize));
      _grid[y][x].initHolder(position, "boardsquare.png", x, y);
      _grid[y][x].setGameTag(0);
      piece[0] = bitToPieceNotation(y, x);
      _grid[y][x].setNotation(piece);
    }
  }

  setBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
  /*setBoardFromFEN("rnbqkbnr/8/8/8/8/8/8/RNBQKBNR");*/
  GenerateMoves(getCurrentPlayer()->playerNumber());
}

ChessPiece getPiece(int tag) { return ChessPiece(tag & 127); }
bool isSlidingPiece(ChessPiece piece) {
  return piece == Bishop || piece == Rook || piece || Queen;
}

int getPlayerNumber(int tag) { return tag < 128 ? 0 : 1; }

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

constexpr auto setup_squares_to_edge() {
  std::array<std::array<int, 8>, 64> edges{};
  for (int file = 0; file < 8; ++file) {
    for (int rank = 0; rank < 8; ++rank) {
      int index = rank * 8 + file;
      edges[index] = compute_square_to_edge(file, rank);
    }
  }
  return edges;
}

constexpr auto squares_to_edge = setup_squares_to_edge();

void Chess::GenerateMoves(int playerNumber) {
  _moves.clear();
  for (int i = 0; i < 64; i++) {
    Bit *bit = _grid[i / 8][i % 8].bit();
    if (!bit)
      continue;

    int tag = bit->gameTag();
    if (getCurrentPlayer()->playerNumber() != getPlayerNumber(tag))
      continue;

    ChessPiece piece = getPiece(tag);
    switch (piece) {
    case NoPiece:
      exit(1);
    case Pawn:
      GeneratePawnMoves(i, tag);
      break;
    case Knight:
      GenerateKnightMoves(i, tag);
      break;
    case Bishop:
    case Rook:
    case Queen:
      GenerateSlidingMoves(i, tag);
      break;
    case King:
      GenerateKingMoves(i, tag);
      break;
    default:
      break;
    }
  }
}
void Chess::GeneratePawnMoves(int start_index, int tag) {
  int north_free = squares_to_edge[start_index][0];
  int south_free = squares_to_edge[start_index][1];
  int west_free = squares_to_edge[start_index][2];
  int east_free = squares_to_edge[start_index][3];

  int playerNumber = getPlayerNumber(tag);
  int baseMovementOffset = playerNumber == 0 ? 8 : -8;
  int extendedMovementOffset = playerNumber == 0 ? 16 : -16;
  int captureOffsetWest = playerNumber == 0 ? 7 : -9;
  int captureOffsetEast = playerNumber == 0 ? 9 : -7;
  int freeMoveSquares = playerNumber == 0 ? north_free : south_free;

  int startingRank = playerNumber == 0 ? 1 : 6;
  bool inStartingRank = start_index / 8 == startingRank;

  if (freeMoveSquares < 1)
    return;

  int target_index = start_index + baseMovementOffset;
  Bit *target_bit = _grid[target_index / 8][target_index % 8].bit();
  if (!target_bit) {
    _moves.push_back(Move{start_index, target_index});
    if (inStartingRank && freeMoveSquares >= 2) {
      target_index = start_index + extendedMovementOffset;
      target_bit = _grid[target_index / 8][target_index % 8].bit();
      if (!target_bit) {
        _moves.push_back(Move{start_index, target_index});
      }
    }
  }

  if (west_free >= 1) {
    target_index = start_index + captureOffsetWest;
    target_bit = _grid[target_index / 8][target_index % 8].bit();
    if (target_bit &&
        getPlayerNumber(tag) ^ getPlayerNumber(target_bit->gameTag()))
      _moves.push_back(Move{start_index, target_index});
  }
  if (east_free >= 1) {
    target_index = start_index + captureOffsetEast;
    target_bit = _grid[target_index / 8][target_index % 8].bit();
    if (target_bit &&
        getPlayerNumber(tag) ^ getPlayerNumber(target_bit->gameTag()))
      _moves.push_back(Move{start_index, target_index});
  }
}

void Chess::GenerateSlidingMoves(int start_index, int tag) {
  int start_dir_index = (getPiece(tag) == Bishop) ? 4 : 0;
  int end_dir_index = (getPiece(tag) == Rook) ? 4 : 8;

  for (int dir_index = start_dir_index; dir_index < end_dir_index;
       dir_index++) {
    for (int n = 0; n < squares_to_edge[start_index][dir_index]; n++) {
      int target_index = start_index + directional_offsets[dir_index] * (n + 1);
      Bit *target_bit = _grid[target_index / 8][target_index % 8].bit();
      if (!target_bit) {
        _moves.push_back(Move{start_index, target_index});
        continue;
      }
      int target_tag = target_bit->gameTag();

      if (getPlayerNumber(tag) == getPlayerNumber(target_tag))
        break;

      _moves.push_back(Move{start_index, target_index});

      if (getPlayerNumber(tag) ^ getPlayerNumber(target_tag))
        break;
    }
  }
}

void Chess::GenerateKnightMoves(int start_index, int tag) {
  int north_free = squares_to_edge[start_index][0];
  int south_free = squares_to_edge[start_index][1];
  int west_free = squares_to_edge[start_index][2];
  int east_free = squares_to_edge[start_index][3];

  auto tryAddMove = [&](int offset) {
    int target_index = start_index + offset;
    Bit *target_bit = _grid[target_index / 8][target_index % 8].bit();
    if (!target_bit ||
        getPlayerNumber(tag) != getPlayerNumber(target_bit->gameTag())) {
      _moves.push_back(Move{start_index, target_index});
    }
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
}

void Chess::GenerateKingMoves(int start_index, int tag) {
  auto tryAddMove = [&](int offset) {
    int target_index = start_index + offset;
    Bit *target_bit = _grid[target_index / 8][target_index % 8].bit();
    if (!target_bit ||
        getPlayerNumber(tag) != getPlayerNumber(target_bit->gameTag())) {
      _moves.push_back(Move{start_index, target_index});
    }
  };

  for (int dir = 0; dir < 8; ++dir) {
    if (squares_to_edge[start_index][dir] >= 1) {
      tryAddMove(directional_offsets[dir]);
    }
  }
}
//
// about the only thing we need to actually fill out for tic-tac-toe
//
bool Chess::actionForEmptyHolder(BitHolder &holder) { return false; }

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src) { return true; }

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
  ChessSquare &src_Square = static_cast<ChessSquare &>(src);
  ChessSquare &dst_Square = static_cast<ChessSquare &>(dst);

  int src_index = src_Square.getSquareIndex();
  int dst_index = dst_Square.getSquareIndex();

  Move target = {src_index, dst_index};

  for (const auto &move : _moves) {
    if (move == target) {
      return true;
    }
  }
  return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
  endTurn();
  GenerateMoves(getCurrentPlayer()->playerNumber());
}

//
// free all the memory used by the game on the heap
//
void Chess::stopGame() {}

Player *Chess::checkForWinner() {
  // check to see if either player has won
  return nullptr;
}

bool Chess::checkForDraw() {
  // check to see if the board is full
  return false;
}

//
// add a helper to Square so it returns out FEN chess notation in the form p
// for white pawn, K for black king, etc. this version is used from the top
// level board to record moves
//
const char Chess::bitToPieceNotation(int row, int column) const {
  if (row < 0 || row >= 8 || column < 0 || column >= 8) {
    return '0';
  }

  const char *wpieces = {"?PNBRQK"};
  const char *bpieces = {"?pnbrqk"};
  unsigned char notation = '0';
  Bit *bit = _grid[row][column].bit();
  if (bit) {
    notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()]
                                    : bpieces[bit->gameTag() & 127];
  } else {
    notation = '0';
  }
  return notation;
}

void Chess::setPieceAt(int rank, int file, int playerNumber, ChessPiece piece) {
  Bit *bit = PieceForPlayer(playerNumber, piece);
  bit->setPosition(_grid[rank][file].getPosition());
  bit->setParent(&_grid[rank][file]);
  bit->setGameTag((playerNumber == 0 ? White : Black) | piece);
  _grid[rank][file].setBit(bit);
  char piece_str[2];
  piece_str[1] = 0;
  piece_str[0] = bitToPieceNotation(rank, file);
  _grid[rank][file].setNotation(piece_str);
}

void Chess::setBoardFromFEN(const std::string &string) {
  std::unordered_map<char, std::pair<int, ChessPiece>> fenToPiece = {
      {'p', {1, ChessPiece::Pawn}},   {'n', {1, ChessPiece::Knight}},
      {'b', {1, ChessPiece::Bishop}}, {'r', {1, ChessPiece::Rook}},
      {'q', {1, ChessPiece::Queen}},  {'k', {1, ChessPiece::King}},
      {'P', {0, ChessPiece::Pawn}},   {'N', {0, ChessPiece::Knight}},
      {'B', {0, ChessPiece::Bishop}}, {'R', {0, ChessPiece::Rook}},
      {'Q', {0, ChessPiece::Queen}},  {'K', {0, ChessPiece::King}}};

  int file = 0, rank = 7;

  for (const char &c : string) {
    if (c == '/') {
      file = 0;
      rank--;
    } else if (isdigit(c)) {
      file += (c - '0') - 1;
    } else {
      auto iter = fenToPiece.find(c);
      if (iter != fenToPiece.end()) {
        ChessPiece piece;
        int player;
        std::tie(player, piece) = iter->second;
        setPieceAt(rank, file, player, piece);
        file += 1;
      } else {
        printf("what the flip\n");
      }
    }
  }
}

//
// state strings
//
std::string Chess::initialStateString() { return stateString(); }

//
// this still needs to be tied into imguis init and shutdown
// we will read the state string and store it in each turn object
//
std::string Chess::stateString() {
  std::string s;
  for (int y = 0; y < _gameOptions.rowY; y++) {
    for (int x = 0; x < _gameOptions.rowX; x++) {
      /*s += bitToPieceNotation(y, x);*/
      // remove below this later
      //
      if (Bit *bit = _grid[y][x].bit()) {
        s += std::to_string(bit->gameTag());
      } else {
        s += 'x';
      }
      s += " ";
    }
  }
  return s;
}

//
// this still needs to be tied into imguis init and shutdown
// when the program starts it will load the current game from the imgui ini
// file and set the game state to the last saved state
//
void Chess::setStateString(const std::string &s) {
  for (int y = 0; y < _gameOptions.rowY; y++) {
    for (int x = 0; x < _gameOptions.rowX; x++) {
      int index = y * _gameOptions.rowX + x;
      int playerNumber = s[index] - '0';
      if (playerNumber) {
        _grid[y][x].setBit(PieceForPlayer(playerNumber - 1, Pawn));
      } else {
        _grid[y][x].setBit(nullptr);
      }
    }
  }
}

//
// this is the function that will be called by the AI
//
void Chess::updateAI() {}
