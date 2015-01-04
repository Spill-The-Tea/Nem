#include "hashtables.h"
#include "position.h"

namespace pawn {

	Entry Table[PAWN_TABLE_SIZE];

	void initialize() {

	}

	Entry * probe(const position &pos) {
		Entry * result = &Table[pos.GetPawnKey() & (PAWN_TABLE_SIZE - 1)];
		if (result->Key == pos.GetPawnKey()) return result;
		result->Score = VALUE_ZERO;
		Bitboard bbWhite = pos.PieceBB(PAWN, WHITE);
		Bitboard bbBlack = pos.PieceBB(PAWN, BLACK);
		result->attackSet[WHITE] = ((bbWhite << 9) & NOT_A_FILE) | ((bbWhite << 7) & NOT_H_FILE);
		result->attackSet[BLACK] = ((bbBlack >> 9) & NOT_H_FILE) | ((bbBlack >> 7) & NOT_A_FILE);
		//frontspans
		Bitboard bbWFrontspan = FrontFillNorth(bbWhite);
		Bitboard bbBFrontspan = FrontFillSouth(bbBlack);
		//attacksets
		Bitboard bbWAttackset = FrontFillNorth(result->attackSet[WHITE]);
		Bitboard bbBAttackset = FrontFillSouth(result->attackSet[BLACK]);
		result->passedPawns[WHITE] = bbWhite & (~(bbBAttackset | bbBFrontspan));
		result->passedPawns[BLACK] = bbBlack & (~(bbWAttackset | bbWFrontspan));
		for (int rank = 3; rank < 7; ++rank) {
			result->Score += popcount(result->passedPawns[WHITE] & RANKS[rank]) * PASSED_PAWN_BONUS[rank - 3];
			result->Score -= popcount(result->passedPawns[BLACK] & RANKS[7 - rank]) * PASSED_PAWN_BONUS[rank - 3];
		}
		//bonus for protected passed pawns (on 5th rank or further)
		result->Score += (popcount(result->passedPawns[WHITE] & result->attackSet[WHITE] & HALF_OF_BLACK) 
			- popcount(result->passedPawns[BLACK] & result->attackSet[BLACK] & HALF_OF_WHITE)) * BONUS_PROTECTED_PASSED_PAWN;
		//isolated pawns
		//Bitboard west = (bbWhite >> 1) & NOT_H_FILE;
		//Bitboard east = (bbWhite << 1) & NOT_A_FILE;
		//Bitboard bbIsolatedWhite;
		return result;
	}
}