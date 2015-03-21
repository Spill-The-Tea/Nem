#include "hashtables.h"
#include "position.h"
#include <iostream>
#ifdef _MSC_VER
#include <mmintrin.h>
#endif
#include "stdlib.h"
#include "search.h"

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
		Bitboard attacksWhite = ((bbWhite << 9) & NOT_A_FILE) | ((bbWhite << 7) & NOT_H_FILE);
		Bitboard attacksBlack = ((bbBlack >> 9) & NOT_H_FILE) | ((bbBlack >> 7) & NOT_A_FILE);
		//frontspans
		Bitboard bbWFrontspan = FrontFillNorth(bbWhite);
		Bitboard bbBFrontspan = FrontFillSouth(bbBlack);
		//attacksets
		Bitboard bbWAttackset = FrontFillNorth(attacksWhite);
		Bitboard bbBAttackset = FrontFillSouth(attacksBlack);
		result->passedPawns[WHITE] = bbWhite & (~(bbBAttackset | bbBFrontspan));
		result->passedPawns[BLACK] = bbBlack & (~(bbWAttackset | bbWFrontspan));
		result->Score += popcount(result->passedPawns[WHITE] & RANK4) * PASSED_PAWN_BONUS[0]
			+ popcount(result->passedPawns[WHITE] & RANK5) * PASSED_PAWN_BONUS[1]
			+ popcount(result->passedPawns[WHITE] & RANK6) * PASSED_PAWN_BONUS[2]
			+ popcount(result->passedPawns[WHITE] & RANK7) * PASSED_PAWN_BONUS[3];
		result->Score -= popcount(result->passedPawns[BLACK] & RANK5) * PASSED_PAWN_BONUS[0]
			+ popcount(result->passedPawns[BLACK] & RANK4) * PASSED_PAWN_BONUS[1]
			+ popcount(result->passedPawns[BLACK] & RANK3) * PASSED_PAWN_BONUS[2]
			+ popcount(result->passedPawns[BLACK] & RANK2) * PASSED_PAWN_BONUS[3];
		//bonus for protected passed pawns (on 5th rank or further)
		result->Score += (popcount(result->passedPawns[WHITE] & attacksWhite & HALF_OF_BLACK)
			- popcount(result->passedPawns[BLACK] & attacksBlack & HALF_OF_WHITE)) * BONUS_PROTECTED_PASSED_PAWN;
		//isolated pawns
		Bitboard west = (bbWhite >> 1) & NOT_H_FILE;
		Bitboard east = (bbWhite << 1) & NOT_A_FILE;
		Bitboard  bbIsolatedWhite = bbWhite & ~(FrontFillNorth(west) | FrontFillSouth(west) | FrontFillNorth(east) | FrontFillSouth(east));
		west = (bbBlack >> 1) & NOT_H_FILE;
		east = (bbBlack << 1) & NOT_A_FILE;
		Bitboard  bbIsolatedBlack = bbBlack & ~(FrontFillNorth(west) | FrontFillSouth(west) | FrontFillNorth(east) | FrontFillSouth(east));
		result->Score -= (popcount(bbIsolatedWhite) - popcount(bbIsolatedBlack)) * MALUS_ISOLATED_PAWN;
		result->Key = pos.GetPawnKey();
		return result;
	}
}

namespace tt {

	uint8_t _generation = 0;
    uint64_t ProbeCounter = 0;
	uint64_t HitCounter = 0;
	uint64_t FillCounter = 0;
	std::atomic_flag lock = ATOMIC_FLAG_INIT;

	uint64_t GetProbeCounter() { return ProbeCounter; }
	uint64_t GetHitCounter() { return HitCounter; }
	uint64_t GetFillCounter() { return FillCounter; }
	uint64_t GetHashFull() {
		return 1000 * FillCounter / GetEntryCount();
	}

	Cluster * Table = nullptr;
	uint64_t MASK;

	void InitializeTranspositionTable(int sizeInMB) {
		FreeTranspositionTable();
		int clusterCount = sizeInMB * 1024 * 1024 / sizeof(Cluster);
		clusterCount = 1ul << msb(clusterCount);
		if (clusterCount < 1024) clusterCount = 1024;
		Table = (Cluster *)calloc(clusterCount, sizeof(Cluster));
		MASK = clusterCount - 1;
		std::cout << "infostd::string Hash Size: " << ((clusterCount * sizeof(Cluster)) >> 20) << " MByte" << std::endl;
		ResetCounter();
	}

	void FreeTranspositionTable() {
		if (Table != nullptr) {
			delete[](Table);
			Table = nullptr;
		}
	}

	Entry* firstEntry(const uint64_t hash) {
		return &Table[hash & MASK].entry[0];
	}

	void prefetch(uint64_t hash) {
#ifdef _MSC_VER
		_mm_prefetch((char*)&Table[hash & MASK], _MM_HINT_T0);
#endif // _MSC_VER
#ifdef __GNUC__
        __builtin_prefetch((char*)&Table[hash & MASK]);
#endif // __GNUC__
	}

	uint64_t GetClusterCount() {
		return MASK + 1;
	}
	uint64_t GetEntryCount() {
		return GetClusterCount() * CLUSTER_SIZE;
	}
}
