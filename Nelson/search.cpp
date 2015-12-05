#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <math.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <unordered_set>
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
	Stop.store(false);
	PonderMode.store(false);
	History.initialize();
	cmHistory.initialize();
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

std::string baseSearch::PrincipalVariation(position & pos, int depth) {
	std::stringstream ss;
	int i = 0;
	for (; i < depth && i < PV_MAX_LENGTH; ++i) {
		if (PVMoves[i] == MOVE_NONE || !pos.validateMove(PVMoves[i])) break;
		position next(pos);
		if (!next.ApplyMove(PVMoves[i])) break;
		pos = next;
		if (i>0) ss << " ";
		ss << toString(PVMoves[i]);
	}
	for (; i < depth; ++i) {
		Move hashmove = HelperThreads == 0 ? tt::hashmove<tt::UNSAFE>(pos.GetHash()) : tt::hashmove<tt::THREAD_SAFE>(pos.GetHash());
		if (hashmove == MOVE_NONE || !pos.validateMove(hashmove)) break;
		position next(pos);
		if (!next.ApplyMove(hashmove)) break;
		pos = next;
		if (i>0) ss << " ";
		ss << toString(hashmove);
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
	PonderMode.store(false);
	Stop.store(false);
	Exit.store(false);
	BestMove.move = MOVE_NONE;
	BestMove.score = VALUE_NOTYETDETERMINED;
	cmHistory.initialize();
	History.initialize();
	_thinkTime = 0;
}

