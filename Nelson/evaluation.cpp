#include "evaluation.h"
#include "position.h"
#include "settings.h"


evaluation evaluate(position& pos) {
	evaluation result;
	result.Material = pos.material->Score;
	return result;
}

evaluation evaluateFromScratch(position& pos) {
	evaluation result;
	pos.material->Score = VALUE_ZERO;
	eval materialEvaluation;
	for (PieceType pt = QUEEN; pt <= PAWN; ++pt) {
		int diff = popcount(pos.PieceBB(pt, WHITE)) - popcount(pos.PieceBB(pt, BLACK));
		materialEvaluation.egScore += diff * PieceValuesEG[pt];
		materialEvaluation.mgScore += diff * PieceValuesMG[pt];
	}
	Phase_t phase = Phase(popcount(pos.PieceBB(QUEEN, WHITE)), popcount(pos.PieceBB(QUEEN, BLACK)),
		popcount(pos.PieceBB(ROOK, WHITE)), popcount(pos.PieceBB(ROOK, BLACK)),
		popcount(pos.PieceBB(BISHOP, WHITE)), popcount(pos.PieceBB(BISHOP, BLACK)),
		popcount(pos.PieceBB(KNIGHT, WHITE)), popcount(pos.PieceBB(KNIGHT, BLACK)));
	pos.material->Phase = phase;
	result.Material = pos.material->Score = materialEvaluation.getScore(phase);
	pos.material->EvaluationFunction = &evaluate;
	return result;
}


