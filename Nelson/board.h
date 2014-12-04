#pragma once

#include "types.h"

const Bitboard ALL_SQUARES = 0xffffffffffffffff;
const Bitboard NOT_H_FILE = 0x7f7f7f7f7f7f7f7f;
const Bitboard NOT_A_FILE = 0xfefefefefefefefe;
const Bitboard A_FILE = 0x101010101010101;
const Bitboard B_FILE = A_FILE << 1;
const Bitboard C_FILE = B_FILE << 1;
const Bitboard D_FILE = C_FILE << 1;;
const Bitboard E_FILE = D_FILE << 1;
const Bitboard F_FILE = E_FILE << 1;
const Bitboard G_FILE = F_FILE << 1;
const Bitboard H_FILE = G_FILE << 1;
const Bitboard FILES[] = { A_FILE, B_FILE, C_FILE, D_FILE, E_FILE, F_FILE, G_FILE, H_FILE };
const Bitboard RANK1 = 0xFF;
const Bitboard RANK2 = 0xFF00;
const Bitboard RANK3 = 0xFF0000;
const Bitboard RANK4 = 0xFF000000;
const Bitboard RANK5 = 0xFF00000000;
const Bitboard RANK6 = 0xFF0000000000;
const Bitboard RANK7 = 0xff000000000000;
const Bitboard RANK8 = 0xff00000000000000;
const Bitboard RANKS[] = { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 };
const Bitboard RANK1and8 = RANK1 | RANK8;

extern Bitboard InitialKingSquareBB[2];
extern Square InitialKingSquare[2];
extern Bitboard InitialRookSquareBB[4];
extern Square InitialRookSquare[4];
extern Bitboard SquaresToBeUnattacked[4];
extern Bitboard SquaresToBeEmpty[4];
extern bool Chess960;

extern Bitboard MagicMovesRook[88576];
extern Bitboard MagicMovesBishop[4800];
extern int BishopShift[64];
extern int RookShift[64];
extern Bitboard OccupancyMaskRook[64];
extern Bitboard OccupancyMaskBishop[64];
extern int IndexOffsetRook[64];
extern int IndexOffsetBishop[64];
extern uint64_t RookMagics[64];
extern uint64_t BishopMagics[64];
extern Bitboard InBetweenFields[64][64];
extern Bitboard KnightAttacks[64];
extern Bitboard KingAttacks[64];
extern Bitboard PawnAttacks[2][64];

void Initialize();

inline Bitboard RookTargets(Square rookSquare, Bitboard occupied) {
	int index = (int)(((OccupancyMaskRook[rookSquare] & occupied) * RookMagics[rookSquare]) >> RookShift[rookSquare]);
	return MagicMovesRook[index + IndexOffsetRook[rookSquare]];
}

inline Bitboard BishopTargets(Square bishopSquare, Bitboard occupied) {
	int index = (int)(((OccupancyMaskBishop[bishopSquare] & occupied) * BishopMagics[bishopSquare]) >> BishopShift[bishopSquare]);
	return MagicMovesBishop[index + IndexOffsetBishop[bishopSquare]];
}

inline Bitboard QueenTargets(Square queenSquare, Bitboard occupied) {
	return RookTargets(queenSquare, occupied) | BishopTargets(queenSquare, occupied);
}