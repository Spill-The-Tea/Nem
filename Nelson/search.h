#pragma once

#include "types.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include "polyglotbook.h"
#include <iostream>
#include <sstream>
#include <thread>

enum NodeType { PV, NULL_MOVE, EXPECTED_CUT_NODE, QSEARCH_DEPTH_0, QSEARCH_DEPTH_NEGATIVE };

enum ThreadType { MASTER, SLAVE };

template<ThreadType T> struct search {
public:
	ValuatedMove BestMove;
	int64_t NodeCount;
	int64_t QNodeCount;	
	bool UciOutput = false;
	bool PonderMode = false;
	double BranchingFactor = 0;
	std::string BookFile = "";

	~search();
	inline ValuatedMove Think(position &pos, SearchStopCriteria ssc);
	inline std::string PrincipalVariation(int depth = PV_MAX_LENGTH);
	inline void Reset();
	inline void NewGame();
	inline int Depth() const { return _depth; }
	inline int64_t ThinkTime() const { return _thinkTime; }
	inline double cutoffAt1stMoveRate() const { return 100.0 * cutoffAt1stMove / cutoffCount; }
	inline double cutoffAverageMove() const { return 1 + 1.0 * cutoffMoveIndexSum / cutoffCount; }
	inline Move PonderMove() const { return PVMoves[1]; }
	inline void ExtendStopTimes(int64_t extensionTime) { searchStopCriteria.HardStopTime += extensionTime; searchStopCriteria.SoftStopTime += extensionTime; }
	inline void StopThinking(){ Stop = true; searchStopCriteria.HardStopTime = 0; }
	inline Move GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount);

private:
	SearchStopCriteria searchStopCriteria;
	int rootMoveCount;
	ValuatedMove * rootMoves;
	Move PVMoves[PV_MAX_LENGTH];
	HistoryStats History;
	DblHistoryStats * DblHistory = nullptr;
	int _depth = 0;
	int64_t _thinkTime;
	uint64_t cutoffAt1stMove = 0;
	uint64_t cutoffCount = 0;
	uint64_t cutoffMoveIndexSum = 0;
	ExtendedMove killer[2*MAX_DEPTH];
	Move counterMove[12 * 64];
	Value gains[12 * 64];
	polyglot::polyglotbook book;
	bool Stop = false;

	template<NodeType NT> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv);
	Value SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv);
	template<NodeType NT> Value QSearch(Value alpha, Value beta, position &pos, int depth);
	void updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex);
	inline bool Stopped() { return Stop && (!PonderMode); }

};


template<ThreadType T> inline void search<T>::Reset() {
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
	if (!DblHistory) DblHistory = new DblHistoryStats();
	DblHistory->initialize();
	for (int i = 0; i < 2*MAX_DEPTH; ++i) killer[i] = EXTENDED_MOVE_NONE;
}

template<ThreadType T> inline void search<T>::NewGame() {
	Reset();
	for (int i = 0; i < 12 * 64; ++i) {
		counterMove[i] = MOVE_NONE;
		//gains[i] = VALUE_ZERO;
	}
	for (int i = 0; i < 12 * 64; ++i) gains[i] = VALUE_ZERO;
}

template<ThreadType T> inline std::string search<T>::PrincipalVariation(int depth) {
	std::stringstream ss;
	for (int i = 0; i < depth; ++i) {
		if (PVMoves[i] == MOVE_NONE) break;
		if (i>0) ss << " ";
		ss << toString(PVMoves[i]);
	}
	return ss.str();
}

template<ThreadType T> inline Move search<T>::GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount) {
	return (USE_BOOK && BookFile.size() > 0) ? book.probe(pos, BookFile, true, moves, moveCount) : MOVE_NONE;
}


template<ThreadType T> inline ValuatedMove search<T>::Think(position &pos, SearchStopCriteria ssc) {
	searchStopCriteria = ssc;
	_thinkTime = 1;
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
	rootMoves = new ValuatedMove[rootMoveCount];
	memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	//Iterativ Deepening Loop
	for (_depth = 1; _depth <= searchStopCriteria.MaxDepth; ++_depth) {
		Value alpha = -VALUE_MATE;
		Value beta = VALUE_MATE;
		SearchRoot(alpha, beta, pos, _depth, &PVMoves[0]);
		std::stable_sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
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
				std::cout << "info depth " << _depth << " seldepth 0 multipv 1" << " score cp " << BestMove.score << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
				<< " hashfull " << tt::GetHashFull()
				//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
				<< " pv " << PrincipalVariation(_depth) << std::endl;
			else {
				int pliesToMate;
				if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
				std::cout << "info depth " << _depth << " seldepth 0 multipv 1" << " score mate " << pliesToMate / 2 << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
					<< " hashfull " << tt::GetHashFull()
					//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
					<< " pv " << PrincipalVariation(_depth) << std::endl;
			}
		}
		if (Stopped()) break;
	}

END:	while (PonderMode) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	delete[] nodeCounts;
	return BestMove;
}

template<ThreadType T> search<T>::~search() {
	delete DblHistory;
}


template<ThreadType T> Value search<T>::SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Value bonus;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool ZWS = false;
	for (int i = 0; i < rootMoveCount; ++i) {
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		if (type(rootMoves[i].move) == CASTLING) bonus = BONUS_CASTLING; else bonus = VALUE_ZERO;
		if (ZWS) {
			score = bonus - Search<EXPECTED_CUT_NODE>(Value(bonus - alpha - 1), bonus - alpha, next, depth - 1, &subpv[0]);
			if (score > alpha && score < beta) score = bonus - Search<PV>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0]);
		}
		else score = bonus - Search<PV>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0]);
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
	return bestScore;
}

template<ThreadType T> template<NodeType NT> Value search<T>::Search(Value alpha, Value beta, position &pos, int depth, Move * pv) {
	++NodeCount;
	Stop = !PonderMode && (Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime));
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
	tt::Entry* ttPointer = (T == SLAVE || HelperThreads > 0) ? tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value, pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	if (ttFound
		&& ttEntry.depth >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		//if (ttValue > beta && ttEntry->move != MOVE_NONE && pos.IsQuiet(ttEntry->move)) updateCutoffStats(ttEntry->move, depth, pos, 0);
		return ttValue;
	}
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool checked = pos.Checked();
	Value staticEvaluation = checked ? VALUE_NOTYETDETERMINED :
		ttFound && ttEntry.evalValue != VALUE_NOTYETDETERMINED ? ttEntry.evalValue : pos.evaluate();
	Move ttMove = ttFound ? ttEntry.move : MOVE_NONE;
	//Razoring a la SF (no measurable ELO change)
	//if (!checked && depth < 4
	//	&& (staticEvaluation + Value(512 + 32 * depth)) <= alpha
	//	&& ttMove == MOVE_NONE
	//	&& !pos.PawnOn7thRank())
	//{
	//	if (depth <= 1 && (staticEvaluation + 608) <= alpha) return QSearch<QSEARCH_DEPTH_0>(alpha, beta, pos, 0);
	//	Value ralpha = alpha - Value(512 + 32 * depth);
	//	Value v = QSearch<QSEARCH_DEPTH_0>(ralpha, Value(ralpha + 1), pos, 0);
	//	if (v <= ralpha) return v;
	//}
	// Beta Pruning
	if (!checked
		&& depth < 7
		&& staticEvaluation < VALUE_KNOWN_WIN
		&& (staticEvaluation - BETA_PRUNING_FACTOR * depth) >= beta
		//&& !pos.GetMaterialTableEntry()->IsLateEndgame()
		&& pos.NonPawnMaterial(pos.GetSideToMove()))
		return staticEvaluation - BETA_PRUNING_FACTOR * depth;
	//Null Move Pruning
	if (NT != NULL_MOVE && !checked && staticEvaluation > beta && depth > 4 && !pos.GetMaterialTableEntry()->IsLateEndgame() && pos.NonPawnMaterial(pos.GetSideToMove())) {
		int reduction = depth >> 1;
		Square epsquare = pos.GetEPSquare();
		pos.NullMove();
		Value nullscore = -Search<NULL_MOVE>(-beta, -alpha, pos, depth - reduction - 1, &subpv[0]);
		pos.NullMove(epsquare);
		if (nullscore >= beta) return beta;
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

	//IID (Seems to be neutral => therefore deactivated
	if (NT == PV && depth >= 3 && !ttMove) {
		position next(pos);
		next.copy(pos);
		Search<NT>(alpha, beta, next, depth - 2, &subpv[0]);
		if (Stopped()) return VALUE_ZERO;
		ttPointer = (T == SLAVE || HelperThreads > 0) ? tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry);
		ttMove = ttFound ? ttEntry.move : MOVE_NONE;
	}
	Value score;
	Value bestScore = -VALUE_MATE;
	tt::NodeType nodeType = tt::UPPER_BOUND;
	//Futility Pruning: Prune quiet moves that give no check if they can't raise alpha
	bool futilityPruning = NT != NULL_MOVE && !checked && depth <= FULTILITY_PRUNING_DEPTH && beta < VALUE_MATE_THRESHOLD && pos.NonPawnMaterial(pos.GetSideToMove());
	if (futilityPruning && alpha >(staticEvaluation + FUTILITY_PRUNING_LIMIT[depth]))
		pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, DblHistory, nullptr, counterMove, ttMove);
	else
		pos.InitializeMoveIterator<MAIN_SEARCH>(&History, DblHistory, &killer[2 * pos.GetPliesFromRoot()], counterMove, ttMove);
	bool lmr = !checked && depth >= 3;
	Move move;
	int moveIndex = 0;
	bool ZWS = false;
	while ((move = pos.NextMove())) {
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
			int extension = (next.Checked() && pos.SEE_Sign(move) >= VALUE_ZERO) ? 1 : 0;
			int reduction = 0;
			if (lmr && moveIndex >= 2 && !extension && pos.IsQuietAndNoCastles(move) && !next.Checked()) {
				if (NT == PV) {
					if (moveIndex >= 5) reduction = 1;
				}
				else {
					if (moveIndex >= 5) reduction = depth / 3; else reduction = 1;
				}
			}
			if (ZWS) {
				score = -Search<EXPECTED_CUT_NODE>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction + extension, &subpv[0]);
				if (score > alpha && score < beta) score = -Search<PV>(-beta, -alpha, next, depth - 1 + extension, &subpv[0]);
			}
			else
				score = -Search<PV>(-beta, -alpha, next, depth - 1 - reduction + extension, &subpv[0]);
			if (score >= beta) {
				updateCutoffStats(move, depth, pos, moveIndex);
				if (T == SLAVE || HelperThreads > 0) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
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
	if (T == SLAVE || HelperThreads > 0) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	return bestScore;
}

template<ThreadType T> template<NodeType NT> Value search<T>::QSearch(Value alpha, Value beta, position &pos, int depth) {
	++QNodeCount;
	if (NT == QSEARCH_DEPTH_NEGATIVE) {
		++NodeCount;
		Stop = !PonderMode && (Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime));
		if (Stopped()) return VALUE_ZERO;
		if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	}
	//TT lookup
    tt::Entry ttEntry;
	bool ttFound;
	tt::Entry* ttPointer = (T == SLAVE || HelperThreads > 0) ? tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value, pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	if (ttFound
		&& ttEntry.depth >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		return ttValue;
	}
	bool checked = pos.Checked();
	Value standPat;
	if (checked) {
		standPat = VALUE_NOTYETDETERMINED;
	}
	else {
		standPat = ttFound && ttEntry.evalValue != VALUE_NOTYETDETERMINED ? ttEntry.evalValue : pos.evaluate();
		if (standPat >= beta) {
			if (T == SLAVE || HelperThreads > 0) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, MOVE_NONE, standPat);
			else ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, MOVE_NONE, standPat);
			return beta;
		}
		//Delta Pruning
		if (!pos.GetMaterialTableEntry()->IsLateEndgame()) {
			Value delta = PieceValuesEG[pos.GetMostValuablePieceType(~pos.GetSideToMove())] + int(pos.PawnOn7thRank()) * (PieceValuesEG[QUEEN] - PieceValuesEG[PAWN]) + DELTA_PRUNING_SAFETY_MARGIN;
			if (standPat + delta < alpha) return alpha;
		}
		if (alpha < standPat) alpha = standPat;
	}
	if (NT == QSEARCH_DEPTH_0) pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, DblHistory, nullptr, nullptr);
	else pos.InitializeMoveIterator<QSEARCH>(&History, DblHistory, nullptr, nullptr);
	Move move;
	Value score;
	tt::NodeType nt = tt::UPPER_BOUND;
	while ((move = pos.NextMove())) {
		if (pos.SEE_Sign(move) < VALUE_ZERO) continue;
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<QSEARCH_DEPTH_NEGATIVE>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				if (T == SLAVE || HelperThreads > 0) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, move, standPat);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, depth, move, standPat);
				return beta;
			}
			if (score > alpha) {
				nt = tt::EXACT;
				alpha = score;
			}
		}
	}
	if (T == SLAVE || HelperThreads > 0) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), alpha, nt, depth, MOVE_NONE, standPat);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), alpha, nt, depth, MOVE_NONE, standPat);
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
			DblHistory->update(v, prevPiece, prevTo, movingPiece, toSquare);
		}
		if (moveIndex > 0) {
			ValuatedMove * alreadyProcessedQuiets = pos.GetMovesOfCurrentPhase();
			for (int i = 0; i < pos.GetMoveNumberInPhase() - 1; ++i) {
				movingPiece = pos.GetPieceOnSquare(from(alreadyProcessedQuiets->move));
				toSquare = to(alreadyProcessedQuiets->move);
				History.update(-v, movingPiece, toSquare);
				if (pos.GetLastAppliedMove())
					DblHistory->update(-v, prevPiece, prevTo, movingPiece, toSquare);
				alreadyProcessedQuiets++;
			}
		}
	}
}