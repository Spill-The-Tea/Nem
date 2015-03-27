#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <math.h>
#include <cstring>
#include <thread>
#include <chrono>
#include "search.h"
#include "hashtables.h"

void startThread(search<SLAVE> & slave) {
	slave.startHelper();
}

void baseSearch::Reset() {
	BestMove.move = MOVE_NONE;
	BestMove.score = VALUE_ZERO;
	NodeCount = 0;
	QNodeCount = 0;
	cutoffAt1stMove = 0;
	cutoffCount = 0;
	cutoffMoveIndexSum = 0;
	Stop = false;
	PonderMode = false;
	History.initialize();
	DblHistory.initialize();
	for (int i = 0; i < 2 * MAX_DEPTH; ++i) killer[i] = EXTENDED_MOVE_NONE;
}

void baseSearch::NewGame() {
	Reset();
	for (int i = 0; i < 12 * 64; ++i) {
		counterMove[i] = MOVE_NONE;
		//gains[i] = VALUE_ZERO;
	}
	for (int i = 0; i < 12 * 64; ++i) gains[i] = VALUE_ZERO;
}

std::string baseSearch::PrincipalVariation(int depth) {
	std::stringstream ss;
	for (int i = 0; i < depth; ++i) {
		if (PVMoves[i] == MOVE_NONE) break;
		if (i>0) ss << " ";
		ss << toString(PVMoves[i]);
	}
	return ss.str();
}

Move baseSearch::GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount) {
	if (USE_BOOK && BookFile.size() > 0) {
		book.probe(pos, BookFile, true, moves, moveCount);
	}
	return MOVE_NONE;
}

baseSearch::~baseSearch() {

}

baseSearch::baseSearch() {
	DblHistory.initialize();
}

