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
		sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
		BestMove = rootMoves[0];
	}
	delete[] rootMoves;
	return BestMove;
}
 
template<> Value search::Search<ROOT>(Value alpha, Value beta, position &pos, int depth) {
	Value score;
	bool ZWS = false; //zero window search
	for (int i = 0; i < rootMoveCount; ++i) {
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		//if (ZWS) {
		//	score = -Search<STANDARD>(Value(-alpha - 1), -alpha, next, depth - 1);
		//	if (score > alpha && score < beta)
		//		score = -Search<PV>(-beta, -alpha, next, depth - 1);
		//}
		//else {
			score = -Search<PV>(-beta, -alpha, next, depth - 1);
		//}
		rootMoves[i].score = score;
		if (score >= beta) {
			return score;
		}
		if (score > alpha)
		{
			alpha = score;
			ZWS = true;
		}
	}
	return alpha;
}

template<NodeType NT> Value search::Search(Value alpha, Value beta, position &pos, int depth) {
	if (depth <= 0) {
		return QSearch<STANDARD>(alpha, beta, pos, depth);
	}
	Value score;
	pos.InitializeMoveIterator<MAIN_SEARCH>();
	bool validMoveExists = false;
	bool ZWS = false; //zero window search
	Move move;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			validMoveExists = true;
			//if (ZWS) {
			//	score = -Search<STANDARD>(Value(-alpha - 1), -alpha, next, depth - 1);
			//	if (score > alpha && score < beta)
			//		score = -Search<PV>(-beta, -alpha, next, depth - 1);
			//}
			//else {
				score = -Search<PV>(-beta, -alpha, next, depth - 1);
			//}
			if (score >= beta) {
				return score;
			}
			if (score > alpha)
			{
				alpha = score;
				ZWS = true;
			}
		}
	}
	if (!validMoveExists) {
		if (pos.Checked()) return Value(-VALUE_MATE + pos.GetPliesFromRoot()); else return VALUE_DRAW; //Mated or Stalemate
	}
	return alpha;
}

template<NodeType NT> Value search::QSearch(Value alpha, Value beta, position &pos, int depth) {
	Value standPat = pos.evaluate();
	if (standPat > beta) return beta;
	if (alpha < standPat) alpha = standPat;
	pos.InitializeMoveIterator<QSEARCH>();
	Move move;
	Value score;
	bool validMoveFound = false;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			validMoveFound = true;
			score = -QSearch<STANDARD>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				return beta;
			}
			if (score > alpha) {
				alpha = score;
			}
		}
	}
	if (validMoveFound) return alpha;
	else if (pos.Checked()) return Value(-VALUE_MATE + pos.GetPliesFromRoot());
	else {
		ValuatedMove * vm = pos.GenerateMoves<QUIETS>();
		while ((vm->move)) {
			position next(pos);
			if (next.ApplyMove(move)) return alpha;
			vm++;
		}
		//No valid move and position isn't checked => stalemate
		return VALUE_DRAW;
	}
}

