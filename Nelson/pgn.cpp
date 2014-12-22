#include <regex>
#include <sstream>
#include <fstream>
#include <iostream>
#include "pgn.h"

using namespace std;

namespace pgn {

	const string WHITESPACE = " \n\r\t";

	string TrimLeft(const string& s)
	{
		size_t startpos = s.find_first_not_of(WHITESPACE);
		return (startpos == string::npos) ? "" : s.substr(startpos);
	}

	string TrimRight(const string& s)
	{
		size_t endpos = s.find_last_not_of(WHITESPACE);
		return (endpos == string::npos) ? "" : s.substr(0, endpos + 1);
	}

	string Trim(const string& s)
	{
		return TrimRight(TrimLeft(s));
	}

	PieceType getPieceType(char c) {
		switch (c) {
		case 'Q': case 'D': return QUEEN;
		case 'R': case 'T': return ROOK;
		case 'B': case 'L': return BISHOP;
		case 'N': case 'S': return KNIGHT;
		case 'K': return KING;
		default: return PAWN;
		}
	}

	Square getSquare(char file, char rank) {
		int fileI = int(file) - int('a');
		int rankI = int(rank) - int('1');
		return Square(8 * rankI + fileI);
	}
		 
	/* Tokenizing a string */
	vector<string> tokenize(const string& p_pcstStr, char delim)  {
		vector<string> tokens;
		stringstream   mySstream(p_pcstStr);
		string         temp;
		while (getline(mySstream, temp, delim)) {
			tokens.push_back(temp);
		}
		return tokens;
	}
	//Parses a single move (assuming that only move data is part of the string - no annotations,
	//no check symbol, ...
	Move parseSANMove(string move, position & pos) {
		size_t indx = move.find("+");
		if (indx != string::npos) move = move.substr(0, indx);
		indx = move.find("#");
		if (indx != string::npos) move = move.substr(0, indx);
		move = Trim(move);
		ValuatedMove * moves = pos.GenerateMoves<ALL>();
		Move m;
		if (!move.compare("0-0") || !move.compare("O-O") || !move.compare("0-0-0") || !move.compare("O-O-O")) {
			m = moves->move;
			while (m) {
				if (type(m) == CASTLING) {
					if ((to(m) & 7) == 6 && move.length() < 5) return m;
					else if ((to(m) & 7) == 2 && move.length() > 4) return m;
				}
				++moves;
				m = moves->move;
			}
			return MOVE_NONE;
		}
		PieceType movingPieceType = getPieceType(move[0]);
		bool isPromotion = movingPieceType == PAWN && move.find("=") != string::npos;
		size_t len = move.length();
		PieceType promotionType;
		Square targetSquare;
		if (isPromotion) {
			promotionType = getPieceType(move[len - 1]);
			targetSquare = getSquare(move[len - 4], move[len - 3]);
			m = moves->move;
			while (m) {
				if ((type(m) == PROMOTION) && (to(m) == targetSquare) && (GetPieceType(pos.GetPieceOnSquare(from(m))) == movingPieceType)) return m;
				++moves;
				m = moves->move;
			}
			return MOVE_NONE;
		}
		targetSquare = getSquare(move[len - 2], move[len - 1]);
		Move m1 = MOVE_NONE;
		Move m2 = MOVE_NONE;
		m = moves->move;
		while (m) {
			if ((to(m) == targetSquare) && (GetPieceType(pos.GetPieceOnSquare(from(m))) == movingPieceType)) {
				if (m1) {
					m2 = m;
					break;
				}
				else m1 = m;
			}
			++moves;
			m = moves->move;
		}
		//Now we either have a unique move or we need disambiguation
		if (m2) {
			if (movingPieceType == PAWN) {
				//Only option for ambiguity: pawn capture => file is stored in 1st char
				int fromFile = int(move[0]) - int('a');
				if ((from(m1) & 7) == fromFile) return m1; else return m2;
			}
			else {
				//Non-Pawn move => 2nd char needed 
				int fromFile = int(move[1]) - int('a');
				if (fromFile >= 0 && fromFile <= 7) {
					if ((from(m1) & 7) == fromFile) return m1; else return m2;
				}
				int fromRank = int(move[1]) - int('1');
				if ((from(m1) / 8) == fromRank) return m1; else return m2;
			}
		}
		else return m1;
	}

	const string INITIAL_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	vector<Move> parsePGNGame(vector<string> lines, string & fen) {
		vector<Move> moves;
		position * pos = new position();
		fen = INITIAL_FEN;
		for (vector<string>::iterator it = lines.begin(); it != lines.end(); ++it) {
			string line = Trim(*it);
			if (line.length() == 0) continue;
			if (line[0] == '[') {
				if (!line.substr(0, 4).compare("[FEN")) {
					size_t indx1 = line.find_first_of("\"");
					size_t indx2 = line.find_last_of("\"");
					fen = line.substr(indx1 + 1, indx2 - indx1 - 1);
					delete(pos);
					pos = new position(fen);
				}
				continue;
			}
			
			vector<string> tokens = tokenize(line, ' ');
			for (vector<string>::iterator t = tokens.begin(); t != tokens.end(); ++t) {
				string token((*t).append("\0"));
				if (token.find_first_not_of("abcdefgh1234567890KQRBNx+#=.") != string::npos) goto END;
				if (token.find('.') != string::npos) continue; //Move number
				Move move = parseSANMove(token, *pos);
				if (move) {
					moves.push_back(move);
					pos->ApplyMove(move);
				}
			}
		}
		END: delete(pos);
		return moves;
	}

	vector<Move> parsePGNGame(vector<string> lines) {
		string fen;
		return parsePGNGame(lines, fen);
	}

	vector<vector<Move>> parsePGNFile(string filename) {
		vector<vector<Move>> result;
		ifstream s;
		s.open(filename);
		string l;
		bool inTag = false;
		bool inMoves = false;
		vector<string> * gameLines = new vector<string>();
		if (s.is_open())
		{
			long lineCount = 0;
			while (s)
			{
				getline(s, l);
				string line = Trim(l);
				gameLines->push_back(line);
				if (line.length() > 0) {
					if (inTag && line[0] != '[') {
						inTag = false;
						inMoves = true;
					}
					else if (!inTag && line[0] == '[') {
						if (inMoves) {
							result.push_back(parsePGNGame(*gameLines));
							gameLines->clear();
						}
						inTag = true;
						inMoves = false;
					}
				}
			}
			if (gameLines->size() > 0) result.push_back(parsePGNGame(*gameLines));
		}
		delete(gameLines);
		return result;
	}

	map<string, Move> parsePGNExerciseFile(string filename) {
		map<string, Move> result;
		ifstream s;
		s.open(filename);
		string l;
		bool inTag = false;
		bool inMoves = false;
		uint64_t gameCount = 0;
		vector<string> * gameLines = new vector<string>();
		if (s.is_open())
		{
			long lineCount = 0;
			while (s)
			{
				getline(s, l);
				string line = Trim(l);
				gameLines->push_back(line);
				if (line.length() > 0) {
					if (inTag && line[0] != '[') {
						inTag = false;
						inMoves = true;
					}
					else if (!inTag && line[0] == '[') {
						if (inMoves) {
							string fen;
							vector<Move> moves = parsePGNGame(*gameLines, fen);
							if (moves.size() > 0){
								result[fen] = moves[0];
								gameCount++;
								if ((gameCount % 1000) == 0) cout << gameCount << endl;
							}
							gameLines->clear();
						}
						inTag = true;
						inMoves = false;
					}
				}
			}
			if (gameLines->size() > 0) {
				string fen;
				vector<Move> moves = parsePGNGame(*gameLines, fen);
				result[fen] = moves[0];
			}
		}
		delete(gameLines);
		return result;
	}

}