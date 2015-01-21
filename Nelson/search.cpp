#include "search.h"
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <math.h>
#include <cstring>
#include "hashtables.h"

ValuatedMove search::Think(position &pos, SearchStopCriteria ssc) {
	searchStopCriteria = ssc;
	_thinkTime = 1;
	int64_t * nodeCounts = new int64_t[ssc.MaxDepth + 1];
	nodeCounts[0] = 0;
	pos.ResetPliesFromRoot();
	tt::newSearch();
	//Iterativ Deepening Loop
	ValuatedMove * generatedMoves = pos.GenerateMoves<LEGAL>();
	rootMoveCount = pos.GeneratedMoveCount();
	rootMoves = new ValuatedMove[rootMoveCount];
	memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	for (_depth = 1; _depth <= ssc.MaxDepth; ++_depth) {
		Value alpha = -VALUE_MATE;
		Value beta = VALUE_MATE;
		Search<ROOT>(alpha, beta, pos, _depth, &PVMoves[0]);
		stable_sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
		BestMove = rootMoves[0];
		if (Stop) break;
		int64_t tNow = now();
		_thinkTime = tNow - ssc.StartTime;
		nodeCounts[_depth] = NodeCount - nodeCounts[_depth - 1];
		if (_depth > 3) BranchingFactor = sqrt(1.0 * nodeCounts[_depth] / nodeCounts[_depth - 2]);
		if (UciOutput && _thinkTime > 200) {
			if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
				cout << "info depth " << _depth << " nodes " << NodeCount << " score cp " << BestMove.score << " nps " << NodeCount * 1000 / _thinkTime
				<< " hashfull " << tt::GetHashFull()
				//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
				<< " pv " << PrincipalVariation(_depth) << endl;
			else {
				int pliesToMate;
				if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
				cout << "info depth " << _depth << " nodes " << NodeCount << " score mate " << pliesToMate / 2 << " nps " << NodeCount * 1000 / _thinkTime
					<< " hashfull " << tt::GetHashFull()
					//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
					<< " pv " << PrincipalVariation(_depth) << endl;
			}
		}
		if (tNow > ssc.SoftStopTime || (3 * (tNow - ssc.StartTime) + ssc.StartTime) > ssc.SoftStopTime || (abs(int(BestMove.score)) > int(VALUE_MATE_THRESHOLD) && abs(int(BestMove.score)) <= int(VALUE_MATE))) break;

	}
	delete[] rootMoves;
	delete[] nodeCounts;
	return BestMove;
}

template<> Value search::Search<ROOT>(Value alpha, Value beta, position &pos, int depth, Move * pv) {
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
		rootMoves[i].score = score;
		if (score >= beta) {
			updateCutoffStats(rootMoves[i].move, depth, pos, i);
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

template<NodeType NT> Value search::Search(Value alpha, Value beta, position &pos, int depth, Move * pv) {
	++NodeCount;
	if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	//TT lookup
	bool ttFound;
	tt::Entry* ttEntry = tt::probe(pos.GetHash(), ttFound);
	Value ttValue = ttFound ? tt::fromTT(ttEntry->value, pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	if (ttFound
		&& ttEntry->depth >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry->type() == tt::EXACT) || (ttValue >= beta && ttEntry->type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry->type() == tt::UPPER_BOUND))) {
		return ttValue;
	}
	if (depth <= 0) {
		return QSearch<QSEARCH_DEPTH_0>(alpha, beta, pos, depth);
	}
	Stop = Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime);
	if (Stop) return VALUE_ZERO;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	if (NT != NULL_MOVE && depth > 4 && pos.NonPawnMaterial(pos.GetSideToMove()) && !pos.Checked()) {
		int reduction;
		depth > 6 ? reduction = 2 : reduction = 3;
		Square epsquare = pos.GetEPSquare();
		pos.NullMove();
		Value nullscore = -Search<NULL_MOVE>(-beta, -alpha, pos, depth - reduction - 1, &subpv[0]);
		pos.NullMove(epsquare);
		if (nullscore >= beta) {
			return beta;
		}
	}
	Value score;
	Value bestScore = -VALUE_MATE;
	tt::NodeType nodeType = tt::UPPER_BOUND;
	pos.InitializeMoveIterator<MAIN_SEARCH>(&History, killer[pos.GetPliesFromRoot()][0], killer[pos.GetPliesFromRoot()][1], counterMove, ttFound ? ttEntry->move : MOVE_NONE);
	Move move;
	int moveIndex = 0;
	bool ZWS = false;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			if (ZWS) {
				score = -Search<EXPECTED_CUT_NODE>(Value(-alpha - 1), -alpha, next, depth - 1, &subpv[0]);
				if (score > alpha && score < beta) score = -Search<PV>(-beta, -alpha, next, depth - 1, &subpv[0]);
			}
			else
				score = -Search<PV>(-beta, -alpha, next, depth - 1, &subpv[0]);
			if (score >= beta) {
				updateCutoffStats(move, depth, pos, moveIndex);
				ttEntry->update(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, VALUE_NOTYETDETERMINED);
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
	ttEntry->update(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], VALUE_NOTYETDETERMINED);
	return bestScore;
}

template<NodeType NT> Value search::QSearch(Value alpha, Value beta, position &pos, int depth) {
	++QNodeCount;
	if (NT == QSEARCH_DEPTH_NEGATIVE) {
		++NodeCount;
		if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	}
	Stop = Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime);
	if (Stop) return VALUE_ZERO;
	Value standPat = pos.evaluate();
	if (standPat > beta || pos.GetResult() != OPEN) return beta;
	if (alpha < standPat) alpha = standPat;
	if (NT == QSEARCH_DEPTH_0) pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, EXTENDED_MOVE_NONE, EXTENDED_MOVE_NONE, nullptr); else pos.InitializeMoveIterator<QSEARCH>(&History, EXTENDED_MOVE_NONE, EXTENDED_MOVE_NONE, nullptr);
	Move move;
	Value score;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<QSEARCH_DEPTH_NEGATIVE>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				return beta;
			}
			if (score > alpha) {
				alpha = score;
			}
		}
	}
	return alpha;
}

void search::updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex) {
	cutoffAt1stMove += moveIndex == 0;
	cutoffCount++;
	cutoffMoveIndexSum += moveIndex;
	Piece movingPiece;
	Square toSquare;
	if (pos.GetMoveGenerationType() >= QUIETS_POSITIVE) {
		movingPiece = pos.GetPieceOnSquare(from(cutoffMove));
		ExtendedMove killerMove(movingPiece, cutoffMove);
		int pfr = pos.GetPliesFromRoot();
		killer[pfr][1] = killer[pfr][0];
		killer[pfr][0] = killerMove;
		toSquare = to(cutoffMove);
		Value v = Value(depth * depth);
		History.update(v, movingPiece, toSquare);
		ValuatedMove * alreadyProcessedQuiets = pos.GetMovesOfCurrentPhase();
		for (int i = 0; i < pos.GetMoveNumberInPhase() - 1; ++i) {
			movingPiece = pos.GetPieceOnSquare(from(alreadyProcessedQuiets->move));
			toSquare = to(alreadyProcessedQuiets->move);
			History.update(v, movingPiece, toSquare);
			alreadyProcessedQuiets++;
		}
		Square lastToSquare = to(pos.GetLastAppliedMove());
		PieceType lastMovedPT = GetPieceType(pos.GetPieceOnSquare(lastToSquare));
		counterMove[(lastMovedPT << 6) + lastToSquare] = cutoffMove;
	}
}

string search::PrincipalVariation(int depth) {
	std::stringstream ss;
	for (int i = 0; i < depth; ++i) {
		if (PVMoves[i] == MOVE_NONE) break;
		if (i>0) ss << " ";
		ss << toString(PVMoves[i]);
	}
	return ss.str();
}

