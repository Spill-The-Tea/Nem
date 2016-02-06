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
	Stop.store(false);
	PonderMode.store(false);
	History.initialize();
	searchMoves.clear();
	cmHistory.initialize();
	for (int i = 0; i < MAX_DEPTH; ++i){
		killer[2*i] = EXTENDED_MOVE_NONE;
		killer[2 * i + 1] = EXTENDED_MOVE_NONE;
		//excludedMoves[i] = MOVE_NONE;
	}
	for (int i = 0; i < PV_MAX_LENGTH; ++i) PVMoves[i] = MOVE_NONE;
}

void baseSearch::NewGame() {
	Reset();
	for (int i = 0; i < 12 * 64; ++i) {
		counterMove[i] = MOVE_NONE;
	}
}

std::string baseSearch::PrincipalVariation(position & pos, int depth) {
	std::stringstream ss;
	//When pondering start PV with ponder move 
	if (PonderMode.load() && pos.GetLastAppliedMove() != MOVE_NONE) ss << toString(pos.GetLastAppliedMove()) << " ";
	int i = 0;
	ponderMove = MOVE_NONE;
	//First get PV from PV array...
	for (; i < depth && i < PV_MAX_LENGTH; ++i) {
		if (PVMoves[i] == MOVE_NONE || !pos.validateMove(PVMoves[i])) break;
		position next(pos);
		if (!next.ApplyMove(PVMoves[i])) break;
		pos = next;
		if (i>0) ss << " ";
		ss << toString(PVMoves[i]);
		if (i == 1) ponderMove = PVMoves[i];
	}
	//...then continue with moves from transposition table
	for (; i < depth; ++i) {
		Move hashmove = HelperThreads == 0 ? tt::hashmove<tt::UNSAFE>(pos.GetHash()) : tt::hashmove<tt::THREAD_SAFE>(pos.GetHash());
		if (hashmove == MOVE_NONE || !pos.validateMove(hashmove)) break;
		position next(pos);
		if (!next.ApplyMove(hashmove)) break;
		pos = next;
		if (i>0) ss << " ";
		ss << toString(hashmove);
		if (i == 1) ponderMove = hashmove;
	}
	return ss.str();
}

//Creates the "thinking output" while running in UCI or XBoard mode
void baseSearch::info(position &pos, int pvIndx, SearchResultType srt) {
	if ((UciOutput || XBoardOutput)) {
		position npos(pos);
		npos.copy(pos);
		if (UciOutput) {
			std::string srtString;
			if (srt == SearchResultType::FAIL_LOW) srtString = " upperbound"; else if (srt == SearchResultType::FAIL_HIGH) srtString = " lowerbound";
			if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
				sync_cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score cp " << BestMove.score << srtString << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
				<< " hashfull " << tt::GetHashFull()
#ifdef TB
				<< " tbhits " << tbHits
#endif
				<< " time " << _thinkTime
				<< " pv " << PrincipalVariation(npos, _depth) << sync_endl;
			else {
				int pliesToMate;
				if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
				sync_cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score mate " << pliesToMate / 2 << srtString << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
					<< " hashfull " << tt::GetHashFull()
#ifdef TB
					<< " tbhits " << tbHits
#endif
					<< " time " << _thinkTime
					<< " pv " << PrincipalVariation(npos, _depth) << sync_endl;
			}
		}
		else if (XBoardOutput) {
			if (_depth < 5) return;
			const char srtChar[3] = { ' ', '?', '!' };
			int xscore = BestMove.score;
			if (abs(int(BestMove.score)) > int(VALUE_MATE_THRESHOLD)) {
				if (int(BestMove.score) > 0) {
					int pliesToMate = VALUE_MATE - BestMove.score;
					xscore = 100000 + pliesToMate;
				}
				else {
					int pliesToMate = -BestMove.score - VALUE_MATE;
					xscore = -100000 - pliesToMate;
				}
			}
			sync_cout << _depth << " " << xscore << " " << _thinkTime / 10 << " " << NodeCount << " " /*<< MaxDepth << " " << (NodeCount / _thinkTime) * 1000
#ifdef TB
				<< " " << tbHits
#endif
				<< "\t"*/ << PrincipalVariation(npos, _depth)  << srtChar[srt] << sync_endl;
		}
	}
}

void baseSearch::debugInfo(std::string info)
{
	if (UciOutput) {
		sync_cout << "info string " << info << sync_endl;
	}
	else if (XBoardOutput) {
		sync_cout << "# " << info << sync_endl;
	}
	//else
	//{
	//	sync_cout << info << sync_endl;
	//}
}

Move baseSearch::GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount) {
	if (USE_BOOK && BookFile.size() > 0) {
		if (book == nullptr) book = new polyglot::book(BookFile);
		book->probe(pos, true, moves, moveCount);
	}
	return MOVE_NONE;
}

std::string baseSearch::GetXAnalysisOutput() {
	std::lock_guard<std::mutex> lck(mtxXAnalysisOutput);
	return XAnalysisOutput;
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
	for (int i = 0; i < PV_MAX_LENGTH; ++i) PVMoves[i] = MOVE_NONE;
}

