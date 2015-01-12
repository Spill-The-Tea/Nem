#include "hashtables.h"
#include "position.h"
#include <iostream>
#include <mmintrin.h>
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
		result->Score += popcount(result->passedPawns[WHITE] & RANK4) * PASSED_PAWN_BONUS[0]
			+ popcount(result->passedPawns[WHITE] & RANK5) * PASSED_PAWN_BONUS[1]
			+ popcount(result->passedPawns[WHITE] & RANK6) * PASSED_PAWN_BONUS[2]
			+ popcount(result->passedPawns[WHITE] & RANK7) * PASSED_PAWN_BONUS[3];
		result->Score -= popcount(result->passedPawns[BLACK] & RANK5) * PASSED_PAWN_BONUS[0]
			+ popcount(result->passedPawns[BLACK] & RANK4) * PASSED_PAWN_BONUS[1]
			+ popcount(result->passedPawns[BLACK] & RANK3) * PASSED_PAWN_BONUS[2]
			+ popcount(result->passedPawns[BLACK] & RANK2) * PASSED_PAWN_BONUS[3];
		//bonus for protected passed pawns (on 5th rank or further)
		result->Score += (popcount(result->passedPawns[WHITE] & result->attackSet[WHITE] & HALF_OF_BLACK) 
			- popcount(result->passedPawns[BLACK] & result->attackSet[BLACK] & HALF_OF_WHITE)) * BONUS_PROTECTED_PASSED_PAWN;
		//isolated pawns
		Bitboard west = (bbWhite >> 1) & NOT_H_FILE;
		Bitboard east = (bbWhite << 1) & NOT_A_FILE;
		Bitboard  bbIsolatedWhite = bbWhite & ~(FrontFillNorth(west) | FrontFillSouth(west) | FrontFillNorth(east) | FrontFillSouth(east));
		west = (bbBlack >> 1) & NOT_H_FILE;
		east = (bbBlack << 1) & NOT_A_FILE;
		Bitboard  bbIsolatedBlack = bbBlack & ~(FrontFillNorth(west) | FrontFillSouth(west) | FrontFillNorth(east) | FrontFillSouth(east));
		result->Score -= (popcount(bbIsolatedWhite) - popcount(bbIsolatedBlack)) * MALUS_ISOLATED_PAWN;
		return result;
	}
}

namespace tt {

    uint64_t ProbeCounter = 0;
	uint64_t HitCounter = 0;
	uint64_t FillCounter = 0;

	uint64_t GetProbeCounter() { return ProbeCounter; }
	uint64_t GetHitCounter() { return HitCounter; }
	uint64_t GetFillCounter() { return HitCounter; }
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
		cout << "info string Hash Size: " << ((clusterCount * sizeof(Cluster)) >> 20) << " MByte" << endl;
		ResetCounter();
	}

	void FreeTranspositionTable() {
		if (Table != nullptr) {
			delete[](Table);
			Table = nullptr;
		}
	}

	Entry* probe(const uint64_t hash, bool& found) {
	   ProbeCounter++;
       Entry* const tte = firstEntry(hash);
		for (unsigned i = 0; i < CLUSTER_SIZE; ++i)
			if (!tte[i].key || tte[i].key == hash)
			{
				if (tte[i].key) {
					tte[i].gentype = uint8_t(_generation | tte[i].type()); // Refresh
					HitCounter++;
				}
				return found = tte[i].key != 0, &tte[i];
			}
		// Find an entry to be replaced according to the replacement strategy
		Entry* replace = tte;
		for (unsigned i = 1; i < CLUSTER_SIZE; ++i)
			if ((tte[i].generation() == _generation || tte[i].type() == EXACT)
				- (replace->generation() == _generation)
				- (tte[i].depth < replace->depth) < 0)
				replace = &tte[i];
		return found = false, replace;
	}

	Entry* firstEntry(const uint64_t hash) {
		return &Table[hash & MASK].entry[0];
	}

	void prefetch(uint64_t hash) {
		_mm_prefetch((char*)&Table[hash & MASK], _MM_HINT_T0);
	}

	uint64_t GetClusterCount() {
		return MASK + 1;
	}
	uint64_t GetEntryCount() {
		return GetClusterCount() * CLUSTER_SIZE;
	}
}