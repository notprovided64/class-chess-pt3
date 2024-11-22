#include "Chess.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

const int AI_PLAYER = 1;
const int HUMAN_PLAYER = -1;

// should realistically be in a different file
std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;

  while (getline(ss, item, delim)) {
    result.push_back(item);
  }

  return result;
}

std::string reverseLines(const std::string &input) {
  std::vector<std::string> lines;
  std::istringstream stream(input);
  std::string line;

  while (std::getline(stream, line)) {
    lines.push_back(line);
  }

  std::reverse(lines.begin(), lines.end());

  std::ostringstream output;
  for (const auto &line : lines) {
    output << line << "\n";
  }

  return output.str();
}

ChessPiece getPiece(int tag) { return ChessPiece(tag & 127); }
ChessPiece char_to_piece(char piece) {
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

bool is_white(char piece) {
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

int getPlayerNumber(int tag) { return tag < 128 ? 0 : 1; }
bool isWhite(int tag) { return getPlayerNumber(tag) == 0; }

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

  /*setGameFromFEN("r3k4r/1b4bq/8/8/8/8/7B/R3K4R b KQkq - 0 1");*/
  setGameFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
  _state = stateString();

  GenerateMoves(getCurrentPlayer()->playerNumber());
}

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
    if (!isFriendly(i))
      continue;

    ChessPiece piece = char_to_piece(_state[i]);
    switch (piece) {
    case NoPiece:
      continue;
    case Pawn:
      GeneratePawnMoves(i);
      break;
    case Knight:
      GenerateKnightMoves(i);
      break;
    case Bishop:
    case Rook:
    case Queen:
      GenerateSlidingMoves(piece, i);
      break;
    case King:
      GenerateKingMoves(i);
      break;
    default:
      break;
    }
  }
}
void Chess::GeneratePawnMoves(int index) {
  int north_free = squares_to_edge[index][0];
  int south_free = squares_to_edge[index][1];
  int west_free = squares_to_edge[index][2];
  int east_free = squares_to_edge[index][3];

  int baseMovementOffset = _is_white_turn ? 8 : -8;
  int extendedMovementOffset = _is_white_turn ? 16 : -16;
  int captureOffsetWest = _is_white_turn ? 7 : -9;
  int captureOffsetEast = _is_white_turn ? 9 : -7;
  int freeMoveSquares = _is_white_turn ? north_free : south_free;

  int startingRank = _is_white_turn ? 1 : 6;
  bool inStartingRank = index / 8 == startingRank;

  if (freeMoveSquares < 1)
    return;

  int target_index = index + baseMovementOffset;
  if (isEmpty(target_index)) {
    _moves.push_back(Move{index, target_index});
    if (inStartingRank && freeMoveSquares >= 2) {
      target_index = index + extendedMovementOffset;
      if (isEmpty(target_index)) {
        _moves.push_back(Move{index, target_index});
      }
    }
  }

  if (west_free >= 1) {
    target_index = index + captureOffsetWest;
    if (isOpponent(target_index) || (target_index == _enpas_index))
      _moves.push_back(Move{index, target_index});
  }
  if (east_free >= 1) {
    target_index = index + captureOffsetEast;
    if (isOpponent(target_index) || (target_index == _enpas_index))
      _moves.push_back(Move{index, target_index});
  }
}

bool Chess::isEmpty(int index) {
  if (char_to_piece(_state[index]) == NoPiece)
    return true;
  return false;
}
bool Chess::isFriendly(int index) {
  if (char_to_piece(_state[index]) == NoPiece)
    return false;
  return is_white(_state[index]) == _is_white_turn;
}
bool Chess::isOpponent(int index) {
  if (char_to_piece(_state[index]) == NoPiece)
    return false;
  return is_white(_state[index]) != _is_white_turn;
}

void Chess::GenerateSlidingMoves(ChessPiece piece, int index) {
  int start_dir_index = (piece == Bishop) ? 4 : 0;
  int end_dir_index = (piece == Rook) ? 4 : 8;

  for (int dir_index = start_dir_index; dir_index < end_dir_index;
       dir_index++) {
    for (int n = 0; n < squares_to_edge[index][dir_index]; n++) {
      int target_index = index + directional_offsets[dir_index] * (n + 1);
      ChessPiece target_piece = char_to_piece(_state[target_index]);
      if (isFriendly(target_index)) {
        break;
      } else if (target_piece != NoPiece) {
        _moves.push_back(Move{index, target_index});
        break;
      }

      _moves.push_back(Move{index, target_index});
    }
  }
}

void Chess::GenerateKnightMoves(int index) {
  int north_free = squares_to_edge[index][0];
  int south_free = squares_to_edge[index][1];
  int west_free = squares_to_edge[index][2];
  int east_free = squares_to_edge[index][3];

  auto tryAddMove = [&](int offset) {
    int target_index = index + offset;
    int target_piece = char_to_piece(_state[target_index]);
    if (!isFriendly(target_index))
      _moves.push_back(Move{index, target_index});
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

void Chess::GenerateKingMoves(int index) {
  auto tryAddMove = [&](int offset) {
    int target_index = index + offset;
    if (!isFriendly(target_index)) {
      _moves.push_back(Move{index, target_index});
    }
  };

  int startingRank = _is_white_turn ? 0 : 7;

  for (int dir = 0; dir < 8; ++dir) {
    if (squares_to_edge[index][dir] >= 1) {
      tryAddMove(directional_offsets[dir]);
    }
  }

  GenerateCastlingMoves(index);
}
void Chess::GenerateCastlingMoves(int index) {
  auto tryAddMove = [&](bool isRight) {
    int limit = isRight ? squares_to_edge[index][3] : squares_to_edge[index][2];
    for (int i = 1; i <= limit; i++) {
      int target_index = isRight ? index + i : index - i;
      if (char_to_piece(_state[target_index]) == Rook)
        _moves.push_back(Move{index, isRight ? index + 2 : index - 2});
      else if (!isEmpty(target_index))
        return;
    }
  };

  if (_is_white_turn) {
    if (_castle_status & CastleStatus::K) {
      tryAddMove(true);
    }
    if (_castle_status & CastleStatus::Q) {
      tryAddMove(false);
    }
  } else {
    if (_castle_status & CastleStatus::k) {
      tryAddMove(true);
    }
    if (_castle_status & CastleStatus::q) {
      tryAddMove(false);
    }
  }
}

//
// about the only thing we need to actually fill out for tic-tac-toe
//
bool Chess::actionForEmptyHolder(BitHolder &holder) { return false; }

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src) {
  ChessSquare &src_Square = static_cast<ChessSquare &>(src);
  int src_index = src_Square.getSquareIndex();

  for (auto move : _moves) {
    if (move.StartSquare == src_index)
      return true;
  }

  return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
  ChessSquare &src_Square = static_cast<ChessSquare &>(src);
  ChessSquare &dst_Square = static_cast<ChessSquare &>(dst);

  int src_index = src_Square.getSquareIndex();
  int dst_index = dst_Square.getSquareIndex();

  Move target = {src_index, dst_index};

  return std::find(_moves.begin(), _moves.end(), target) != _moves.end();
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
  ChessSquare &src_Square = static_cast<ChessSquare &>(src);
  ChessSquare &dst_Square = static_cast<ChessSquare &>(dst);

  int src_index = src_Square.getSquareIndex();
  int dst_index = dst_Square.getSquareIndex();

  Move target = {src_index, dst_index};

  int row = dst_Square.getRow();
  int col = dst_Square.getColumn();

  updateExtrinsicState(target); // do we update castling and enpassant based on
                                // pre move state or post?
  _state[src_Square.getSquareIndex()] = '0';
  _state[dst_Square.getSquareIndex()] = bitToPieceNotation(row, col);

  _is_white_turn = !_is_white_turn;
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

bool Chess::canCastle(bool kingside) {
  if (_castle_status == 0)
    return false;
  else if (_is_white_turn) {
    if (_castle_status & CastleStatus::K && kingside)
      return true;
    if (_castle_status & CastleStatus::Q && !kingside)
      return true;
  } else {
    if (_castle_status & CastleStatus::k && kingside)
      return true;
    if (_castle_status & CastleStatus::q && !kingside)
      return true;
  }
}

//
// add a helper to Square so it returns out FEN chess notation in the form p
// for white pawn, K for black king, etc. this version is used from the top
// level board to record moves
//
void Chess::disableCastlability(bool is_white) {
  // fmkcl
  _castle_status &= is_white ? ~(K | Q) : ~(k | q);
}
void Chess::disableCastlability(bool is_white, bool kingside) {
  if (is_white) {
    _castle_status ^= kingside ? K : Q;
  } else {
    _castle_status ^= kingside ? k : q;
  }
}
void Chess::setEnpasSquare(int startSquare, bool isWhiteTurn) {
  int offset = isWhiteTurn ? 8 : -8;

  _enpas_index = startSquare + offset;
}
void Chess::handleEnpas(int endSquare, bool isWhiteTurn) {
  int offset, captureIndex;

  offset = isWhiteTurn ? -8 : 8;
  captureIndex = endSquare + offset;

  _grid[captureIndex / 8][captureIndex % 8].setBit(nullptr);
  _state[captureIndex] = '0';
}

void Chess::handleCastling(int startSquare, int endSquare, bool isWhiteTurn) {
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

  setPieceAt(rookIndex / 8, rookIndex % 8, isWhiteTurn ? 0 : 1, Rook);
  _state[rookIndex] = rookChar;

  _grid[row][rookStartColumn].setBit(nullptr);
  _state[8 * row + rookStartColumn] = '0';
}

void Chess::handlePromotion(int endSquare, bool isWhiteTurn) {
  int finalRow = isWhiteTurn ? 7 : 0;
  int squareRow = endSquare / 8;

  if (squareRow != finalRow)
    return;

  char queenChar = isWhiteTurn ? 'Q' : 'q';

  setPieceAt(endSquare / 8, endSquare % 8, isWhiteTurn ? 0 : 1, Queen);
  _state[endSquare] = queenChar;
}

// this is where we handle castling and en passant
void Chess::updateExtrinsicState(Move move) {

  // disable ability to castle once rook has been moved
  ChessPiece piece = char_to_piece(_state[move.StartSquare]);

  bool can_castle_kingside = canCastle(true);
  bool can_castle_queenside = canCastle(false);
  bool can_castle = can_castle_kingside || can_castle_queenside;

  if (can_castle) {
    if (piece == Rook) {
      if (can_castle_kingside &&
          (move.StartSquare == (_is_white_turn ? 7 : 63)))
        disableCastlability(_is_white_turn, true);
      if (can_castle_queenside &&
          (move.StartSquare == (_is_white_turn ? 0 : 56)))
        disableCastlability(_is_white_turn, false);
    }
    if (piece == King) {
      disableCastlability(_is_white_turn);

      // if king moves 2 squares we castling
      if (std::abs(move.StartSquare - move.EndSquare) == 2) {
        handleCastling(move.StartSquare, move.EndSquare, _is_white_turn);
      }
    }
  }

  if (piece == Pawn) {
    // if pawn moved two spaces we set en passant
    if (std::abs(move.StartSquare - move.EndSquare) == 16) {
      setEnpasSquare(move.StartSquare, _is_white_turn);
    } else if (std::abs(move.StartSquare - move.EndSquare) != 8) {
      // we captured something
      ChessPiece capture = char_to_piece(_state[move.EndSquare]);
      if (capture == NoPiece)
        handleEnpas(move.EndSquare, _is_white_turn);
    }
    handlePromotion(move.EndSquare, _is_white_turn);
  }
}

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

void Chess::setGameFromFEN(const std::string &string) {
  std::vector<std::string> tokens = split(string, ' ');
  if (tokens.size() == 0 || (tokens.size() != 1 && tokens.size() != 6)) {
    fprintf(stderr, "invalid fen string\n");
    exit(1);
  }

  setBoardFromFEN(tokens[0]);
  if (tokens.size() != 6) {
    return;
  }

  std::string player = tokens[1];
  if (player.find('b') != std::string::npos) {
    _gameOptions.currentTurnNo += 1;
  }

  std::string castle = tokens[2];
  _castle_status = 0;
  if (castle[0] != '-') {
    if (castle.find('K') != std::string::npos) {
      _castle_status |= CastleStatus::K;
    }
    if (castle.find('Q') != std::string::npos) {
      _castle_status |= CastleStatus::Q;
    }
    if (castle.find('k') != std::string::npos) {
      _castle_status |= CastleStatus::k;
    }
    if (castle.find('q') != std::string::npos) {
      _castle_status |= CastleStatus::q;
    }
  }

  std::string enpas = tokens[3];
  if (enpas[0] != '-') {
    int file = enpas[0] - 'a';
    int rank = enpas[0] - '0';

    _enpas_index = rank * 8 + file;
  }

  int full_turn = std::stoi(tokens[5]);
  _gameOptions.currentTurnNo += (full_turn - 1) * 2;
  if (_gameOptions.currentTurnNo % 2 == 1)
    _is_white_turn = false;
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

  // parse piece placement
  size_t string_pos = 0;
  for (const char &c : string) {
    if (c == ' ') {
      break;
    } else if (c == '/') {
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
      string_pos++;
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
      s += bitToPieceNotation(y, x);
    }
  }
  return s;
}

std::string Chess::stateStringPretty() {
  std::string s = _state;
  std::stringstream ss;

  for (int i = 0; i < s.size(); ++i) {
    if (i % 8 == 0 && i != 0)
      ss << '\n';
    ss << s[i];
  }

  return reverseLines(ss.str());
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
