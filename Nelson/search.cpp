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
	MaxDepth = 0;
	cutoffAt1stMove = 0;
	cutoffCount = 0;
	cutoffMoveIndexSum = 0;
	Stop = false;
	PonderMode = false;
	History.initialize();
	DblHistory.initialize();
	for (int i = 0; i < MAX_DEPTH; ++i){
		killer[2*i] = EXTENDED_MOVE_NONE;
		killer[2 * i + 1] = EXTENDED_MOVE_NONE;
		//excludedMoves[i] = MOVE_NONE;
	}
}

void baseSearch::NewGame() {
	Reset();
	for (int i = 0; i < 12 * 64; ++i) {
		counterMove[i] = MOVE_NONE;
	}
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
		if (book == nullptr) book = new polyglot::book(BookFile);
		book->probe(pos, true, moves, moveCount);
	}
	return MOVE_NONE;
}

baseSearch::~baseSearch() {
	if (book != nullptr) delete book;
}

baseSearch::baseSearch() {
	BestMove.move = MOVE_NONE;
	BestMove.score = VALUE_NOTYETDETERMINED;
	DblHistory.initialize();
	History.initialize();
	_thinkTime = 0;
}

