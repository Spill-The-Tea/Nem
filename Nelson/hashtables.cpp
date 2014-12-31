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
		result->attackSet[WHITE] = ((pos.PieceBB(PAWN, WHITE) << 9) & NOT_A_FILE) | ((pos.PieceBB(PAWN, WHITE) << 7) & NOT_H_FILE);
		result->attackSet[BLACK] = ((pos.PieceBB(PAWN, BLACK) >> 9) & NOT_H_FILE) | ((pos.PieceBB(PAWN, BLACK) >> 7) & NOT_A_FILE);
		return result;
	}
}