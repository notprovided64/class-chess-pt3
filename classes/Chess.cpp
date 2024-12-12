#include "Chess.h"
#include "Board.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <stdlib.h>
#include <string>
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

Chess::Chess() {
  setNumberOfPlayers(2);
  setAIPlayer(AI_PLAYER);
  _gameOptions.rowX = 8;
  _gameOptions.rowY = 8;
}

Chess::~Chess() {
  for (auto &_turn : _turns) {
    delete _turn;
  }
  _turns.clear();
  for (auto &_player : _players) {
    delete _player;
  }
  _players.clear();

  _table = nullptr;
  _winner = nullptr;
  _lastMove = "";
}

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
  /*setGameFromFEN("4k3/8/8/8/8/8/8/RRBQKBRR");*/
  _board.state = stateString();

  generateMoves();
}

bool Chess::actionForEmptyHolder(BitHolder &holder) { return false; }

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src) {
  ChessSquare &src_Square = static_cast<ChessSquare &>(src);
  int src_index = src_Square.getSquareIndex();

  for (auto move : getCurrentMoves()) {
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

  auto moves = _moves;
  return std::find(moves.begin(), moves.end(), target) != moves.end();
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
  ChessSquare &src_Square = static_cast<ChessSquare &>(src);
  ChessSquare &dst_Square = static_cast<ChessSquare &>(dst);
  int src_index = src_Square.getSquareIndex();
  int dst_index = dst_Square.getSquareIndex();

  Move target = {src_index, dst_index};
  makeMove(target);
}
//
// free all the memory used by the game on the heap
//
void Chess::stopGame() {
  for (int y = 0; y < _gameOptions.rowY; y++) {
    for (int x = 0; x < _gameOptions.rowX; x++) {
      _grid[y][x].destroyBit();
    }
  }
}

Player *Chess::checkForWinner() {
  if (getCurrentMoves().size() != 0) {
    return nullptr;
  }
  // current player is loser here, therefore
  if (getCurrentPlayer()->playerNumber() == AI_PLAYER) {
    return getPlayerAt(getHumanPlayer());
  } else {
    return getPlayerAt(getAIPlayer());
  }
}

bool Chess::checkForDraw() {
  // check to see if the board is full
  return false;
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
  _board.castleStatus = 0;
  if (castle[0] != '-') {
    if (castle.find('K') != std::string::npos) {
      _board.castleStatus |= CastleStatus::K;
    }
    if (castle.find('Q') != std::string::npos) {
      _board.castleStatus |= CastleStatus::Q;
    }
    if (castle.find('k') != std::string::npos) {
      _board.castleStatus |= CastleStatus::k;
    }
    if (castle.find('q') != std::string::npos) {
      _board.castleStatus |= CastleStatus::q;
    }
  }

  std::string enpas = tokens[3];
  if (enpas[0] != '-') {
    int file = enpas[0] - 'a';
    int rank = enpas[0] - '0';

    _board.enPassantIndex = rank * 8 + file;
  }

  int full_turn = std::stoi(tokens[5]);
  _gameOptions.currentTurnNo += (full_turn - 1) * 2;
  if (_gameOptions.currentTurnNo % 2 == 1)
    _board.isWhiteTurn = false;
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
  std::string s = _board.state;
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

void Chess::updateGrid() {
  for (int rank = 0; rank < 8; ++rank) {
    for (int file = 0; file < 8; ++file) {
      char notation = _board.state[rank * 8 + file];

      int playerNumber;
      ChessPiece piece = charToPiece(notation);

      if (notation == '0') {
        if (_grid[rank][file].bit() != nullptr) {
          _grid[rank][file].setBit(nullptr);
          _grid[rank][file].setNotation("0");
        }
        continue;
      }
      playerNumber = isupper(notation) ? 0 : 1;

      Bit *existingBit = _grid[rank][file].bit();
      if (!existingBit || existingBit->gameTag() !=
                              ((playerNumber == 0 ? White : Black) | piece)) {
        setPieceAt(rank, file, playerNumber, piece);
      }
    }
  }
}

void Chess::generateMoves() { _moves = _board.GenerateLegalMoves(); }

void Chess::makeMove(Move move) {
  _board.makeMove(move);
  if (move.flag != MoveFlag::None ||
      getAIPlayer() == getCurrentPlayer()->playerNumber())
    updateGrid();

  generateMoves();
  endTurn();
}

//
// this is the function that will be called by the AI
//
void Chess::updateAI() {
  if (_winner != nullptr)
    return;

  auto moves = getCurrentMoves();
  if (moves.size() == 0)
    return;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, moves.size() - 1);

  int randomIndex = dis(gen);

  makeMove(moves[randomIndex]);
}
