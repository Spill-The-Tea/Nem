#include "evaluation.h"
#include "position.h"
#include "settings.h"


evaluation evaluate(const position& pos) {
	evaluation result;
	result.Material = pos.GetMaterialScore();
	result.Mobility = evaluateMobility(pos);
	return result;
}

evaluation evaluateDraw(const position& pos) {
	return DrawEvaluation;
}

eval evaluateMobility(const position& pos) {
	eval result;
	//Excluded fields (fields attacked by pawns or occupied by own king or pawns)
	for (int i = 0; i <= 1; ++i) {
		Color them = Color(i ^ 1);
		Color us = Color(i);
		int factor = 1 - 2 * i;
		Bitboard excluded = pos.AttackedByPawns(them) | pos.PieceBB(KING, us) | pos.PieceBB(PAWN, us);
		//Knight mobility
		Bitboard knights = pos.PieceBB(KNIGHT, us);
		while (knights) {
			Bitboard targets = pos.GetAttacksFrom(lsb(knights));
			result += factor * MOBILITY_BONUS_KNIGHT[popcount(targets)];
			knights &= knights - 1;
		}
		Bitboard bishops = pos.PieceBB(BISHOP, us);
		while (bishops) {
			Bitboard targets = pos.GetAttacksFrom(lsb(bishops));
			result += factor * MOBILITY_BONUS_BISHOP[popcount(targets)];
			bishops &= bishops - 1;
		}
		Bitboard rooks = pos.PieceBB(ROOK, us);
		while (rooks) {
			Bitboard targets = pos.GetAttacksFrom(lsb(rooks));
			result += factor * MOBILITY_BONUS_ROOK[popcount(targets)];
			rooks &= rooks - 1;
		}
		Bitboard queens = pos.PieceBB(QUEEN, us);
		while (queens) {
			Bitboard targets = pos.GetAttacksFrom(lsb(queens));
			result += factor * MOBILITY_BONUS_QUEEN[popcount(targets)];
			queens &= queens - 1;
		}
	}

	return result / 3;
}

evaluation evaluateFromScratch(const position& pos) {
	evaluation result;
	MaterialTableEntry * material = pos.GetMaterialTableEntry();
	material->Score = VALUE_ZERO;
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
	material->Phase = phase;
	result.Material = material->Score = materialEvaluation.getScore(phase);
	material->EvaluationFunction = &evaluate;
	return result;
}


