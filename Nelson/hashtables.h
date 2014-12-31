#pragma once

#include "types.h"
#include "board.h"
#include "settings.h"


namespace pawn {

	struct Entry {
		PawnKey_t Key;
		Value Score;
		Bitboard attackSet[2];
	};

	extern Entry Table[PAWN_TABLE_SIZE];

	void initialize();

	Entry * probe(const position &pos);

}