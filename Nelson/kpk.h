#pragma once
#include "types.h"
#include "board.h"
#include "position.h"


namespace kpk
{
	bool probe_kpk(Square wksq, Square wpsq, Square bksq, Color us);
	void init_kpk();
	template <Color STRONG_SIDE> Value EvaluateKPK(const position &pos);

	template <Color STRONG_SIDE> Value EvaluateKPK(const position &pos) {
		Square pawnSquare;
		Square wKingSquare;
		Square bKingSquare;
		Color stm;
		if (STRONG_SIDE == WHITE) {
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
		if (probe_kpk(wKingSquare, pawnSquare, bKingSquare, stm)) {
			Value result = VALUE_KNOWN_WIN + PieceValuesEG[PAWN] + Value(pawnSquare >> 3);
			if (STRONG_SIDE == BLACK) result = -result;
			return  result * (1 - 2 * pos.GetSideToMove());
		}
		else return VALUE_DRAW;
	}

	template <Color COL> const std::string printEvaluation(position &pos);

	template <Color COL> const std::string printEvaluation(position &pos) {
		Square pawnSquare;
		Square wKingSquare;
		Square bKingSquare;
		Color stm;
		if (COL == WHITE) {
			pawnSquare = scan(pos._bitboards[WPAWN]);
			wKingSquare = pos._kingSquare[WHITE];
			bKingSquare = pos._kingSquare[BLACK];
			stm = pos._sideToMove;
		}
		else {
			//flip colors
			pawnSquare = Square(scan(pos._bitboards[BPAWN]) ^ 56);
			wKingSquare = Square(pos._kingSquare[BLACK] ^ 56);
			bKingSquare = Square(pos._kingSquare[WHITE] ^ 56);
			stm = Color(pos._sideToMove ^ 1);
		}
		if (fileOfSquare(pawnSquare) > FileD) {
			pawnSquare = Square(pawnSquare ^ 7);
			wKingSquare = Square(wKingSquare ^ 7);
			bKingSquare = Square(bKingSquare ^ 7);
		}
		ostringstream ss;
		ss << "Transformed to K" << toString(wKingSquare) << " " << toString(pawnSquare) << " k" << toString(bKingSquare) << " STM: " << stm << std::endl;
		if (COL == WHITE)
			ss << "Evaluation: " << EvaluateKPK<WHITE>(pos);
		else
			ss << "Evaluation: " << EvaluateKPK<BLACK>(pos);

		return ss.str();
	}

}

