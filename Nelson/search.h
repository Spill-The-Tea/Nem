#pragma once

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include "types.h"
#include "book.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include "timemanager.h"
#include "utils.h"
#include "tablebase.h"


enum ThreadType { SINGLE, MASTER, SLAVE };
enum SearchResultType { EXACT_RESULT, FAIL_LOW, FAIL_HIGH, TABLEBASE_MOVE, UNIQUE_MOVE, BOOK_MOVE };

//Struct contains thread local data, which isn't shared among threads
struct threadData {
	//History tables used in move ordering during search
	MoveSequenceHistoryManager cmHistory;
	MoveSequenceHistoryManager followupHistory;
	HistoryManager History;
	killer::manager killerManager;
};

class search {
public:
	//the search itself issues the info output while thinking. To do is it needs to know whether it's running in xboard, or uci mode. 
	//This is of course from OO perspective bad, and the output should be done in the resp. protocol drivers, but as long as the number
	//of protocols will stay at 2, it's easier, than creating an generic info class/struct, which then can be handled by the different protocol drivers
	bool UciOutput = false;
	bool XBoardOutput = false;
	bool PrintCurrmove = true;
	//Flag indicating the engine is currently pondering
	std::atomic<bool> PonderMode;
	//The search's result (will be updated while searching)
	ValuatedMove BestMove;
	//Total Node Count (including nodes from Quiescence Search)
	int64_t NodeCount = 0;
	//Quiescence Search Node Count
	int64_t QNodeCount = 0;
	//Maximum Depth reached during Search
	int MaxDepth = 0;
	//Flag, indicating that search shall stop as soon as possible 
	std::atomic<bool> Stop;
	//Polyglot bookfile if provided
	std::string * BookFile = nullptr;
	int rootMoveCount = 0;
	ValuatedMove rootMoves[MAX_MOVE_COUNT];
	position rootPosition;
	//For analysis purposes, list of root moves which shall be considered
	std::vector<Move> searchMoves;
	//Number of root moves for which full analysis shall be done
	int MultiPv = 1;
	//The timemanager, which handles the time assignment and checks if a search shall be continued
	timemanager timeManager;

	//Re-initialize BestMove, History, CounterMoveHistory, Killer, Node counters, ....
	void Reset();
	//Determines the Principal Variation (PV is stored during search in "triangular" array and is completed from hash table, if
	//it's incomplete (due to prunings)
	std::string PrincipalVariation(position & pos, int depth = PV_MAX_LENGTH);
	//Reset engine to start a new game
	void NewGame();
	//Returns the current nominal search depth
	inline int Depth() const { return _depth; }
	//Returns the thinkTime needed so far (is updated after every iteration)
	inline Time_t ThinkTime() const { return _thinkTime; }
	//Statistic data no needed for playing
	inline double cutoffAt1stMoveRate() const { return 100.0 * cutoffAt1stMove / cutoffCount; }
	inline double cutoffAverageMove() const { return 1 + 1.0 * cutoffMoveIndexSum / cutoffCount; }
	//Determines the move the engine assumes that the opponent is playing
	inline Move PonderMove() const { return ponderMove; }
	//Stops the current search immediatialy
	inline void StopThinking() {
		PonderMode.store(false);
		Stop.store(true);
	}
	//Get's the "best" move from the polyglot opening book
	Move GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount);
	//returns the analysis output for xboard in thread-safe mode 
	std::string GetXAnalysisOutput();
	//handling of the thinking output for uci and xboard
	void info(position &pos, int pvIndx, SearchResultType srt = EXACT_RESULT);

	void debugInfo(std::string info);

	search();
	~search();
	//Main entry point 
	ValuatedMove Think(position &pos);
	//In case of SMP, start Slave threads
	void startHelper();

	std::mutex mtxSearch;
	//slave threads
	std::vector<std::thread> subThreads;
	//Utility method (not used when thinking). Checks if a position is quiet (that's static evalution is about the same as QSearch result). Might be
	//useful for tuning
	bool isQuiet(position &pos);

	Value qscore(position * pos);

private:
	//Mutex to synchronize access to analysis output
	std::mutex mtxXAnalysisOutput;
	//Polyglot book object to read book moves
	polyglot::book * book = nullptr;

	threadData threadLocalData;

	//Internal statistics (to get a measure of move ordering quality)
	uint64_t cutoffAt1stMove = 0;
	uint64_t cutoffCount = 0;
	uint64_t cutoffMoveIndexSum = 0;
	//in xboard protocol in analysis mode, the GUI determines the point in time when information shall be provided (in UCI the engine determines these 
	//point in times. As this information is only available while the search's recursion level is root the information is prepared, whenever feasible and
	//stored in member XAnylysisOutput, where the protocol driver can retrieve it at any point in time
	std::string * XAnalysisOutput = nullptr;

	//Array where moves from Principal Variation are stored
	Move PVMoves[PV_MAX_LENGTH];
	//Move which engine expects opponent will reply to it's next move
	Move ponderMove = MOVE_NONE;
	int _depth = 0;
	Time_t _thinkTime;
	Move counterMove[12][64];
	uint64_t tbHits = 0;
	//Flag indicating whether TB probes shall be made during search
	bool probeTB = true;

	inline bool Stopped() { return Stop; }

	//Main recursive search method
	template<ThreadType T> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv, threadData& tlData, bool cutNode, bool prune = true, Move excludeMove = MOVE_NONE);
	//At root level there is a different search method (as there is some special logic requested)
	template<ThreadType T> Value SearchRoot(Value alpha, Value beta, position &pos, int depth, ValuatedMove * moves, Move * pv, threadData& tlData, int startWithMove = 0);
	//Quiescence Search (different implementations for positions in check and not in check)
	template<ThreadType T> Value QSearch(Value alpha, Value beta, position &pos, int depth, threadData& tlData);
	//Updates killer, history and counter move history tables, whenever a cutoff has happened
	void updateCutoffStats(threadData& tlData, const Move cutoffMove, int depth, position &pos, int moveIndex);
	//Thread id (0: MASTER or SINGLE, SLAVES are sequentially numbered starting with 1

};


template<ThreadType T> Value search::SearchRoot(Value alpha, Value beta, position &pos, int depth, ValuatedMove * moves, Move * pv, threadData& tlData, int startWithMove) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	pv[1] = MOVE_NONE; //pv[1] will be the ponder move, so we should make sure, that it is initialized as well
	bool lmr = !pos.Checked() && depth >= 3;
	bool ttFound;
	tt::Entry ttEntry;
	tt::Entry* ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	tt::NodeType nt = tt::NodeType::UPPER_BOUND;
	//move loop
	for (int i = startWithMove; i < rootMoveCount; ++i) {
		if (T != SLAVE) {
			//Send information about the current move to GUI
			if (UciOutput && PrintCurrmove && depth > 5 && (now() - timeManager.GetStartTime()) > 3000) {
				sync_cout << "info depth " << depth << " currmove " << toString(moves[i].move) << " currmovenumber " << i + 1 << sync_endl;
			}
			else if (XBoardOutput && timeManager.Mode() == INFINIT) {
				std::lock_guard<std::mutex> lck(mtxXAnalysisOutput);
				std::stringstream ss;
				ss << "stat01: " << (now() - timeManager.GetStartTime()) / 10 << " " << NodeCount << " "
					<< depth << " " << rootMoveCount - i << " " << rootMoveCount + 1 << " " << toString(moves[i].move);
				if (XAnalysisOutput != nullptr) delete(XAnalysisOutput);
				XAnalysisOutput = new std::string(ss.str());
			}
		}
		//apply move
		position next(pos);
		next.ApplyMove(moves[i].move);
		if (i > startWithMove) {
			int reduction = 0;
			//Lmr for root moves
			if (lmr && i >= startWithMove + 5 && pos.IsQuietAndNoCastles(moves[i].move) && !next.Checked()) {
				++reduction;
			}
			score = -Search<T>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction, subpv, tlData, true);
			if (reduction > 0 && score > alpha && score < beta) {
				score = -Search<T>(Value(-alpha - 1), -alpha, next, depth - 1, subpv, tlData, true);
			}
			if (score > alpha && score < beta) {
				//Research without reduction and with full alpha-beta window
				score = -Search<T>(-beta, -alpha, next, depth - 1, subpv, tlData, false);
			}
		}
		else {
			score = -Search<T>(-beta, -alpha, next, depth - 1, subpv, tlData, false);
		}
		if (Stopped()) break;
		moves[i].score = score;
		if (score > bestScore) {
			bestScore = score;
			pv[0] = moves[i].move;
			memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1) * sizeof(Move));
			if (i > 0) {
				//make sure that best move is always in first place 
				ValuatedMove bm = moves[i];
				for (int j = i; j > startWithMove; --j)moves[j] = moves[j - 1];
				moves[startWithMove] = bm;
			}
			if (score >= beta) {
				//Update transposition table
				if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::NodeType::LOWER_BOUND, depth, pv[0], VALUE_NOTYETDETERMINED);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::NodeType::LOWER_BOUND, depth, pv[0], VALUE_NOTYETDETERMINED);
				return score;
			}
			else if (score > alpha)
			{
				alpha = score;
				nt = tt::NodeType::EXACT;
			}
		}
	}
	//Update transposition table
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nt, depth, pv[0], VALUE_NOTYETDETERMINED);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nt, depth, pv[0], VALUE_NOTYETDETERMINED);
	return bestScore;
}

//This is the main alpha-beta search routine
template<ThreadType T> Value search::Search(Value alpha, Value beta, position &pos, int depth, Move * pv, threadData& tlData, bool cutNode, bool prune, Move excludeMove) {
	if (T != SLAVE) {
		if (depth > 0) {
			++NodeCount;
		}
		if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
	}
	if (Stopped()) return VALUE_ZERO;
#ifdef TRACE
	size_t traceIndex;
	if (depth > 0) {
		traceIndex = utils::addTraceEntry(NodeCount, pos.printPath(), depth, pos.GetHash());
	}
#endif
	if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	//Mate distance pruning
	alpha = Value(std::max(int(-VALUE_MATE) + pos.GetPliesFromRoot(), int(alpha)));
	beta = Value(std::min(int(VALUE_MATE) - pos.GetPliesFromRoot() - 1, int(beta)));
	if (alpha >= beta) return SCORE_MDP(alpha);
	//If depth = 0 is reached go to Quiescence Search
	if (depth <= 0) {
		return QSearch<T>(alpha, beta, pos, 0, tlData);
	}
	//TT lookup
	bool ttFound;
	tt::Entry ttEntry;
	tt::Entry* ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	Move ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		if (ttMove && pos.IsQuiet(ttMove) && pos.validateMove(ttMove)) updateCutoffStats(tlData, ttMove, depth, pos, -1);
		return SCORE_TT(ttValue);
	}
	// Tablebase probe
	// Probing is only done if root position was no tablebase position (probeTB is true) and if drawPlyCount = 0 (in that case we get the necessary WDL information) to return
	// immediately
	if (probeTB && pos.GetDrawPlyCount() == 0 && pos.GetMaterialTableEntry()->IsTablebaseEntry() && depth >= settings::parameter.TBProbeDepth)
	{
		int found;
		int v = Tablebases::probe_wdl(pos, &found);
		if (found)
		{
			tbHits++;
			Value value = v < -1 ? -VALUE_MATE + MAX_DEPTH + pos.GetPliesFromRoot()
				: v >  1 ? VALUE_MATE - MAX_DEPTH - pos.GetPliesFromRoot()
				: VALUE_DRAW + 2 * v;
			if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			return SCORE_TB(value);
		}
	}
	bool PVNode = (beta > alpha + 1);
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool checked = pos.Checked();
	Value staticEvaluation = checked ? VALUE_NOTYETDETERMINED :
		ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate() + settings::parameter.BONUS_TEMPO;
	prune = prune && !PVNode && !checked && (pos.GetLastAppliedMove() != MOVE_NONE) && (!pos.GetMaterialTableEntry()->SkipPruning());
	if (prune) {
		//Check if Value from TT is better
		// Beta Pruning
		if (depth < 7
			&& staticEvaluation < VALUE_KNOWN_WIN
			&& (staticEvaluation - settings::parameter.BetaPruningMargin(depth)) >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove()))
			return SCORE_BP(staticEvaluation - settings::parameter.BetaPruningMargin(depth));
		Value effectiveEvaluation = staticEvaluation;
		if (ttFound &&
			((ttValue > staticEvaluation && ttEntry.type() == tt::LOWER_BOUND)
				|| (ttValue < staticEvaluation && ttEntry.type() == tt::UPPER_BOUND))) effectiveEvaluation = ttValue;

		//Razoring
		if (depth < 4
			&& (staticEvaluation + settings::parameter.RazoringMargin(depth)) <= alpha
			&& ttMove == MOVE_NONE
			&& !pos.PawnOn7thRank())
		{
			//if (depth <= 1 && (effectiveEvaluation + settings::RazoringMargin(depth)) <= alpha) return QSearch(alpha, beta, pos, 0);
			Value razorAlpha = alpha - settings::parameter.RazoringMargin(depth);
			Value razorScore = QSearch<T>(razorAlpha, Value(razorAlpha + 1), pos, 0, tlData);
			if (razorScore <= razorAlpha) return SCORE_RAZ(razorScore);
		}

		//Null Move Pruning
		if (depth > 1 //only if there is available depth to reduce
			&& effectiveEvaluation >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove())
			&& excludeMove == MOVE_NONE
			) {
			int reduction = (depth + 14) / 5;
			if (int(effectiveEvaluation - beta) > int(settings::parameter.PieceValues[PAWN].egScore)) ++reduction;
			Square epsquare = pos.GetEPSquare();
			Move lastApplied = pos.GetLastAppliedMove();
			pos.NullMove();
			Value nullscore = -Search<T>(-beta, -beta + 1, pos, depth - reduction, subpv, tlData, !cutNode);
			pos.NullMove(epsquare, lastApplied);
			if (nullscore >= beta) {
				if (nullscore >= VALUE_MATE_THRESHOLD) nullscore = beta;
				if (depth < 9 && beta < VALUE_KNOWN_WIN) return SCORE_NMP(nullscore);
				// Do verification search at high depths
				Value verificationScore = Search<T>(beta - 1, beta, pos, depth - reduction, subpv, tlData, false, false);
				if (verificationScore >= beta) return SCORE_NMP(nullscore);
			}
		}

		// ProbCut (copied from SF)
		if (!PVNode
			&&  depth >= 5
			&& std::abs(int(beta)) < VALUE_MATE_THRESHOLD)
		{
			Value rbeta = std::min(Value(beta + 90), VALUE_INFINITE);
			int rdepth = depth - 4;

			position cpos(pos);
			cpos.copy(pos);
			Value limit = settings::parameter.PieceValues[GetPieceType(pos.getCapturedInLastMove())].mgScore;
			Move ttm = ttMove;
			if (ttm != MOVE_NONE && cpos.SEE(ttMove) < limit) ttm = MOVE_NONE;
			cpos.InitializeMoveIterator<QSEARCH>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, MOVE_NONE, ttm);
			Move move;
			while ((move = cpos.NextMove())) {
				position next(cpos);
				if (next.ApplyMove(move)) {
					Value score = -Search<T>(-rbeta, Value(-rbeta + 1), next, rdepth, subpv, tlData, !cutNode);
					if (score >= rbeta)
						return SCORE_PC(score);
				}
			}
		}
	}
	//Internal Iterative Deepening - it seems as IID helps as well if the found hash entry has very low depth
	int iidDepth = PVNode ? depth - 2 : depth / 2;
	if ((!ttMove || ttEntry.depth() < iidDepth) && (PVNode ? depth > 3 : depth > 6)) {
		position next(pos);
		next.copy(pos);
		//If there is no hash move, we are looking for a move => therefore search should is called with prune = false
		Search<T>(alpha, beta, next, iidDepth, subpv, tlData, cutNode, ttMove != MOVE_NONE);
		if (Stopped()) return VALUE_ZERO;
		ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
		ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	}
	if (!checked && ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED && pos.GetStaticEval() == VALUE_NOTYETDETERMINED) pos.SetStaticEval(ttEntry.evalValue() - settings::parameter.BONUS_TEMPO);
	Move counter = pos.GetCounterMove(counterMove);
	//Futility Pruning I: If quiet moves can't raise alpha, only generate tactical moves and moves which give check
	bool futilityPruning = pos.GetLastAppliedMove() != MOVE_NONE && !checked && depth <= settings::parameter.FULTILITY_PRUNING_DEPTH && beta < VALUE_MATE_THRESHOLD && pos.NonPawnMaterial(pos.GetSideToMove());
	if (futilityPruning && alpha > (staticEvaluation + settings::parameter.FUTILITY_PRUNING_LIMIT[depth]))
		pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, counter, ttMove);
	else
		pos.InitializeMoveIterator<MAIN_SEARCH>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, &tlData.killerManager, counter, ttMove);
	Value score;
	Value bestScore = -VALUE_MATE;
	tt::NodeType nodeType = tt::UPPER_BOUND;
	bool lmr = !checked && depth >= 3;
	Move move;
	int moveIndex = 0;
	bool ZWS = !PVNode;
	Square recaptureSquare = pos.GetLastAppliedMove() != MOVE_NONE && pos.Previous()->GetPieceOnSquare(to(pos.GetLastAppliedMove())) != BLANK ? to(pos.GetLastAppliedMove()) : OUTSIDE;
	bool trySE = depth >= 8 && ttMove != MOVE_NONE &&  abs(ttValue) < VALUE_KNOWN_WIN
		&& excludeMove == MOVE_NONE && (ttEntry.type() == tt::LOWER_BOUND || ttEntry.type() == tt::EXACT) && ttEntry.depth() >= depth - 3;
	while ((move = pos.NextMove())) {
		if (move == excludeMove) continue;
		int reducedDepth = lmr ? depth - settings::parameter.LMRReduction(depth, moveIndex) : depth;
		bool prunable = !PVNode && reducedDepth <= 4 && !checked && move != ttMove && move != counter && std::abs(int(bestScore)) <= VALUE_MATE_THRESHOLD
			&& !tlData.killerManager.isKiller(pos, move) && (pos.IsQuietAndNoCastles(move) || pos.GetMoveGenerationPhase() == MoveGenerationType::LOOSING_CAPTURES)
			&& !pos.IsAdvancedPawnPush(move) && !pos.givesCheck(move);
		if (prunable) {
			//assert(type(move) == MoveType::NORMAL && pos.GetPieceOnSquare(to(move)) == Piece::BLANK);
			// late-move pruning II
			if (moveIndex >= depth * 4) {
				moveIndex++;
				continue;
			}
			//SEE pruning 
			if (pos.SEE_Sign(move) < 0) {
				moveIndex++;
				continue;
			}
		}
		position next(pos);
		if (next.ApplyMove(move)) {
			tlData.killerManager.enterLevel(next);
			//critical = critical || GetPieceType(pos.GetPieceOnSquare(from(move))) == PAWN && ((pos.GetSideToMove() == WHITE && from(move) > H5) || (pos.GetSideToMove() == BLACK && from(move) < A4));
			//Check extension
			int extension;
			if (next.GetMaterialTableEntry()->Phase == 256 && pos.GetMaterialTableEntry()->Phase < 256 && next.GetMaterialScore() >= -settings::parameter.PieceValues[PAWN].egScore && next.GetMaterialScore() <= settings::parameter.PieceValues[PAWN].egScore)
				extension = 3;
			else extension = (next.Checked() && pos.SEE_Sign(move) >= VALUE_ZERO) ? 1 : 0;
			if (!extension && to(move) == recaptureSquare) {
				++extension;
			}
			if (trySE &&  move == ttMove && !extension)
			{
				Value rBeta = ttValue - 2 * depth;
				if (Search<T>(rBeta - 1, rBeta, pos, depth / 2, subpv, tlData, cutNode, true, move) < rBeta) ++extension;
			}
			if (!extension && moveIndex == 0 && pos.GeneratedMoveCount() == 1 && (pos.GetMoveGenerationPhase() == MoveGenerationType::CHECK_EVASION || pos.QuietMoveGenerationPhaseStarted())) {
				++extension;
			}
			int reduction = 0;
			//LMR: Late move reduction
			if (lmr && moveIndex != 0 && move != counter && pos.QuietMoveGenerationPhaseStarted()) {
				reduction = settings::parameter.LMRReduction(depth, moveIndex);
				if (cutNode) ++reduction;
				if ((PVNode || extension) && reduction > 0) --reduction;
			}
			if (ZWS) {
				score = -Search<T>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction + extension, subpv, tlData, !cutNode);
				if (score > alpha && reduction)
					score = -Search<T>(Value(-alpha - 1), -alpha, next, depth - 1 + extension, subpv, tlData, !cutNode);
				if (score > alpha && score < beta) {
					score = -Search<T>(-beta, -alpha, next, depth - 1 + extension, subpv, tlData, false);
				}
			}
			else {
				score = -Search<T>(-beta, -alpha, next, depth - 1 - reduction + extension, subpv, tlData, (PVNode ? false : !cutNode));
				if (score > alpha && reduction > 0) {
					score = -Search<T>(-beta, -alpha, next, depth - 1 + extension, subpv, tlData, (PVNode ? false : !cutNode));
				}
			}
			if (score >= beta) {
				updateCutoffStats(tlData, move, depth, pos, moveIndex);
				//Update transposition table
				if (T != SINGLE)  ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				return SCORE_BC(score);
			}
			ZWS = true;
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha)
				{
					nodeType = tt::EXACT;
					alpha = score;
					pv[0] = move;
					memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1) * sizeof(Move));
				}
			}
		}
		moveIndex++;
	}
	//Update transposition table
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	return SCORE_EXACT(bestScore);
}

template<ThreadType T> Value search::QSearch(Value alpha, Value beta, position &pos, int depth, threadData& tlData) {
	if (T != SLAVE) {
		++QNodeCount;
		++NodeCount;
#ifdef TRACE
		size_t traceIndex = utils::addTraceEntry(NodeCount, pos.printPath(), depth, pos.GetHash());
#endif
		MaxDepth = std::max(MaxDepth, pos.GetPliesFromRoot());
		if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
	}
	if (Stopped()) return VALUE_ZERO;
	if (pos.GetResult() != OPEN)  return SCORE_FINAL(pos.evaluateFinalPosition());
	//Mate distance pruning
	alpha = Value(std::max(int(-VALUE_MATE) + pos.GetPliesFromRoot(), int(alpha)));
	beta = Value(std::min(int(VALUE_MATE) - pos.GetPliesFromRoot() - 1, int(beta)));
	if (alpha >= beta) return SCORE_MDP(alpha);
	Move ttMove = MOVE_NONE;
#ifndef TUNE
	//TT lookup
	tt::Entry ttEntry;
	bool ttFound = false;
	Value ttValue = VALUE_NOTYETDETERMINED;
	tt::Entry* ttPointer;
	if (depth >= settings::parameter.LIMIT_QSEARCH_TT) {
		ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
		if (ttFound) ttValue = tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot());
		if (ttFound
			&& ttEntry.depth() >= depth
			&& ttValue != VALUE_NOTYETDETERMINED
			&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
			return SCORE_TT(ttValue);
		}
		if (ttFound && ttEntry.move()) ttMove = ttEntry.move();
	}
#endif
	bool WithChecks = depth == 0;
	bool checked = pos.Checked();
	Value standPat;
	if (checked) {
		standPat = VALUE_NOTYETDETERMINED;
	}
	else {
#ifdef TUNE
		standPat = pos.evaluate() + settings::parameter.BONUS_TEMPO;
#else
		standPat = ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate() + settings::parameter.BONUS_TEMPO;
		//check if ttValue is better
		if (ttFound && ttValue != VALUE_NOTYETDETERMINED && ((ttValue > standPat && ttEntry.type() == tt::LOWER_BOUND) || (ttValue < standPat && ttEntry.type() == tt::UPPER_BOUND))) standPat = ttValue;
		if (standPat >= beta) {
			if (depth >= settings::parameter.LIMIT_QSEARCH_TT) {
				if (T == SINGLE) ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, MOVE_NONE, standPat);
				else ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, MOVE_NONE, standPat);
			}
			return SCORE_SP(beta);
		}
#endif
		//Delta Pruning
		if (!checked && !pos.GetMaterialTableEntry()->IsLateEndgame()) {
			Value delta = settings::parameter.PieceValues[pos.GetMostValuableAttackedPieceType()].egScore + int(pos.PawnOn7thRank()) * (settings::parameter.PieceValues[QUEEN].egScore - settings::parameter.PieceValues[PAWN].egScore) + settings::parameter.DELTA_PRUNING_SAFETY_MARGIN;
			if (standPat + delta < alpha) return SCORE_DP(alpha);
		}
		if (alpha < standPat) alpha = standPat;
	}
	if (WithChecks) pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, MOVE_NONE, ttMove);
	else pos.InitializeMoveIterator<QSEARCH>(&tlData.History, &tlData.cmHistory, &tlData.followupHistory, nullptr, MOVE_NONE, ttMove);
	Move move;
	Value score;
	tt::NodeType nt = tt::UPPER_BOUND;
	Move bestMove = MOVE_NONE;
	while ((move = pos.NextMove())) {
		if (!checked) {
			if (depth > settings::parameter.LIMIT_QSEARCH) {
				if (pos.SEE_Sign(move) < VALUE_ZERO) continue;
			}
			else return standPat + pos.SEE(move);
		}
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<T>(-beta, -alpha, next, depth - 1, tlData);
			if (score >= beta) {
#ifndef TUNE
				if (depth >= settings::parameter.LIMIT_QSEARCH_TT) {
					if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, move, standPat);
					else ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, move, standPat);
				}
#endif
				return SCORE_BC(beta);
			}
			if (score > alpha) {
				nt = tt::EXACT;
				alpha = score;
				bestMove = move;
			}
		}
	}
#ifndef TUNE
	if (depth >= settings::parameter.LIMIT_QSEARCH_TT) {
		if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), alpha, nt, depth, bestMove, standPat);
		else ttPointer->update<tt::UNSAFE>(pos.GetHash(), alpha, nt, depth, bestMove, standPat);
	}
#endif
	return SCORE_EXACT(alpha);
}