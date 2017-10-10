#pragma once
#include "types.h"
#include "board.h"
#include "position.h"


namespace kpk
{
	bool probe(Square WhiteKingSquare, Square WhitePawnSquare, Square BlackKingSquare, Color stm);
	template <Color StrongerSide> Value EvaluateKPK(const Position &pos);

	template <Color StrongerSide> Value EvaluateKPK(const Position &pos) {
		Square pawnSquare = Square::OUTSIDE;
		Square wKingSquare = Square::OUTSIDE;
		Square bKingSquare = Square::OUTSIDE;
		Color stm = pos.GetSideToMove();
		Value result = VALUE_ZERO;
		if (StrongerSide == WHITE) {
			pawnSquare = lsb(pos.PieceBB(PAWN, WHITE));
			wKingSquare = pos.KingSquare(WHITE);
			bKingSquare = pos.KingSquare(BLACK);
		}
		else {
			//flip colors
			pawnSquare = static_cast<Square>(lsb(pos.PieceBB(PAWN, BLACK)) ^ 56);
			wKingSquare = static_cast<Square>(pos.KingSquare(BLACK) ^ 56);
			bKingSquare = static_cast<Square>(pos.KingSquare(WHITE) ^ 56);
			stm = static_cast<Color>(pos.GetSideToMove() ^ 1);
		}
		if ((pawnSquare & 7) > FileD) {
			pawnSquare = static_cast<Square>(pawnSquare ^ 7);
			wKingSquare = static_cast<Square>(wKingSquare ^ 7);
			bKingSquare = static_cast<Square>(bKingSquare ^ 7);
		}
		if (probe(wKingSquare, pawnSquare, bKingSquare, stm)) {
			result = VALUE_KNOWN_WIN + settings::parameter.PieceValues[PAWN].egScore + static_cast<Value>(pawnSquare >> 3);
		}
		else {
			return pos.GetSideToMove() == settings::parameter.EngineSide ? -settings::parameter.Contempt : settings::parameter.Contempt;  //To force taking of the pawn (if possible)
		}
		return StrongerSide == pos.GetSideToMove() ? result : -result;
	}

}

namespace kqkp
{
	bool probeA2(const Square WhiteKingSquare, const Square WhiteQueenSquare, const Square BlackKingSquare, const Color stm);
	bool probeC2(const Square WhiteKingSquare, const Square WhiteQueenSquare, const Square BlackKingSquare, const Color stm);
}

