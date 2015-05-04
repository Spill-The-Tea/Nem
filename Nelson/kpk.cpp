#include "kpk.h"
#include <vector>
#include <string>

namespace kpk {


	// There are 24 possible pawn squares: the first 4 files and ranks from 2 to 7
	const unsigned MAX_INDEX = 2 * 24 * 64 * 64; // stm * psq * wksq * bksq = 196608

	// Each uint32_t stores results of 32 positions, one per bit
	static uint32_t KPKBitbase[6144];

	// A KPK bitbase index is an integer in [0, IndexMax] range
	//
	// Information is mapped in a way that minimizes the number of iterations:
	//
	// bit 0- 5: white king square (from SQ_A1 to SQ_H8)
	// bit 6-11: black king square (from SQ_A1 to SQ_H8)
	// bit 12: side to move (WHITE or BLACK)
	// bit 13-14: white pawn file (from FILE_A to FILE_D)
	// bit 15-17: white pawn RANK_7 - rank (from RANK_7 - RANK_7 to RANK_7 - RANK_2)
	uint32_t index(Color us, Square bksq, Square wksq, Square psq) {
		return wksq | (bksq << 6) | (us << 12) | ((psq & 7) << 13) | ((Rank7 - (psq >> 3)) << 15);
	}

	enum Result {
		INVALID = 0,
		UNKNOWN = 1,
		DRAW = 2,
		WIN = 4
	};

	inline Result& operator|=(Result& r, Result v) { return r = Result(r | v); }

	struct KPKPosition {

		KPKPosition(unsigned idx);
		operator Result() const { return result; }
		int GetResult() const { return int(result); }
		Result classify(const std::vector<KPKPosition>& db)
		{
			return us == WHITE ? classify<WHITE>(db) : classify<BLACK>(db);
		}

	private:
		template<Color Us> Result classify(const std::vector<KPKPosition>& db);

		Color us;
		Square bksq, wksq, psq;
		Result result;
	};


	bool probe_kpk(Square wksq, Square wpsq, Square bksq, Color us) {

		assert((wpsq & 7) <= FileD);

		uint32_t idx = index(us, bksq, wksq, wpsq);
		return (KPKBitbase[idx / 32] & (1 << (idx & 0x1F))) != 0;
	}


	void init_kpk() {

		unsigned idx, repeat = 1;
		std::vector<KPKPosition> db;
		db.reserve(MAX_INDEX);

		// Initialize db with known win / draw positions
		for (idx = 0; idx < MAX_INDEX; ++idx)
			db.push_back(KPKPosition(idx));

		// Iterate through the positions until none of the unknown positions can be
		// changed to either wins or draws (15 cycle.s needed).
		while (repeat)
			for (repeat = idx = 0; idx < MAX_INDEX; ++idx)
				repeat |= (db[idx] == UNKNOWN && db[idx].classify(db) != UNKNOWN);

		// Map 32 results into one KPKBitbase[] entry
		for (idx = 0; idx < MAX_INDEX; ++idx) {
			if (db[idx] == WIN)
				KPKBitbase[idx >> 5] |= 1 << (idx & 0x1F);
		}
	}


	KPKPosition::KPKPosition(unsigned idx) {

		wksq = Square((idx >> 0) & 0x3F);
		bksq = Square((idx >> 6) & 0x3F);
		us = Color((idx >> 12) & 0x01);
		psq = createSquare(Rank7 - Rank((idx >> 15) & 0x7), File((idx >> 13) & 0x3));
		result = UNKNOWN;

		// Check if two pieces are on the same square or if a king can be captured
		Bitboard bbBKing = ToBitboard(bksq);
		if ((KingAttacks[wksq] & bbBKing) != 0
			|| wksq == bksq
			|| wksq == psq
			|| bksq == psq
			|| (us == WHITE && (PawnAttacks[WHITE][psq] & bbBKing) != 0))
			result = INVALID;
		else if (us == WHITE)
		{
			// Immediate win if a pawn can be promoted without getting captured
			if ((psq >> 3) == Rank7
				&& wksq != psq + 8 //White King doesn't block own pawn
				&& (ChebishevDistance(bksq, Square(psq + 8)) > 1 //Black King doesn't control conversion square
				|| (KingAttacks[wksq] & ToBitboard(psq + 8))))//or White King controls conversion square
				result = WIN;
		}
		// Immediate draw if it is a stalemate or a king captures undefended pawn
		else if (!(KingAttacks[bksq] & (~(KingAttacks[wksq] | PawnAttacks[WHITE][psq]))) //Stalemate detection
#pragma warning(suppress: 6385)
			|| (KingAttacks[bksq] & psq & ~KingAttacks[wksq]))
			result = DRAW;
	}

	template<Color Us>
	Result KPKPosition::classify(const std::vector<KPKPosition>& db) {

		// White to Move: If one move leads to a position classified as WIN, the result
		// of the current position is WIN. If all moves lead to positions classified
		// as DRAW, the current position is classified as DRAW, otherwise the current
		// position is classified as UNKNOWN.
		//
		// Black to Move: If one move leads to a position classified as DRAW, the result
		// of the current position is DRAW. If all moves lead to positions classified
		// as WIN, the position is classified as WIN, otherwise the current position is
		// classified as UNKNOWN.

		const Color Them = (Us == WHITE ? BLACK : WHITE);

		Result r = INVALID;
		Bitboard b;
		if (Us == WHITE) b = KingAttacks[wksq]; else b = KingAttacks[bksq];

		while (b) {
			uint32_t indx;
			if
				(Us == WHITE) indx = index(Them, bksq, lsb(b), psq);
			else
				indx = index(Them, lsb(b), wksq, psq);
			r |= db[indx];
			b &= b - 1;
		}

		if (Us == WHITE && (psq >> 3) < Rank7)
		{
			Square s = Square(psq + 8);
			r |= db[index(BLACK, bksq, wksq, s)]; // Single push

			if (Rank(psq >> 3) == Rank2 && s != wksq && s != bksq)
				r |= db[index(BLACK, bksq, wksq, Square(s + 8))]; // Double push
		}

		if (Us == WHITE)
			return result = r & WIN ? WIN : r & UNKNOWN ? UNKNOWN : DRAW;
		else
			return result = r & DRAW ? DRAW : r & UNKNOWN ? UNKNOWN : WIN;
	}

}
