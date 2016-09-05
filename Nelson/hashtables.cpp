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
		clear();
	}

	void clear() {
		std::memset(Table, 0, PAWN_TABLE_SIZE * sizeof(Entry));
	}

	Entry * probe(const position &pos) {
		Entry * result = &Table[pos.GetPawnKey() & (PAWN_TABLE_SIZE - 1)];
		if (result->Key == pos.GetPawnKey()) return result;
		result->Score = eval(0);
		Bitboard bbWhite = pos.PieceBB(PAWN, WHITE);
		Bitboard bbBlack = pos.PieceBB(PAWN, BLACK);
		Bitboard bbFilesWhite = FileFill(bbWhite);
		Bitboard bbFilesBlack = FileFill(bbBlack);
		Bitboard attacksWhite = ((bbWhite << 9) & NOT_A_FILE) | ((bbWhite << 7) & NOT_H_FILE);
		Bitboard attacksBlack = ((bbBlack >> 9) & NOT_H_FILE) | ((bbBlack >> 7) & NOT_A_FILE);
		//frontspans
		Bitboard bbWFrontspan = FrontFillNorth(bbWhite);
		Bitboard bbBFrontspan = FrontFillSouth(bbBlack);
		//attacksets
		Bitboard bbWAttackset = FrontFillNorth(attacksWhite);
		Bitboard bbBAttackset = FrontFillSouth(attacksBlack);
		Bitboard ppW = bbWhite & (~(bbBAttackset | bbBFrontspan));
		Bitboard ppB = bbBlack & (~(bbWAttackset | bbWFrontspan));
		result->passedPawns = ppW | ppB;
		result->openFiles = Fileset(~bbFilesWhite & ~bbFilesBlack);
		result->halfOpenFilesWhite = Fileset(~bbFilesWhite & bbFilesBlack);
		result->halfOpenFilesBlack = Fileset(bbFilesWhite & ~bbFilesBlack);
		while (ppW) {
			int rank = (lsb(ppW) >> 3) - 1;
			result->Score += PASSED_PAWN_BONUS[rank];
			if (rank > 0 && (isolateLSB(ppW) & attacksWhite) != 0) result->Score += BONUS_PROTECTED_PASSED_PAWN[rank];
			ppW &= ppW - 1;
		}
		while (ppB) {
			int rank = 6 - (lsb(ppB) >> 3);
			result->Score -= PASSED_PAWN_BONUS[rank];
			if (rank > 0 && (isolateLSB(ppB) & attacksBlack) != 0) result->Score -= BONUS_PROTECTED_PASSED_PAWN[rank];
			ppB &= ppB - 1;
		}
		//Candidate passed pawns
		Bitboard potentialCandidates = bbWhite & ~bbBFrontspan & bbBAttackset & ~attacksBlack; //open, but not passed pawns
		while (potentialCandidates) {
			Bitboard candidateBB = isolateLSB(potentialCandidates);
			Bitboard sentries = FrontFillNorth(((candidateBB << 17) & NOT_A_FILE) | ((candidateBB << 15) & NOT_H_FILE)) & bbBlack;
			Bitboard helper = FrontFillSouth(sentries >> 16) & bbWhite;
			if (popcount(helper) >= popcount(sentries)) {
				result->Score += BONUS_CANDIDATE * (lsb(candidateBB) >> 3);
			}
			potentialCandidates &= potentialCandidates - 1;
		}
		potentialCandidates = bbBlack & ~bbWFrontspan & bbWAttackset & ~attacksWhite; //open, but not passed pawns
		while (potentialCandidates) {
			Bitboard candidateBB = isolateLSB(potentialCandidates);
			Bitboard sentries = FrontFillSouth(((candidateBB >> 15) & NOT_A_FILE) | ((candidateBB >> 17) & NOT_H_FILE)) & bbWhite;
			Bitboard helper = FrontFillNorth(sentries << 16) & bbBlack;
			if (popcount(helper) >= popcount(sentries)) {
				result->Score -= BONUS_CANDIDATE * (7 - (lsb(candidateBB) >> 3));
			}
			potentialCandidates &= potentialCandidates - 1;
		}
		//Levers 
		Bitboard levers = bbWhite & (RANK5 | RANK6) & attacksBlack;
		while (levers) {
			int leverRank = int(lsb(levers) >> 3) - 3;
			assert(leverRank == 1 || leverRank == 2);
			result->Score += leverRank * BONUS_LEVER;
			levers &= levers - 1;
		}
		levers = bbBlack & (RANK4 | RANK3) & attacksWhite;
		while (levers) {
			int leverRank = 4 - int(lsb(levers) >> 3);
			assert(leverRank == 1 || leverRank == 2);
			result->Score -= leverRank * BONUS_LEVER;
			levers &= levers - 1;
		}
		//isolated pawns
		result->Score -= (popcount(IsolatedFiles(bbFilesWhite)) - popcount(IsolatedFiles(bbFilesBlack))) * MALUS_ISOLATED_PAWN;
		//pawn islands
		result->Score += MALUS_ISLAND_COUNT*(popcount(IslandsEastFiles(bbBlack)) - popcount(IslandsEastFiles(bbWhite)));
		//backward pawns
		Bitboard bbWBackward = bbWhite & ~bbBFrontspan; //Backward pawns are open pawns
		Bitboard frontspan = FrontFillNorth(bbWBackward << 8);
		Bitboard stopSquares = frontspan & attacksBlack; //Where the advancement is stopped by an opposite pawn
		stopSquares &= ~bbWAttackset; //and the stop square isn't part of the own pawn attack
		bbWBackward &= FrontFillSouth(stopSquares);
		Bitboard bbBBackward = bbBlack & ~bbWFrontspan; //Backward pawns are open pawns
		frontspan = FrontFillSouth(bbBBackward >> 8);
		stopSquares = frontspan & attacksWhite; //Where the advancement is stopped by an opposite pawn
		stopSquares &= ~bbBAttackset; //and the stop square isn't part of the own pawn attack
		bbBBackward &= FrontFillNorth(stopSquares);
		result->Score -= (popcount(bbWBackward) - popcount(bbBBackward)) * MALUS_BACKWARD_PAWN;
		//doubled pawns
		Bitboard doubled = FrontFillNorth(bbWhite << 8) & bbWhite;
		result->Score -= popcount(doubled) * MALUS_DOUBLED_PAWN;
		doubled = FrontFillSouth(bbBlack >> 8) & bbBlack;
		result->Score += popcount(doubled) * MALUS_DOUBLED_PAWN;
		result->Key = pos.GetPawnKey();
		return result;
	}
}

namespace tt {

	int initializedSizeInMB = 0;

	uint8_t _generation = 0;
	uint64_t ProbeCounter = 0;
	uint64_t HitCounter = 0;
	uint64_t FillCounter = 0;

	uint64_t GetProbeCounter() { return ProbeCounter; }
	uint64_t GetHitCounter() { return HitCounter; }
	uint64_t GetFillCounter() { return FillCounter; }
	uint64_t GetHashFull() {
		return 1000 * FillCounter / GetEntryCount();
	}

	Cluster * Table = nullptr;
	uint64_t MASK;

	//Calculates the number of clusters in the transposition table if the table size should use
	//sizeMB Megabytes (which is treated as upper limit)
	uint64_t CalculateClusterCount(int SizeMB) {
		uint64_t clusterCount = SizeMB * 1024ull * 1024 / sizeof(Cluster);
		clusterCount = 1ull << msb(clusterCount);
		if (clusterCount < 1024) clusterCount = 1024;
		return clusterCount;
	}

	void InitializeTranspositionTable() {
		int newHashSize = settings::options.getInt(settings::OPTION_HASH);
		if (initializedSizeInMB != newHashSize) {
			FreeTranspositionTable();
			uint64_t clusterCount = CalculateClusterCount(newHashSize);
			Table = static_cast<Cluster *>(calloc(clusterCount, sizeof(Cluster)));
			MASK = clusterCount - 1;
			ResetCounter();
			initializedSizeInMB = newHashSize;
		}
	}

	void clear() {
		std::memset(Table, 0, (MASK + 1) * sizeof(Cluster));
	}

	void FreeTranspositionTable() {
		if (Table != nullptr) {
			delete[](Table);
			Table = nullptr;
		}
	}

	void Clear() {

	}

	uint64_t NMASK;

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

namespace killer {

	ExtendedMove manager::getMove(const position & pos, int index) const
	{
		//Move result;
		//if (index < NB_SLOTS_KILLER) result = plyTable[NB_SLOTS_KILLER * pos.GetPliesFromRoot() + index];
		//else if (pos.GetPliesFromRoot() >= 2) {
		//	result = plyTable[NB_SLOTS_KILLER * (pos.GetPliesFromRoot() - 2) + index];
		//}
		return plyTable[NB_SLOTS_KILLER * pos.GetPliesFromRoot() + index];
		//return ctxtTable[utils::MurmurHash2A(pos.ColorBB(pos.GetSideToMove())) & (KILLER_TABLE_SIZE - 1)];
	}

	void manager::store(const position & pos, Move move)
	{
		ExtendedMove em(pos.GetPieceOnSquare(from(move)), move);
		store(pos, em);
		//ctxtTable[utils::MurmurHash2A(pos.ColorBB(pos.GetSideToMove())) & (KILLER_TABLE_SIZE - 1)] = move;
	}

	void manager::store(const position & pos, ExtendedMove move)
	{
		int pfr = pos.GetPliesFromRoot();
		//for (int i = NB_SLOTS_KILLER - 1; i > 0; --i)
		plyTable[NB_SLOTS_KILLER * pfr + 1] = plyTable[NB_SLOTS_KILLER * pfr];
		plyTable[NB_SLOTS_KILLER * pfr] = move;
	}

	void manager::clear()
	{
		for (int i = 0; i < MAX_DEPTH; ++i) {
			plyTable[2 * i] = EXTENDED_MOVE_NONE;
			plyTable[2 * i + 1] = EXTENDED_MOVE_NONE;
		}
	}

	bool manager::isKiller(const position & pos, Move move)
	{
		int pfr = pos.GetPliesFromRoot();
		return (move == plyTable[NB_SLOTS_KILLER * pfr].move) || (move == plyTable[NB_SLOTS_KILLER * pfr + 1].move);
	}

}