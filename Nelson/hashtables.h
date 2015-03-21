#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include "types.h"
#include "board.h"
#include "settings.h"


namespace pawn {

	struct Entry {
		PawnKey_t Key;
		Value Score;
		Bitboard passedPawns[2];
	};

	extern Entry Table[PAWN_TABLE_SIZE];

	void initialize();

	Entry * probe(const position &pos);

}

namespace tt {
	enum NodeType { UNDEFINED = 0, UPPER_BOUND = 1, LOWER_BOUND = 2, EXACT = 3 };
	enum ProbeType { UNSAFE, THREAD_SAFE };

	const int CLUSTER_SIZE = 3;

	extern uint8_t _generation;
	inline void newSearch() { _generation += 4; }

	extern uint64_t ProbeCounter;
	extern uint64_t HitCounter;
	extern uint64_t FillCounter;
	extern std::atomic_flag lock;

	uint64_t GetProbeCounter();
	uint64_t GetHitCounter();
	uint64_t GetFillCounter();
	uint64_t GetHashFull();

	inline void ResetCounter() { ProbeCounter = HitCounter = FillCounter = 0; }

	struct Entry {
		uint16_t key;
		Move move;
		Value value;
		Value evalValue;
		uint8_t gentype; //2 bits type 6 bits generation
		int8_t depth;

		NodeType type() const { return (NodeType)(gentype & 0x03); }
		uint8_t generation() const { return gentype & 0xFC; }

		template <ProbeType PT> inline void update(uint64_t hash, Value v, NodeType nt, int d, Move m, Value ev) {
			if (PT)
			{
				while (std::atomic_flag_test_and_set_explicit(&lock, std::memory_order_acquire)); //spinlock
				if (generation() == _generation
					&& (d < depth || (d == depth && type() == EXACT))) { //Check if other thread already stored better entry
					std::atomic_flag_clear_explicit(&lock, std::memory_order_release);
					return;
				}
			}
			FillCounter += (key == 0); //Initial entry get's overwritten
			if (m || (hash >> 48) != key) // Preserve any existing move for the same position
				move = m;
			key = (uint16_t)(hash >> 48);
			value = v;
			evalValue = ev;
			gentype = (uint8_t)(_generation | nt);
			depth = (int8_t)d;
			if (PT) std::atomic_flag_clear_explicit(&lock, std::memory_order_release);
		}

	};

	struct Cluster {
		Entry entry[CLUSTER_SIZE];
		char padding[2];
	};

	void InitializeTranspositionTable(int sizeInMB);

	void FreeTranspositionTable();
	Entry* firstEntry(const uint64_t hash);

	template <ProbeType PT> inline Entry* probe(const uint64_t hash, bool& found, Entry& entry) {
		ProbeCounter++;
		Entry* const tte = firstEntry(hash);
		if (PT) while (std::atomic_flag_test_and_set_explicit(&lock, std::memory_order_acquire));
		for (unsigned i = 0; i < CLUSTER_SIZE; ++i)
			if (!tte[i].key || tte[i].key == (hash >> 48))
			{
				if (tte[i].key) {
					tte[i].gentype = uint8_t(_generation | tte[i].type()); // Refresh
					HitCounter++;
				}
				found = tte[i].key != 0;
				entry = tte[i];
				if (PT) std::atomic_flag_clear_explicit(&lock, std::memory_order_release);
				return &tte[i];
			}
		// Find an entry to be replaced according to the replacement strategy
		Entry* replace = tte;
		for (unsigned i = 1; i < CLUSTER_SIZE; ++i)
			if ((tte[i].generation() == _generation || tte[i].type() == EXACT)
				- (replace->generation() == _generation)
				- (tte[i].depth < replace->depth) < 0)
				replace = &tte[i];
		if (PT) std::atomic_flag_clear_explicit(&lock, std::memory_order_release);
		return found = false, replace;
	}

	void prefetch(uint64_t hash);

	inline Value toTT(Value v, int pliesFromRoot) {
		if (v < -VALUE_MATE_THRESHOLD) {
			//score is -VALUE_MATE + N where N is the number of plies from root of analysis in the final position
			//currently we are at M = pos.GetPliesFromRoot from the root position. Therefore we should be at N-M plies away from
			//the mate and therefore we will store -VALUE_MATE + (N-M) as score
			//Example: We found a mate at ply 18 => score = -VALUE_MATE + 18 = -31982
			//Now we store the value at ply 14 => entry->score = score - 14 = -31982 - 14 = -31996
			return Value(v - pliesFromRoot);
		}
		else if (v > VALUE_MATE_THRESHOLD) {
			//same but inverse logic as above
			return Value(v + pliesFromRoot);
		}
		else return v;
	}

	inline Value fromTT(Value v, int pliesFromRoot) {
		if (v < -VALUE_MATE_THRESHOLD)  return Value(v + pliesFromRoot);
		else if (v > VALUE_MATE_THRESHOLD) return Value(v - pliesFromRoot);
		else return v;
	}

	uint64_t GetClusterCount();
	uint64_t GetEntryCount();
}