#include "search.h"
#include <stdlib.h>
#include <algorithm>

ValuatedMove search::Think(position &pos, SearchStopCriteria ssc) {
	searchStopCriteria = ssc;
	//Iterativ Deepening Loop
	ValuatedMove * generatedMoves = pos.GenerateMoves<LEGAL>();
	rootMoveCount = pos.GeneratedMoveCount();
	rootMoves = new ValuatedMove[rootMoveCount];
	memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	for (int depth = 1; depth <= ssc.MaxDepth; ++depth) {
		Value alpha = -VALUE_MATE;
		Value beta = VALUE_MATE;
		Search<ROOT>(alpha, beta, pos, depth);
		stable_sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
		BestMove = rootMoves[0];
	}
	delete[] rootMoves;
	return BestMove;
}

template<> Value search::Search<ROOT>(Value alpha, Value beta, position &pos, int depth) {
	Value score;
	Value bestScore = -VALUE_MATE;
	for (int i = 0; i < rootMoveCount; ++i) {
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		score = -Search<PV>(-beta, -alpha, next, depth - 1);
		rootMoves[i].score = score;
		if (score >= beta) {
			return score;
		}
		if (score > bestScore) {
			bestScore = score;
			if (score > alpha)
			{
				alpha = score;
			}
		}
	}
	return bestScore;
}

template<NodeType NT> Value search::Search(Value alpha, Value beta, position &pos, int depth) {
	if (pos.GetResult() != OPEN) return pos.evaluate();
	if (depth <= 0) {
		return QSearch<STANDARD>(alpha, beta, pos, depth);
	}
	Value score;
	Value bestScore = -VALUE_MATE;
	pos.InitializeMoveIterator<MAIN_SEARCH>();
	Move move;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -Search<PV>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				return score;
			}
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha)
				{
					alpha = score;
				}
			}
		}
	}
	return bestScore;
}

template<NodeType NT> Value search::QSearch(Value alpha, Value beta, position &pos, int depth) {
	Value standPat = pos.evaluate();
	if (standPat > beta || pos.GetResult() != OPEN) return beta;
	if (alpha < standPat) alpha = standPat;
	pos.InitializeMoveIterator<QSEARCH>();
	Move move;
	Value score;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<STANDARD>(-beta, -alpha, next, depth - 1);
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

