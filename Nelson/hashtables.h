#pragma once

#include <thread>
#include <atomic>
#include <ios>
#include "types.h"
#include "board.h"
#include "settings.h"


namespace pawn {
	//pawn hash table is currently very simple - it only contains the pawn structure score and a bitboard indicating the passed pawns
	struct Entry {
		PawnKey_t Key;
		Bitboard passedPawns;
		Eval Score;
		uint8_t openFiles;
		uint8_t halfOpenFiles[2];

		inline int assymetry() { return popcount(halfOpenFiles[WHITE] ^ halfOpenFiles[BLACK]); }
	};

	extern Entry Table[PAWN_TABLE_SIZE];

	void initialize();
	void clear();

	//If there is no matching entry the entry is created and the pawn structure evaluation executed
	Entry * probe(const Position &pos);

}

namespace tt {
	enum NodeType { UNDEFINED = 0, UPPER_BOUND = 1, LOWER_BOUND = 2, EXACT = 3 };
	//If engine is running in multi-thread mode, lockless hashing (see https://chessprogramming.wikispaces.com/Shared+Hash+Table#Lockless) is used
	//In single-thread mode, this isn't done
	enum ProbeType { UNSAFE, THREAD_SAFE };

	const int CLUSTER_SIZE = 4;

	extern uint8_t _generation;
	inline void newSearch() { _generation += 4; }

	extern uint64_t ProbeCounter;
	extern uint64_t HitCounter;
	extern uint64_t FillCounter;
	extern uint64_t MASK;

	uint64_t GetProbeCounter();
	uint64_t GetHitCounter();
	uint64_t GetFillCounter();
	uint64_t GetHashFull();

	inline void ResetCounter() { ProbeCounter = HitCounter = FillCounter = 0; }
	void clear();

	//data stored in the transpodition table
	struct NodeData {
		Move move;       //hashmove
		Value value;     //search value
		Value evalValue; //static evaluation value
		uint8_t gentype; //2 bits type 6 bits generation
		int8_t depth;    //depth at which entry has been created
	};

	union DataUnion {
		NodeData details;
		uint64_t dataAsInt;
	};

	struct Entry {
		uint64_t key;
		DataUnion data;

		NodeType type() const { return (NodeType)(data.details.gentype & 0x03); }
		uint8_t generation() const { return data.details.gentype & 0xFC; }
		Value value() { return data.details.value; }
		Move move() { return data.details.move;  }
		Value evalValue() { return data.details.evalValue; }
		int8_t depth() { return data.details.depth; }
		uint64_t GetKey() { return key ^ data.dataAsInt; }

		//Stores a changed 
		template <ProbeType PT> inline void update(uint64_t hash, Value v, NodeType nt, int d, Move m, Value ev) {
			if (PT == THREAD_SAFE) {
				if (m || hash != GetKey()) // Preserve any existing move for the same position
					data.details.move = m;
			}
			else {
				FillCounter += (key == 0); //Initial entry get's overwritten
				if (m || hash != key) // Preserve any existing move for the same position
					data.details.move = m;
			}
			data.details.value = v;
			data.details.evalValue = ev;
			data.details.gentype = (uint8_t)(_generation | nt);
			data.details.depth = (int8_t)d;
			key = PT == THREAD_SAFE ? hash ^ data.dataAsInt : hash;
		}

	};

	struct Cluster {
		Entry entry[CLUSTER_SIZE];
	};

	void InitializeTranspositionTable();

	void FreeTranspositionTable();

	Entry* firstEntry(const uint64_t hash);

	template <ProbeType PT> inline Move hashmove(const uint64_t hash) {
		Entry* const tte = firstEntry(hash);
		if (PT == THREAD_SAFE) {
			for (unsigned i = 0; i < CLUSTER_SIZE; ++i) {
				if (tte[i].GetKey() == hash) return tte[i].move();
			}
		}
		else {
			for (unsigned i = 0; i < CLUSTER_SIZE; ++i) {
				if (tte[i].key == hash) return tte[i].move();
			}
		}
		return MOVE_NONE;
	}

	template <ProbeType PT> inline Value staticEval(const uint64_t hash) {
		Entry* const tte = firstEntry(hash);
		if (PT == THREAD_SAFE) {
			for (unsigned i = 0; i < CLUSTER_SIZE; ++i) {
				if (tte[i].GetKey() == hash) return tte[i].evalValue();
			}
		}
		else {
			for (unsigned i = 0; i < CLUSTER_SIZE; ++i) {
				if (tte[i].key == hash) return tte[i].evalValue();
			}
		}
		return VALUE_NOTYETDETERMINED;
	}

	template <ProbeType PT> inline Entry* probe(const uint64_t hash, bool& found, Entry& entry) {
		Entry* const tte = firstEntry(hash);
		if (PT == THREAD_SAFE) {
			for (unsigned i = 0; i < CLUSTER_SIZE; ++i) {
				if (!tte[i].key || tte[i].GetKey() == hash)
				{
					if (tte[i].key) {
						tte[i].data.details.gentype = uint8_t(_generation | tte[i].type()); // Refresh
					}
					found = tte[i].key != 0;
					entry = tte[i];
					return &tte[i];
				}
			}
		}
		else {
			ProbeCounter++;
			for (unsigned i = 0; i < CLUSTER_SIZE; ++i) {
				if (!tte[i].key || tte[i].key == hash)
				{
					if (tte[i].key) {
						tte[i].data.details.gentype = uint8_t(_generation | tte[i].type()); // Refresh
						HitCounter++;
					}
					found = tte[i].key != 0;
					entry = tte[i];
					return &tte[i];
				}
			}
		}
		// Find an entry to be replaced according to the replacement strategy (which is from an older version of Stockfish)
		Entry* replace = tte;
		for (unsigned i = 1; i < CLUSTER_SIZE; ++i)
			if ((tte[i].generation() == _generation || tte[i].type() == EXACT)
				- (replace->generation() == _generation)
				- (tte[i].depth() < replace->depth()) < 0)
				replace = &tte[i];
		return found = false, replace;
	}

	bool dumpTT(std::ostream &stream);

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

namespace killer {

	const int NB_SLOTS_KILLER = 2;
	const int NB_KILLER = 2;
	const int PLY_TABLE_SIZE = NB_SLOTS_KILLER * (MAX_DEPTH + 1) * 2;

	class Manager {
	public:
		//returns the "index"th killerMove (0 <= index < NB_KILLER) 
		Move getMove(const Position & pos, int index) const;
		//stores a killer move 
		void store(const Position & pos, Move move);
		//clears all killer moves
		void clear();
		//checks if a move is a killer move
		bool isKiller(const Position & pos, Move move) const;
		//Clear killer moves for higher plies
		void enterLevel(const Position& pos);
	private:
		//index of the first killer relevant for the position
		int getIndex(const Position & pos) const;
		int getIndex2(const Position& pos) const;
		//killer table has NB_SLOTS_KILLER (Slots) * MAX_DEPTH (maximum search depth) * 2 (SideToMove) entries
		//Parity: due to null moves there might be entries with same plies from root, with white and with black to move
		Move plyTable[PLY_TABLE_SIZE];
	};

}