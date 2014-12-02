#pragma once
#include "types.h"
#include "board.h"
#include <string>

using namespace std;

struct position
{
public:
	position();
	position(string fen);
	position(position &pos);
	~position();

	Bitboard PieceBB(const PieceType pt, const Color c) const;
	Bitboard ColorBB(const Color c) const;
	Bitboard ColorBB(const int c) const;
	Bitboard OccupiedBB() const;
	string print() const;
	string fen() const;
	void setFromFEN(const string& fen);
	bool ApplyMove(Move move); //Applies a pseudo-legal move and returns true if move is legal
	static inline position UndoMove(position &pos) { return *pos.previous; }
	template<MoveGenerationType MGT> Move * GenerateMoves();
private:
	Bitboard OccupiedByColor[2];
	Bitboard OccupiedByPieceType[6];
	Square EPSquare;
	unsigned char CastlingOptions;
	unsigned char DrawPlyCount;
	Color SideToMove;
	Piece Board[64];

	position * previous;

	Move moves[256];
	int movepointer = 0;
	Bitboard attacks[64];
	Bitboard attackedByThem;
	Bitboard attackedByUs;

	void set(const Piece piece, const Square square);
	void remove(const Square square);
	inline void AddCastlingOption(const CastleFlag castleFlag) { CastlingOptions |= castleFlag; }
	inline void RemoveCastlingOption(const CastleFlag castleFlag) { CastlingOptions &= ~castleFlag; }
	inline void SetEPSquare(const Square square) { EPSquare = square; }
	inline const int PawnStep() const { return 8 - 16 * SideToMove; }
	inline void AddMove(Move move) { moves[movepointer] = move; ++movepointer; }
	void updateCastleFlags(Square fromSquare, Square toSquare);
	Bitboard calculateAttacks(Color color);
};

inline Bitboard position::PieceBB(const PieceType pt, const Color c) const { return OccupiedByColor[c] & OccupiedByPieceType[pt]; }
inline Bitboard position::ColorBB(const Color c) const { return OccupiedByColor[c]; }
inline Bitboard position::ColorBB(const int c) const { return OccupiedByColor[c]; }
inline Bitboard position::OccupiedBB() const { return OccupiedByColor[WHITE] | OccupiedByColor[BLACK]; }

template<MoveGenerationType MGT> Move * position::GenerateMoves() {
	//Rooksliders
	Bitboard targets;
	movepointer = 0;
	if (MGT == ALL || MGT == TACTICAL || MGT == QUIETS) {
		Bitboard empty = ~ColorBB(BLACK) & ~ColorBB(WHITE);
		if (MGT == ALL) targets = ~ColorBB(SideToMove);
		else if (MGT == TACTICAL) targets = ColorBB(SideToMove ^ 1);
		else if (MGT == QUIETS) targets = empty;
		Bitboard sliders = PieceBB(ROOK, SideToMove) | PieceBB(QUEEN, SideToMove) | PieceBB(BISHOP, SideToMove);
		while (sliders) {
			Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
		////Bishopsliders
		//Bitboard bishopSliders = PieceBB(QUEEN, SideToMove) | PieceBB(BISHOP, SideToMove);
		//while (bishopSliders) {
		//	Square from = lsb(bishopSliders);
		//	Bitboard bishopTargets = attacks[from] & targets;
		//	while (bishopTargets) {
		//		AddMove(createMove(from, lsb(bishopTargets)));
		//		bishopTargets &= bishopTargets - 1;
		//	}
		//	bishopSliders &= bishopSliders - 1;
		//}
		//Knights
		Bitboard knights = PieceBB(KNIGHT, SideToMove);
		while (knights) {
			Square from = lsb(knights);
			Bitboard knightTargets = attacks[from] & targets;
			while (knightTargets) {
				AddMove(createMove(from, lsb(knightTargets)));
				knightTargets &= knightTargets - 1;
			}
			knights &= knights - 1;
		}
		//Pawns
		Bitboard pawns = PieceBB(PAWN, SideToMove);
		//Captures
		if (MGT == ALL || MGT == TACTICAL) {
			while (pawns) {
				Square from = lsb(pawns);
				Bitboard pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & ~RANK1and8;
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				//Promotion Captures
				pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
				while (pawnTargets) {
					Square to = lsb(pawnTargets);
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					AddMove(createMove<PROMOTION>(from, to, ROOK));
					AddMove(createMove<PROMOTION>(from, to, BISHOP));
					AddMove(createMove<PROMOTION>(from, to, KNIGHT));
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
		}
		Bitboard singleStepTarget;
		Bitboard doubleSteptarget;
		Bitboard promotionTarget;
		if (SideToMove == WHITE) {
			if (MGT == ALL || MGT == QUIETS) {
				singleStepTarget = (PieceBB(PAWN, WHITE) << 8) & empty & ~RANK8;
				doubleSteptarget = ((singleStepTarget & RANK3) << 8) & empty;
			}
			if (MGT == ALL || MGT == TACTICAL) promotionTarget = (PieceBB(PAWN, WHITE) << 8) & empty & RANK8;
		}
		else {
			if (MGT == ALL || MGT == QUIETS) {
				singleStepTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & ~RANK1;
				doubleSteptarget = ((singleStepTarget & RANK6) >> 8) & empty;
			}
			if (MGT == ALL || MGT == TACTICAL) promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & RANK1;
		}
		if (MGT == ALL || MGT == QUIETS) {
			while (singleStepTarget) {
				Square to = lsb(singleStepTarget);
				AddMove(createMove(to - PawnStep(), to));
				singleStepTarget &= singleStepTarget - 1;
			}
			while (doubleSteptarget) {
				Square to = lsb(doubleSteptarget);
				AddMove(createMove(to - 2 * PawnStep(), to));
				doubleSteptarget &= doubleSteptarget - 1;
			}
		}
		//Promotions
		if (MGT == ALL || MGT == TACTICAL) {
			while (promotionTarget) {
				Square to = lsb(promotionTarget);
				Square from = Square(to - PawnStep());
				AddMove(createMove<PROMOTION>(from, to, QUEEN));
				AddMove(createMove<PROMOTION>(from, to, ROOK));
				AddMove(createMove<PROMOTION>(from, to, BISHOP));
				AddMove(createMove<PROMOTION>(from, to, KNIGHT));
				promotionTarget &= promotionTarget - 1;
			}
			Bitboard epAttacker;
			if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
				while (epAttacker) {
					AddMove(createMove<ENPASSANT>(lsb(epAttacker), EPSquare));
					epAttacker &= epAttacker - 1;
				}
			}
		}
		//Kings
		Square kingSquare = lsb(PieceBB(KING, SideToMove));
		Bitboard kingTargets = KingAttacks[kingSquare] & targets & ~attackedByThem;
		while (kingTargets) {
			AddMove(createMove(kingSquare, lsb(kingTargets)));
			kingTargets &= kingTargets - 1;
		}
		if (MGT == ALL || MGT == QUIETS) {
			if (CastlingOptions & CastlesbyColor[SideToMove] //Castle allowed at all
				&& (PieceBB(KING, SideToMove) & InitialKingSquare[SideToMove])) //King is on initial square
			{
				//King-side castles
				if ((CastlingOptions & (1 << (2 * SideToMove))) //Short castle allowed
					&& (InitialRookSquare[2 * SideToMove] & PieceBB(ROOK, SideToMove)) //Rook on initial square
					&& !(SquaresToBeEmpty[2 * SideToMove] & OccupiedBB()) //Fields between Rook and King are empty
					&& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
					&& !(SquaresToBeUnattacked[2 * SideToMove] & attackedByThem)) //Fields passed by the king are unattacked
					AddMove(createMove<CASTLING>(kingSquare, Square(kingSquare + 2)));
				//Queen-side castles
				if ((CastlingOptions & (1 << (2 * SideToMove + 1))) //Short castle allowed
					&& (InitialRookSquare[2 * SideToMove + 1] & PieceBB(ROOK, SideToMove)) //Rook on initial square
					&& !(SquaresToBeEmpty[2 * SideToMove + 1] & OccupiedBB()) //Fields between Rook and King are empty
					& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
					&& !(SquaresToBeUnattacked[2 * SideToMove + 1] & attackedByThem)) //Fields passed by the king are unattacked
					AddMove(createMove<CASTLING>(kingSquare, Square(kingSquare - 2)));
			}
		}
	}
	else { //Winning, Equal and loosing captures
		Bitboard targets;
		Bitboard sliders;
		if (MGT == EQUAL_CAPTURES || MGT == LOOSING_CAPTURES) { //Queen Captures are never winning
			sliders = PieceBB(QUEEN, SideToMove);
			if (MGT == EQUAL_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1));
			else if (MGT == LOOSING_CAPTURES) targets = PieceBB(ROOK, Color(SideToMove ^ 1)) | PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(PAWN, Color(SideToMove ^ 1));
			while (sliders) {
				Square from = lsb(sliders);
				Bitboard sliderTargets = attacks[from] & targets;
				while (sliderTargets) {
					AddMove(createMove(from, lsb(sliderTargets)));
					sliderTargets &= sliderTargets - 1;
				}
				sliders &= sliders - 1;
			}
		}
		//Rook Captures
		sliders = PieceBB(ROOK, SideToMove);
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1));
		else if (MGT == EQUAL_CAPTURES) targets = PieceBB(ROOK, Color(SideToMove ^ 1));
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(PAWN, Color(SideToMove ^ 1));
		while (sliders) {
			Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
		//Bishop Captures
		sliders = PieceBB(BISHOP, SideToMove);
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1));
		else if (MGT == EQUAL_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1));
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(PAWN, Color(SideToMove ^ 1));
		while (sliders) {
			Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
		//Knight Captures
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1));
		else if (MGT == EQUAL_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1));
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(PAWN, Color(SideToMove ^ 1));
		Bitboard knights = PieceBB(KNIGHT, SideToMove);
		while (knights) {
			Square from = lsb(knights);
			Bitboard knightTargets = attacks[from] & targets;
			while (knightTargets) {
				AddMove(createMove(from, lsb(knightTargets)));
				knightTargets &= knightTargets - 1;
			}
			knights &= knights - 1;
		}
		//Pawn Captures
		if (MGT == WINNING_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				Square from = lsb(pawns);
				Bitboard pawnTargets = ColorBB(SideToMove ^ 1) & ~PieceBB(PAWN, Color(SideToMove ^ 1)) & attacks[from] & ~RANK1and8;
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				//Promotion Captures
				pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
				while (pawnTargets) {
					Square to = lsb(pawnTargets);
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					AddMove(createMove<PROMOTION>(from, to, ROOK));
					AddMove(createMove<PROMOTION>(from, to, BISHOP));
					AddMove(createMove<PROMOTION>(from, to, KNIGHT));
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
		}
		else if (MGT == EQUAL_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				Square from = lsb(pawns);
				Bitboard pawnTargets = PieceBB(PAWN, Color(SideToMove ^ 1)) & attacks[from];
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
			Bitboard epAttacker;
			if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
				while (epAttacker) {
					AddMove(createMove<ENPASSANT>(lsb(epAttacker), EPSquare));
					epAttacker &= epAttacker - 1;
				}
			}
		}
		if (MGT == WINNING_CAPTURES) {
			Bitboard promotionTarget;
			if (SideToMove == WHITE)  promotionTarget = (PieceBB(PAWN, WHITE) << 8) & ~OccupiedBB() & RANK8; else promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & ~OccupiedBB() & RANK1;
			while (promotionTarget) {
				Square to = lsb(promotionTarget);
				Square from = Square(to - PawnStep());
				AddMove(createMove<PROMOTION>(from, to, QUEEN));
				AddMove(createMove<PROMOTION>(from, to, ROOK));
				AddMove(createMove<PROMOTION>(from, to, BISHOP));
				AddMove(createMove<PROMOTION>(from, to, KNIGHT));
				promotionTarget &= promotionTarget - 1;
			}
			//King Captures are always winning as kings can only capture uncovered pieces
			Square kingSquare = lsb(PieceBB(KING, SideToMove));
			Bitboard kingTargets = KingAttacks[kingSquare] & ColorBB(SideToMove ^1) & ~attackedByThem;
			while (kingTargets) {
				AddMove(createMove(kingSquare, lsb(kingTargets)));
				kingTargets &= kingTargets - 1;
			}
		}
	}
	AddMove(MOVE_NONE);
	return &moves[0];
}



