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

Value evaluateDefault(const position& pos);
Value evaluateDraw(const position& pos);
Value evaluateFromScratch(const position& pos);
template<Color WinningSide> Value evaluateKBPK(const position& pos);
template <Color WinningSide> Value easyMate(const position& pos);
template <Color WinningSide> Value evaluateKQKP(const position& pos);
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

//Copied from SF: Doesn't recognize drawn positions (could be improved)
//Idea create bitbase for all positions where pawn is on A7 and C7 and the Black King is controlling
//the conversion square => Size: 8 (black king pawn constellations) * 64 (wking squares) * 64 (wqueen squares) 
// results in a bitbase of 4k (Problem there are some very rare positions where even a pawn on 5th rank draws)
template<Color WinningSide> Value evaluateKQKP(const position& pos) {
	Square winnerKSq = lsb(pos.PieceBB(KING, WinningSide));
	Square loserKSq = lsb(pos.PieceBB(KING, ~WinningSide));
	Square pawnSq = lsb(pos.PieceBB(PAWN, ~WinningSide));

	Value result = Value(BonusDistance[ChebishevDistance(winnerKSq, loserKSq)]);

	int relativeRank = pawnSq >> 3;;
	if (WinningSide == WHITE) relativeRank = 7 - relativeRank;
	if (relativeRank != Rank7
		|| ChebishevDistance(loserKSq, pawnSq) != 1
		|| !((A_FILE | C_FILE | F_FILE | H_FILE) & pos.PieceBB(PAWN, ~WinningSide)))
		result += PieceValuesEG[QUEEN] - PieceValuesEG[PAWN];

	return WinningSide == pos.GetSideToMove() ? result : -result;
}

template<Color WinningSide> Value evaluateKBPK(const position& pos) {
	//Check for draw
	Value result;
	Bitboard bbPawn = pos.PieceBB(PAWN, WinningSide);
	Square pawnSquare = lsb(bbPawn);
	if ((bbPawn & (A_FILE | H_FILE)) != 0) { //Pawn is on rook file
		Square conversionSquare = WinningSide == WHITE ? Square(56 + (pawnSquare & 7)) : Square(pawnSquare & 7);
		if (oppositeColors(conversionSquare, lsb(pos.PieceBB(BISHOP, WinningSide)))) { //Bishop doesn't match conversion's squares color
			//Now check if weak king control the conversion square
			Bitboard conversionSquareControl = KingAttacks[conversionSquare] | (ToBitboard(conversionSquare));
			if (pos.PieceBB(KING, ~WinningSide) & conversionSquareControl) return VALUE_DRAW;
			if ((pos.PieceBB(KING, WinningSide) & conversionSquareControl) == 0) { //Strong king doesn't control conversion square
				Square weakKing = lsb(pos.PieceBB(KING, ~WinningSide));
				Square strongKing = lsb(pos.PieceBB(KING, WinningSide));
				Bitboard bbRank2 = WinningSide == WHITE ? RANK2 : RANK7;
				if ((ChebishevDistance(weakKing, conversionSquare) + (pos.GetSideToMove() != WinningSide) <= ChebishevDistance(pawnSquare, conversionSquare) - ((bbPawn & bbRank2) != 0)) //King is in Pawnsquare
					&& (ChebishevDistance(weakKing, conversionSquare) - (pos.GetSideToMove() != WinningSide) <= ChebishevDistance(strongKing, conversionSquare))) {
					//Weak King is in pawn square and at least as near to the conversion square as the Strong King => search must solve it
					if (WinningSide == WHITE) {
						result = pos.GetMaterialScore()
							+ Value(10 * (ChebishevDistance(weakKing, conversionSquare) - ChebishevDistance(strongKing, conversionSquare)))
							+ Value(5 * (pawnSquare >> 3));
					}
					else {
						result = pos.GetMaterialScore()
							- Value(10 * (ChebishevDistance(weakKing, conversionSquare) - ChebishevDistance(strongKing, conversionSquare)))
							- Value(5 * (7 - (pawnSquare >> 3)));
					}
					return result * (1 - 2 * pos.GetSideToMove());
				}
			}
		}
	}
	result = WinningSide == WHITE ? VALUE_KNOWN_WIN + pos.GetMaterialScore() + Value(pawnSquare >> 3)
		                                : pos.GetMaterialScore() - VALUE_KNOWN_WIN - Value(7 - (pawnSquare >> 3));
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



