#pragma once

#include "types.h"
#include "book.h"
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
	bool PrintCurrmove = true;
	bool PonderMode = false;
	ValuatedMove BestMove;
	int64_t NodeCount = 0;
	int64_t QNodeCount = 0;
	int MaxDepth = 0;
	bool Stop = false;
	double BranchingFactor = 0;
	std::string BookFile = "";
	int rootMoveCount = 0;
	ValuatedMove * rootMoves = nullptr;
	position rootPosition;
	std::vector<Move> searchMoves;
	int MultiPv = 1;

	baseSearch();
	virtual ~baseSearch();
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
	polyglot::book * book = nullptr;
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
		if (book == nullptr) book = new polyglot::book(BookFile);
		Move bookMove = book->probe(pos, false, generatedMoves, rootMoveCount);
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
			//Best move is already in first place, this is assured by SearchRoot
			//therefore we sort only the other moves
			std::stable_sort(rootMoves + pvIndx + 1, &rootMoves[rootMoveCount], sortByScore);
			BestMove = rootMoves[0];
			int64_t tNow = now();
			_thinkTime = std::max(tNow - searchStopCriteria.StartTime, int64_t(1));
			if (!Stopped()) {
				nodeCounts[_depth] = NodeCount - nodeCounts[_depth - 1];
				if (_depth > 3) BranchingFactor = sqrt(1.0 * nodeCounts[_depth] / nodeCounts[_depth - 2]);
				if (!PonderMode && tNow >= searchStopCriteria.MinStopTime && (tNow > searchStopCriteria.SoftStopTime || (3 * (tNow - searchStopCriteria.StartTime) + searchStopCriteria.StartTime) > searchStopCriteria.SoftStopTime)) Stop = true;
			}
			if (Stopped() || UciOutput) {
				if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
					std::cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score cp " << BestMove.score << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
					<< " hashfull " << tt::GetHashFull()
					<< " time " << _thinkTime
					//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
					<< " pv " << PrincipalVariation(_depth) << std::endl;
				else {
					int pliesToMate;
					if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
					std::cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score mate " << pliesToMate / 2 << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
						<< " hashfull " << tt::GetHashFull()
						<< " time " << _thinkTime
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

template<ThreadType T> Value search<T>::SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv, int startWithMove) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Value bonus;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool ZWS = false;
	bool lmr = !pos.Checked() && depth >= 3;
	//	int repetitions = T == SINGLE ? 1 : 1;
	//	for (int repetition = 0; repetition < repetitions; ++repetition){
	for (int i = startWithMove; i < rootMoveCount; ++i) {
		if (PrintCurrmove && depth > 5 && (now() - searchStopCriteria.StartTime) > 3000) {
			std::cout << "info depth " << depth << " currmove " << toString(rootMoves[i].move) << " currmovenumber " << i + 1 << std::endl;
		}
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		//Check if other thread is already processing this node
		//if (T != SINGLE) {
		//	if (repetition == 0 && i > 0 && tt::GetNProc(next.GetHash())> 0) continue; else tt::IncrementNProc(next.GetHash());
		//}
		int reduction = 0;
		if (lmr && i >= startWithMove + 5 && pos.IsQuietAndNoCastles(rootMoves[i].move) && !next.Checked()) {
			reduction = 1;
		}
		if (type(rootMoves[i].move) == CASTLING) bonus = BONUS_CASTLING; else bonus = VALUE_ZERO;
		if (ZWS) {
			score = bonus - Search<EXPECTED_CUT_NODE>(Value(bonus - alpha - 1), bonus - alpha, next, depth - 1 - reduction, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			if (score > alpha && score < beta) score = bonus - Search<PV>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
		}
		else {
			score = bonus - Search<PV>(bonus - beta, bonus - alpha, next, depth - 1 - reduction, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			if (reduction > 0 && score > alpha && score < beta) score = bonus - Search<PV>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
		}
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
				if (i > 0) {
					//make sure that best move is always in first place 
					ValuatedMove bm = rootMoves[i];
					for (int j = i; j > 0; --j) rootMoves[j] = rootMoves[j - 1];
					rootMoves[0] = bm;
				}
			}
		}
	}
	//	}
	return bestScore;
}

template<ThreadType T> template<NodeType NT> Value search<T>::Search(Value alpha, Value beta, position &pos, int depth, Move * pv, bool prune) {
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
		return ttValue;
	}
	Move ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool checked = pos.Checked();
	Value staticEvaluation = checked ? VALUE_NOTYETDETERMINED :
		ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate();
	if (NT != NULL_MOVE && prune && !checked) {
		//Check if Value from TT is better
		Value effectiveEvaluation = staticEvaluation;
		if (ttFound &&
			((ttValue > staticEvaluation && ttEntry.type() == tt::LOWER_BOUND)
			|| (ttValue < staticEvaluation && ttEntry.type() == tt::UPPER_BOUND))) effectiveEvaluation = ttValue;

		//Razoring a la SF 
		if (!checked && depth < 4
			&& (effectiveEvaluation + Value(256 + 16 * depth)) <= alpha
			&& ttMove == MOVE_NONE
			&& !pos.PawnOn7thRank())
		{
			if (depth <= 1 && (effectiveEvaluation + 302) <= alpha) return QSearch<QSEARCH_DEPTH_0>(alpha, beta, pos, 0);
			Value razorAlpha = alpha - Value(256 + 16 * depth);
			Value razorScore = QSearch<QSEARCH_DEPTH_0>(razorAlpha, Value(razorAlpha + 1), pos, 0);
			if (razorScore <= razorAlpha) return razorScore;
		}
		// Beta Pruning
		if (depth < 7
			&& effectiveEvaluation < VALUE_KNOWN_WIN
			&& (effectiveEvaluation - BETA_PRUNING_FACTOR * depth) >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove()))
			return effectiveEvaluation - BETA_PRUNING_FACTOR * depth;
		//Null Move Pruning
		if (depth > 1 //only if there is available depth to reduce
			&& effectiveEvaluation >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove())
			) {
			int reduction = (depth >> 1) + 1;
			Square epsquare = pos.GetEPSquare();
			pos.NullMove();
			Value nullscore = -Search<NULL_MOVE>(-beta, -alpha, pos, depth - reduction, &subpv[0], false);
			pos.NullMove(epsquare);
			if (nullscore >= beta) {
				return beta;
			}
		}

		// ProbCut (copied from SF)
		if (NT != PV
			&&  depth >= 5
			&& std::abs(int(beta)) < VALUE_MATE_THRESHOLD)
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
					Value score = -Search<EXPECTED_CUT_NODE>(-rbeta, Value(-rbeta + 1), next, rdepth, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
					if (score >= rbeta)
						return score;
				}
			}
		}

		//IID: No Hash move available => Try to find one with a reduced search
		if (NT == PV && depth >= 3 && !ttMove) {
			position next(pos);
			next.copy(pos);
			Search<NT>(alpha, beta, next, depth - 2, &subpv[0], false);
			if (Stopped()) return VALUE_ZERO;
			ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
			ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
		}
	}
	//Futility Pruning I: If quiet moves can't raise alpha, only generate tactical moves and moves which give check
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
	Square recaptureSquare = pos.GetLastAppliedMove() && pos.Previous()->GetPieceOnSquare(to(pos.GetLastAppliedMove())) != BLANK ? to(pos.GetLastAppliedMove()) : OUTSIDE;
	while ((move = pos.NextMove())) {
		// late-move pruning
		if (NT != PV && depth <= 3 && moveIndex >= depth * 4 && std::abs(int(bestScore)) <= VALUE_MATE_THRESHOLD  && pos.IsQuietAndNoCastles(move)) {
			moveIndex++;
			continue;
		}
		position next(pos);
		if (next.ApplyMove(move)) {
			//Check extension
			int extension = (next.Checked() && pos.SEE_Sign(move) >= VALUE_ZERO) ? 1 : 0;
			if (!extension && to(move) == recaptureSquare
				//&& (PieceValuesMG[next.GetPieceOnSquare(to(move))] < PieceValuesMG[pos.GetPieceOnSquare(to(move))])
				) {
				++extension;
			}
			int reduction = 0;
			//LMR: Late move reduction
			if (lmr && moveIndex >= 2 && !extension && pos.IsQuietAndNoCastles(move) && !next.Checked()) {
				if (NT == PV) {
					if (moveIndex >= 5) reduction = 1;
				}
				else {
					if (moveIndex >= 5) reduction = depth / 3; else reduction = 1;
				}
			}
			if (ZWS) {
				score = -Search<EXPECTED_CUT_NODE>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
				if (score > alpha && score < beta) score = -Search<PV>(-beta, -alpha, next, depth - 1 + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			}
			else {
				score = -Search<PV>(-beta, -alpha, next, depth - 1 - reduction + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
				if (score > alpha && reduction > 0) score = -Search<PV>(-beta, -alpha, next, depth - 1 + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			}

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
			Value delta = PieceValuesEG[pos.GetMostValuablePieceType(Color(pos.GetSideToMove() ^ 1))] + int(pos.PawnOn7thRank()) * (PieceValuesEG[QUEEN] - PieceValuesEG[PAWN]) + DELTA_PRUNING_SAFETY_MARGIN;
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
