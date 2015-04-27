#pragma once

#include "types.h"
#include "polyglotbook.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

enum NodeType { PV, NULL_MOVE, EXPECTED_CUT_NODE, QSEARCH_DEPTH_0, QSEARCH_DEPTH_NEGATIVE };

enum ThreadType { SINGLE, MASTER, SLAVE };

struct baseSearch {
public:
	bool UciOutput = false;
	bool PonderMode = false;
	ValuatedMove BestMove;
	int64_t NodeCount;
	int64_t QNodeCount;
	int MaxDepth;
	bool Stop;
	double BranchingFactor = 0;
	std::string BookFile = "";
	int rootMoveCount;
	ValuatedMove * rootMoves;
	position rootPosition;
	std::vector<Move> searchMoves;
	int MultiPv = 1;

	baseSearch();
	~baseSearch();
	void Reset();
	std::string PrincipalVariation(int depth = PV_MAX_LENGTH);
	void NewGame();
	inline int Depth() const { return _depth; }
	inline int64_t ThinkTime() const { return _thinkTime; }
	inline double cutoffAt1stMoveRate() const { return 100.0 * cutoffAt1stMove / cutoffCount; }
	inline double cutoffAverageMove() const { return 1 + 1.0 * cutoffMoveIndexSum / cutoffCount; }
	inline Move PonderMove() const { return PVMoves[1]; }
	inline void ExtendStopTimes(int64_t extensionTime) { searchStopCriteria.HardStopTime += extensionTime; searchStopCriteria.SoftStopTime += extensionTime; }
	inline void StopThinking(){ Stop = true; searchStopCriteria.HardStopTime = 0; }
	Move GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount);
	virtual ThreadType GetType() = 0;

	//protected:
	polyglot::polyglotbook book;
	DblHistoryStats DblHistory;
	uint64_t cutoffAt1stMove = 0;
	uint64_t cutoffCount = 0;
	uint64_t cutoffMoveIndexSum = 0;
	HistoryStats History;
	ExtendedMove killer[2 * MAX_DEPTH];
	//Move excludedMoves[MAX_DEPTH];
	SearchStopCriteria searchStopCriteria;
	Move PVMoves[PV_MAX_LENGTH];
	int _depth = 0;
	int64_t _thinkTime;
	Move counterMove[12 * 64];
	Value gains[12 * 64];
	int instance;

	inline bool Stopped() { return Stop && (!PonderMode); }


};

template<ThreadType T> struct search : public baseSearch {
public:
	baseSearch * master = nullptr;

	search();
	~search();
	inline ValuatedMove Think(position &pos, SearchStopCriteria ssc);

	void startHelper();
	void initializeSlave(baseSearch * masterSearch);
	ThreadType GetType() { return T; }

private:
	template<NodeType NT> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv, bool prune = true);
	Value SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv, int startWithMove = 0);
	template<NodeType NT> Value QSearch(Value alpha, Value beta, position &pos, int depth);
	void updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex);

};



void startThread(search<SLAVE> & slave);

template<ThreadType T> inline ValuatedMove search<T>::Think(position &pos, SearchStopCriteria ssc) {
	std::vector<std::thread> subThreads;
	search<SLAVE> * slaves = nullptr;
	searchStopCriteria = ssc; 
	_thinkTime = 1;
	rootPosition = pos;
	int64_t * nodeCounts = new int64_t[searchStopCriteria.MaxDepth + 1];
	nodeCounts[0] = 0;
	pos.ResetPliesFromRoot();
	tt::newSearch();
	ValuatedMove * generatedMoves = pos.GenerateMoves<LEGAL>();
	rootMoveCount = pos.GeneratedMoveCount();
	if (rootMoveCount == 1){
		BestMove = *generatedMoves; //if there is only one legal move save time and return move immediately (although there is no score assigned)
		goto END;
	}
	//check for book
	if (USE_BOOK && BookFile.size() > 0) {
		Move bookMove = book.probe(pos, BookFile, false, generatedMoves, rootMoveCount);
		if (bookMove != MOVE_NONE) {
			BestMove.move = bookMove;
			BestMove.score = VALUE_ZERO;
			if (UciOutput) std::cout << "info string book move" << std::endl;
			goto END;
		}
	}
	if (searchMoves.size()) {
		rootMoveCount = int(searchMoves.size());
		rootMoves = new ValuatedMove[rootMoveCount];
		for (int i = 0; i < rootMoveCount; ++i) rootMoves[i].move = searchMoves[i];
		searchMoves.clear();
	}
	else {
		rootMoves = new ValuatedMove[rootMoveCount];
		memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	}
	std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	if (T == MASTER) {
		slaves = new search<SLAVE>[HelperThreads];
		for (int i = 0; i < HelperThreads; ++i) {
			slaves[i].initializeSlave(this);
			subThreads.push_back(std::thread(&startThread, std::ref(slaves[i])));
		}
	}
	//Iterativ Deepening Loop
	for (_depth = 1; _depth <= searchStopCriteria.MaxDepth; ++_depth) {
		for (int pvIndx = 0; pvIndx < MultiPv; ++pvIndx) {
			Value alpha = -VALUE_MATE;
			Value beta = VALUE_MATE;
			SearchRoot(alpha, beta, pos, _depth, &PVMoves[0], pvIndx);
			std::stable_sort(rootMoves+pvIndx, &rootMoves[rootMoveCount], sortByScore);
			BestMove = rootMoves[0];
			int64_t tNow = now();
			_thinkTime = std::max(tNow - searchStopCriteria.StartTime, int64_t(1));
			if (!Stopped()) {
				nodeCounts[_depth] = NodeCount - nodeCounts[_depth - 1];
				if (_depth > 3) BranchingFactor = sqrt(1.0 * nodeCounts[_depth] / nodeCounts[_depth - 2]);
				if (!PonderMode && (tNow > searchStopCriteria.SoftStopTime || (3 * (tNow - searchStopCriteria.StartTime) + searchStopCriteria.StartTime) > searchStopCriteria.SoftStopTime)) Stop = true;
			}
			if (Stopped() || (UciOutput && _thinkTime > 200)) {
				if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
					std::cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score cp " << BestMove.score << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
					<< " hashfull " << tt::GetHashFull()
					//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
					<< " pv " << PrincipalVariation(_depth) << std::endl;
				else {
					int pliesToMate;
					if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
					std::cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score mate " << pliesToMate / 2 << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
						<< " hashfull " << tt::GetHashFull()
						//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
						<< " pv " << PrincipalVariation(_depth) << std::endl;
				}
			}
			if (Stopped()) break;
		}
		if (Stopped()) break;
	}

	if (T == MASTER) {
		for (int i = 0; i < HelperThreads; ++i) slaves[i].StopThinking();
		for (int i = 0; i < HelperThreads; ++i) subThreads[i].join();
		delete[] slaves;
	}

END:	while (PonderMode) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	delete[] nodeCounts;
	return BestMove;
}

template<ThreadType T> void search<T>::initializeSlave(baseSearch * masterSearch) {
	if (T == SLAVE) {
		master = masterSearch;
		rootMoveCount = masterSearch->rootMoveCount;
		rootMoves = new ValuatedMove[rootMoveCount];
		memcpy(rootMoves, masterSearch->rootMoves, rootMoveCount * sizeof(ValuatedMove));
		rootPosition = position(masterSearch->rootPosition);
		rootPosition.copy(masterSearch->rootPosition);
		rootPosition.InitMaterialPointer();
		//this->DblHistory = masterSearch->DblHistory;
		Stop = false;
	}
}

template<ThreadType T> void search<T>::startHelper() {
	int depth = 1;
	std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	while (!Stop && depth <= MAX_DEPTH) {
		Value alpha = -VALUE_MATE;
		Value beta = VALUE_MATE;
		SearchRoot(alpha, beta, rootPosition, depth, &PVMoves[0]);
		std::stable_sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
		++depth;
	}
	delete[] rootMoves;
}

template<ThreadType T> search<T>::search() {  }

template<ThreadType T> search<T>::~search() {

}

template<ThreadType T> Value search<T>::SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv, int startWithMove = 0) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Value bonus;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool ZWS = false;
	//	int repetitions = T == SINGLE ? 1 : 1;
	//	for (int repetition = 0; repetition < repetitions; ++repetition){
	for (int i = startWithMove; i < rootMoveCount; ++i) {
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		//Check if other thread is already processing this node 
		//if (T != SINGLE) {
		//	if (repetition == 0 && i > 0 && tt::GetNProc(next.GetHash())> 0) continue; else tt::IncrementNProc(next.GetHash());
		//}
		if (type(rootMoves[i].move) == CASTLING) bonus = BONUS_CASTLING; else bonus = VALUE_ZERO;
		if (ZWS) {
			score = bonus - Search<EXPECTED_CUT_NODE>(Value(bonus - alpha - 1), bonus - alpha, next, depth - 1, &subpv[0]);
			if (score > alpha && score < beta) score = bonus - Search<PV>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0]);
		}
		else score = bonus - Search<PV>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0]);
		//if (T != SINGLE) tt::DecrementNProc(next.GetHash());
		if (Stopped()) break;
		rootMoves[i].score = score;
		if (score >= beta) {
			return score;
		}
		if (score > bestScore) {
			bestScore = score;
			if (score > alpha)
			{
				ZWS = true && depth > 2;
				alpha = score;
				pv[0] = rootMoves[i].move;
				memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
			}
		}
	}
	//	}
	return bestScore;
}

template<ThreadType T> template<NodeType NT> Value search<T>::Search(Value alpha, Value beta, position &pos, int depth, Move * pv, bool prune = true) {
	++NodeCount;
	if (T != SLAVE) {
		Stop = !PonderMode && (Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime));
	}
	if (Stopped()) return VALUE_ZERO;
	if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	//Mate distance pruning
	alpha = Value(std::max(int(-VALUE_MATE) + pos.GetPliesFromRoot(), int(alpha)));
	beta = Value(std::min(int(VALUE_MATE) - pos.GetPliesFromRoot() - 1, int(beta)));
	if (alpha >= beta) return alpha;
	if (depth <= 0) {
		return QSearch<QSEARCH_DEPTH_0>(alpha, beta, pos, depth);
	}
	//TT lookup
	bool ttFound;
	tt::Entry ttEntry;
	tt::Entry* ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		//if (ttValue > beta && ttEntry->move != MOVE_NONE && pos.IsQuiet(ttEntry->move)) updateCutoffStats(ttEntry->move, depth, pos, 0);
		return ttValue;
	}
	Move ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool checked = pos.Checked();
	Value staticEvaluation = checked ? VALUE_NOTYETDETERMINED :
		ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate();
	if (prune && !checked) {
		//Check if Value from TT is better
		Value effectiveEvaluation = staticEvaluation;
		if (ttFound &&
			((ttValue > staticEvaluation && ttEntry.type() == tt::LOWER_BOUND)
			|| (ttValue < staticEvaluation && ttEntry.type() == tt::UPPER_BOUND))) effectiveEvaluation = ttValue;

		//Razoring a la SF (no measurable ELO change)
		/*if (!checked && depth < 4
			&& (effectiveEvaluation + Value(512 + 32 * depth)) <= alpha
			&& ttMove == MOVE_NONE
			&& !pos.PawnOn7thRank())
			{
			if (depth <= 1 && (effectiveEvaluation + 608) <= alpha) return QSearch<QSEARCH_DEPTH_0>(alpha, beta, pos, 0);
			Value ralpha = alpha - Value(512 + 32 * depth);
			Value v = QSearch<QSEARCH_DEPTH_0>(ralpha, Value(ralpha + 1), pos, 0);
			if (v <= ralpha) return v;
			}*/
		// Beta Pruning
		if (depth < 7
			&& effectiveEvaluation < VALUE_KNOWN_WIN
			&& (effectiveEvaluation - BETA_PRUNING_FACTOR * depth) >= beta
			//&& !pos.GetMaterialTableEntry()->IsLateEndgame()
			&& pos.NonPawnMaterial(pos.GetSideToMove()))
			return effectiveEvaluation - BETA_PRUNING_FACTOR * depth;
		//Null Move Pruning
		if (NT != NULL_MOVE && effectiveEvaluation > beta && depth > 4 && !pos.GetMaterialTableEntry()->IsLateEndgame() && pos.NonPawnMaterial(pos.GetSideToMove())) {
			int reduction = depth >> 1;
			Square epsquare = pos.GetEPSquare();
			pos.NullMove();
			Value nullscore = -Search<NULL_MOVE>(-beta, -alpha, pos, depth - reduction - 1, &subpv[0], false);
			pos.NullMove(epsquare);
			if (nullscore >= beta) {
				return beta;
			}
			//{
			//	if (nullscore > VALUE_MATE_THRESHOLD) nullscore = beta;
			//	if (depth < 12 && abs(beta) < VALUE_KNOWN_WIN) return nullscore;

			//	//Verification Search
			//	Value verificationValue = Search<NULL_MOVE>(beta - 1, beta, pos, depth - reduction, &subpv[0]);
			//	if (verificationValue >= beta) return nullscore;
			//}
		}
		if (pos.IsQuiet(pos.GetLastAppliedMove())) {
			Square lastToSquare = to(pos.GetLastAppliedMove());
			int gainIndx = (pos.GetPieceOnSquare(lastToSquare) << 6) + lastToSquare;
			gains[gainIndx] = std::max(staticEvaluation - pos.Previous()->GetStaticEval(), gains[gainIndx] - 1);
		}

		// ProbCut (copied from SF)
		if (NT != PV
			&&  depth >= 5
			&& std::abs(beta) < VALUE_MATE_THRESHOLD)
		{
			Value rbeta = std::min(Value(beta + 90), VALUE_INFINITE);
			int rdepth = depth - 4;

			position cpos(pos);
			cpos.copy(pos);
			Value limit = PieceValuesMG[GetPieceType(pos.getCapturedInLastMove())];
			Move ttm = ttMove;
			if (ttm != MOVE_NONE && cpos.SEE(from(ttMove), to(ttMove)) < limit) ttm = MOVE_NONE;
			cpos.InitializeMoveIterator<QSEARCH>(&History, &DblHistory, nullptr, nullptr, ttm, limit);
			Move move;
			while ((move = cpos.NextMove())) {
				position next(cpos);
				if (next.ApplyMove(move)) {
					Value score = -Search<EXPECTED_CUT_NODE>(-rbeta, Value(-rbeta + 1), next, rdepth, &subpv[0]);
					if (score >= rbeta)
						return score;
				}
			}
		}

		//IID (Seems to be neutral => therefore deactivated
		if (NT == PV && depth >= 3 && !ttMove) {
			position next(pos);
			next.copy(pos);
			Search<NT>(alpha, beta, next, depth - 2, &subpv[0], false);
			if (Stopped()) return VALUE_ZERO;
			ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
			ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
		}
	}
	//Futility Pruning: Prune quiet moves that give no check if they can't raise alpha
	bool futilityPruning = NT != NULL_MOVE && !checked && depth <= FULTILITY_PRUNING_DEPTH && beta < VALUE_MATE_THRESHOLD && pos.NonPawnMaterial(pos.GetSideToMove());
	if (futilityPruning && alpha >(staticEvaluation + FUTILITY_PRUNING_LIMIT[depth]))
		pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, &DblHistory, nullptr, counterMove, ttMove);
	else
		pos.InitializeMoveIterator<MAIN_SEARCH>(&History, &DblHistory, &killer[2 * pos.GetPliesFromRoot()], counterMove, ttMove);
	Value score;
	Value bestScore = -VALUE_MATE;
	tt::NodeType nodeType = tt::UPPER_BOUND;
	bool lmr = !checked && depth >= 3;
	Move move;
	int moveIndex = 0;
	bool ZWS = false;
	int validMoveCount = 0;
	//	bool singularExtensionNode = depth >= 8 && ttMove != MOVE_NONE &&  std::abs(ttValue) < VALUE_KNOWN_WIN
	//		&& excludedMoves[pos.GetPliesFromRoot()] == MOVE_NONE && ttEntry.type() == tt::LOWER_BOUND && ttEntry.depth() >= depth - 3;
	//int repetitions = T == SINGLE ? 1 : 2;
	//for (int repetition = 0; repetition < repetitions; ++repetition) {
	//if (T != SINGLE && repetition > 0) pos.InitializeMoveIterator<REPETITION>(&History, &DblHistory, &killer[2 * pos.GetPliesFromRoot()], counterMove, ttMove);
	while ((move = pos.NextMove())) {
		//		if (move == excludedMoves[pos.GetPliesFromRoot()]) continue;
		if (futilityPruning
			&& moveIndex > 0
			&& type(move) != PROMOTION
			&& (staticEvaluation + FUTILITY_PRUNING_MARGIN[depth]
			+ gains[(pos.GetPieceOnSquare(from(move)) << 6) + to(move)]
			+ PieceValuesMG[GetPieceType(pos.GetPieceOnSquare(to(move)))]) <= alpha
			) continue;
		if (NT != PV && depth <= 3 && moveIndex >= depth * 4 && std::abs(bestScore) <= VALUE_MATE_THRESHOLD  && pos.IsQuietAndNoCastles(move)) { // late-move pruning
			continue;
		}
		position next(pos);
		if (next.ApplyMove(move)) {
			//Check if other thread is already processing this node 
			/*if (T != SINGLE) {
				if (repetition == 0 && moveIndex > 0 && tt::GetNProc(next.GetHash()) > 0) continue; else tt::IncrementNProc(next.GetHash());
				}*/
			int extension = (next.Checked() && pos.SEE_Sign(move) >= VALUE_ZERO) ? 1 : 0;

			//Singular extension search. If all moves but one fail low on a search of
			//(alpha-s, beta-s), and just one fails high on (alpha, beta), then that move
			//is singular and should be extended. To verify this we do a reduced search
			//on all the other moves but the ttMove and if the result is lower than
			//ttValue minus a margin then we extend the ttMove.
			//if (singularExtensionNode
			//	&&  move == ttMove
			//	&& !extension)
			//{
			//	Value rBeta = ttValue - 2 * depth;
			//	excludedMoves[pos.GetPliesFromRoot()] = move;
			//	Value score = Search<EXPECTED_CUT_NODE>(Value(rBeta - 1), rBeta, pos, depth / 2, &subpv[0], false);
			//	excludedMoves[pos.GetPliesFromRoot()] = MOVE_NONE;
			//	if (score < rBeta)
			//		extension = 1;
			//}
			int reduction = 0;
			if (lmr && moveIndex >= 2 && !extension && pos.IsQuietAndNoCastles(move) && !next.Checked()) {
				if (NT == PV) {
					if (moveIndex >= 5) reduction = 1;
				}
				else {
					if (moveIndex >= 5) reduction = depth / 3; else reduction = 1;
				}
			}
			//Prune moves with negative sign
			//if ( type(move) == NORMAL
			//	&&  bestScore > -VALUE_MATE_THRESHOLD
			//	&& (depth - 1 - reduction + extension) < 4
			//	&& !next.Checked()
			//	&& pos.IsQuiet(move)				
			//	&& !pos.IsAdvancedPawnMove(move)
			//	&& pos.SEE(from(move), to(move)) < VALUE_ZERO) continue;
			if (ZWS) {
				score = -Search<EXPECTED_CUT_NODE>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction + extension, &subpv[0]);
				if (score > alpha && score < beta) score = -Search<PV>(-beta, -alpha, next, depth - 1 + extension, &subpv[0]);
			}
			else {
				score = -Search<PV>(-beta, -alpha, next, depth - 1 - reduction + extension, &subpv[0]);
				if (score > alpha && reduction > 0) score = -Search<PV>(-beta, -alpha, next, depth - 1 + extension, &subpv[0]);
			}
			//if (T != SINGLE) tt::DecrementNProc(next.GetHash());
			if (score >= beta) {
				updateCutoffStats(move, depth, pos, moveIndex);
				if (T != SINGLE)  ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				return score;
			}
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha)
				{
					ZWS = true;
					nodeType = tt::EXACT;
					alpha = score;
					pv[0] = move;
					memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
				}
			}
		}
		moveIndex++;
	}
	//}
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	return bestScore;
}

template<ThreadType T> template<NodeType NT> Value search<T>::QSearch(Value alpha, Value beta, position &pos, int depth) {
	++QNodeCount;
	if (NT == QSEARCH_DEPTH_NEGATIVE) {
		MaxDepth = std::max(MaxDepth, pos.GetPliesFromRoot());
		++NodeCount;
		if (T == MASTER) {
			Stop = !PonderMode && (Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime));
		}
		if (Stopped()) return VALUE_ZERO;
		if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	}
	//TT lookup
	tt::Entry ttEntry;
	bool ttFound;
	tt::Entry* ttPointer;
	ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		return ttValue;
	}
	bool checked = pos.Checked();
	int ttDepth = NT == QSEARCH_DEPTH_0 || checked ? 0 : -1;
	Value standPat;
	if (checked) {
		standPat = VALUE_NOTYETDETERMINED;
	}
	else {
		standPat = ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate();
		//check if ttValue is better
		if (ttFound && ttValue != VALUE_NOTYETDETERMINED && ((ttValue > standPat && ttEntry.type() == tt::LOWER_BOUND) || (ttValue < standPat && ttEntry.type() == tt::UPPER_BOUND))) standPat = ttValue;
		if (standPat >= beta) {
			if (T == SINGLE) ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, MOVE_NONE, standPat);
			else ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, MOVE_NONE, standPat);
			return beta;
		}
		//Delta Pruning
		if (!pos.GetMaterialTableEntry()->IsLateEndgame()) {
			Value delta = PieceValuesEG[pos.GetMostValuablePieceType(~pos.GetSideToMove())] + int(pos.PawnOn7thRank()) * (PieceValuesEG[QUEEN] - PieceValuesEG[PAWN]) + DELTA_PRUNING_SAFETY_MARGIN;
			if (standPat + delta < alpha) return alpha;
		}
		if (alpha < standPat) alpha = standPat;
	}
	if (NT == QSEARCH_DEPTH_0) pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, &DblHistory, nullptr, nullptr);
	else pos.InitializeMoveIterator<QSEARCH>(&History, &DblHistory, nullptr, nullptr);
	Move move;
	Value score;
	tt::NodeType nt = tt::UPPER_BOUND;
	while ((move = pos.NextMove())) {
		if (pos.SEE_Sign(move) < VALUE_ZERO) continue;
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<QSEARCH_DEPTH_NEGATIVE>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, move, standPat);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, move, standPat);
				return beta;
			}
			if (score > alpha) {
				nt = tt::EXACT;
				alpha = score;
			}
		}
	}
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), alpha, nt, ttDepth, MOVE_NONE, standPat);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), alpha, nt, ttDepth, MOVE_NONE, standPat);
	return alpha;
}

template<ThreadType T> void search<T>::updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex) {
	cutoffAt1stMove += moveIndex == 0;
	cutoffCount++;
	cutoffMoveIndexSum += moveIndex;
	if (pos.GetMoveGenerationType() >= QUIETS_POSITIVE) {
		Square toSquare = to(cutoffMove);
		Piece movingPiece = pos.GetPieceOnSquare(from(cutoffMove));
		ExtendedMove killerMove(movingPiece, cutoffMove);
		int pfr = pos.GetPliesFromRoot();

		killer[2 * pfr + 1] = killer[2 * pfr];
		killer[2 * pfr] = killerMove;
		Value v = Value(depth * depth);
		History.update(v, movingPiece, toSquare);
		Piece prevPiece = BLANK;
		Square prevTo = OUTSIDE;
		if (pos.GetLastAppliedMove()) {
			prevTo = to(pos.GetLastAppliedMove());
			prevPiece = pos.GetPieceOnSquare(prevTo);
			counterMove[(pos.GetPieceOnSquare(prevTo) << 6) + prevTo] = cutoffMove;
			DblHistory.update(v, prevPiece, prevTo, movingPiece, toSquare);
		}
		if (moveIndex > 0) {
			ValuatedMove * alreadyProcessedQuiets = pos.GetMovesOfCurrentPhase();
			for (int i = 0; i < pos.GetMoveNumberInPhase() - 1; ++i) {
				movingPiece = pos.GetPieceOnSquare(from(alreadyProcessedQuiets->move));
				toSquare = to(alreadyProcessedQuiets->move);
				History.update(-v, movingPiece, toSquare);
				if (pos.GetLastAppliedMove())
					DblHistory.update(-v, prevPiece, prevTo, movingPiece, toSquare);
				alreadyProcessedQuiets++;
			}
		}
	}
}