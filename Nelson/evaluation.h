#pragma once

#include "types.h"
#include "board.h"

struct evaluation
{
public:
	Value Material = VALUE_ZERO;
	eval Mobility = EVAL_ZERO;
	eval KingSafety = EVAL_ZERO;
	Value PawnStructure = VALUE_ZERO;

	inline Value GetScore(const Phase_t phase, const Color sideToMove) {
		return (Material + PawnStructure + (Mobility + KingSafety).getScore(phase)) * (1 - 2 * sideToMove);
	}
};

typedef Value(*EvalFunction)(const position&);

Value evaluate(const position& pos);
Value evaluateDraw(const position& pos);
Value evaluateFromScratch(const position& pos);
template <Color WinningSide> Value easyMate(const position& pos);
eval evaluateMobility(const position& pos);
eval evaluateMobility2(const position& pos);
eval evaluateKingSafety(const position& pos);

const int PSQ_GoForMate[64] = {
	100, 90, 80, 70, 70, 80, 90, 100,
	90, 70, 60, 50, 50, 60, 70, 90,
	80, 60, 40, 30, 30, 40, 60, 80,
	70, 50, 30, 20, 20, 30, 50, 70,
	70, 50, 30, 20, 20, 30, 50, 70,
	80, 60, 40, 30, 30, 40, 60, 80,
	90, 70, 60, 50, 50, 60, 70, 90,
	100, 90, 80, 70, 70, 80, 90, 100
};

const int BonusDistance[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

template <Color WinningSide> Value easyMate(const position& pos) {
	Value result = pos.GetMaterialScore();
	if (WinningSide == WHITE) {
		result += VALUE_KNOWN_WIN;
		result += Value(PSQ_GoForMate[lsb(pos.PieceBB(KING, BLACK))]);
		result += Value(BonusDistance[ChebishevDistance(lsb(pos.PieceBB(KING, BLACK)), lsb(pos.PieceBB(KING, WHITE)))]);
	}
	else {
		result -= VALUE_KNOWN_WIN;
		result -= Value(PSQ_GoForMate[lsb(pos.PieceBB(KING, WHITE))]);
		result -= Value(BonusDistance[ChebishevDistance(lsb(pos.PieceBB(KING, BLACK)), lsb(pos.PieceBB(KING, WHITE)))]);
	}

	return result * (1 - 2 * pos.GetSideToMove());
}

const int PSQ_MateInCorner[64] = {
	200, 190, 180, 170, 160, 150, 140, 130,
	190, 180, 170, 160, 150, 140, 130, 140,
	180, 170, 155, 140, 140, 125, 140, 150,
	170, 160, 140, 120, 110, 140, 150, 160,
	160, 150, 140, 110, 120, 140, 160, 170,
	150, 140, 125, 140, 140, 155, 170, 180,
	140, 130, 140, 150, 160, 170, 180, 190,
	130, 140, 150, 160, 170, 180, 190, 200
};

template <Color WinningSide> Value evaluateKNBK(const position& pos) {
	Square winnerKingSquare = lsb(pos.PieceBB(KING, WinningSide));
	Square loosingKingSquare = lsb(pos.PieceBB(KING, ~WinningSide));

	if ((DARKSQUARES & pos.PieceBB(BISHOP, WinningSide)) == 0) {
		//transpose KingSquares
		winnerKingSquare = Square(winnerKingSquare ^ 56);
		loosingKingSquare = Square(loosingKingSquare ^ 56);
	}

	Value result = VALUE_KNOWN_WIN + Value(BonusDistance[ChebishevDistance(winnerKingSquare, loosingKingSquare)]) + Value(PSQ_MateInCorner[loosingKingSquare]);

	if (WinningSide == BLACK) result = -result;
	return result * (1 - 2 * pos.GetSideToMove());
}



