#pragma once
#include "types.h"
#include "board.h"
#include "position.h"


namespace kpk
{
	bool probe(Square WhiteKingSquare, Square WhitePawnSquare, Square BlackKingSquare, Color stm);
	template <Color StrongerSide> Value EvaluateKPK(const position &pos);

	template <Color StrongerSide> Value EvaluateKPK(const position &pos) {
		Square pawnSquare;
		Square wKingSquare;
		Square bKingSquare;
		Color stm;
		Value result;
		if (StrongerSide == WHITE) {
			pawnSquare = lsb(pos.PieceBB(PAWN, WHITE));
			wKingSquare = lsb(pos.PieceBB(KING, WHITE));
			bKingSquare = lsb(pos.PieceBB(KING, BLACK));
			stm = pos.GetSideToMove();
		}
		else {
			//flip colors
			pawnSquare = Square(lsb(pos.PieceBB(PAWN, BLACK)) ^ 56);
			wKingSquare = Square(lsb(pos.PieceBB(KING, BLACK)) ^ 56);
			bKingSquare = Square(lsb(pos.PieceBB(KING, WHITE)) ^ 56);
			stm = Color(pos.GetSideToMove() ^ 1);
		}
		if ((pawnSquare & 7) > FileD) {
			pawnSquare = Square(pawnSquare ^ 7);
			wKingSquare = Square(wKingSquare ^ 7);
			bKingSquare = Square(bKingSquare ^ 7);
		}
		if (probe(wKingSquare, pawnSquare, bKingSquare, stm)) {
			result = VALUE_KNOWN_WIN + PieceValuesEG[PAWN] + Value(pawnSquare >> 3);						
		}
		else {
			return evaluateDraw(pos); //To force taking of the pawn (if possible)
		}
		//if (StrongerSide == BLACK) result = -result;
		//return result * (1 - 2 * pos.GetSideToMove());
		return StrongerSide == pos.GetSideToMove() ? result : -result;
	}

}

namespace kqkp
{
	bool probeA2(Square WhiteKingSquare, Square WhiteQueenSquare, Square BlackKingSquare, Color stm);
	bool probeC2(Square WhiteKingSquare, Square WhiteQueenSquare, Square BlackKingSquare, Color stm);
}

