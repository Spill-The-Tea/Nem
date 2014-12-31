#pragma once

#include <vector>
#include <map>
#include "position.h"

using namespace std;

namespace pgn {

	vector<vector<Move>> parsePGNFile(string filename);
	map<string, Move> parsePGNExerciseFile(string filename);
	Move parseSANMove(string mi, position &pos);
	vector<Move> parsePGNGame(vector<string> lines);
	vector<Move> parsePGNGame(vector<string> lines, string &fen);

	enum Result { WHITE_WINS, BLACK_WINS, DRAW, UNKNOWN };

	struct Game {
		Result result;
		vector<Move> Moves;
	};

}