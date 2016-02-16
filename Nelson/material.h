#pragma once

#include "types.h"

const int materialKeyFactors[] = { 729, 1458, 486, -405, 279, -270, 246, -244, 2916, 26244, 0, 0, 0 };
const int MAX_PIECE_COUNT[] = { 1, 1, 2, 2, 2, 2, 2, 2 }; //Max piece count "normal" positions
const int MATERIAL_KEY_OFFSET = 1839; //this assures that the Material Key is always > 0 => material key = 0 can be used for unusual material due to promotions (2 queens,..)
const int MATERIAL_KEY_KvK = MATERIAL_KEY_OFFSET;
const int MATERIAL_KEY_MAX = 729 + 1458 + 2 * (486 + 279 + 246) + 8 * (2916 + 26244) + MATERIAL_KEY_OFFSET;
const int MATERIAL_KEY_UNUSUAL = MATERIAL_KEY_MAX + 1; //Entry for unusual Material distribution (like 3 Queens, 5 Bishops, ...)

enum MaterialSearchFlags : uint8_t {
	MSF_DEFAULT = 0,
	MSF_SKIP_PRUNING = 1,
	MSF_THEORETICAL_DRAW = 2,
	MSF_TABLEBASE_ENTRY = 4
};

struct MaterialTableEntry {
	Value Score;
	Phase_t Phase;
	EvalFunction EvaluationFunction;
	uint8_t Flags;
	uint8_t MostValuedPiece; //high bits for black piece type

	inline bool IsLateEndgame() { return EvaluationFunction != &evaluateDefault || Phase > 200; }
	inline bool IsPawnEnding() { return Phase == 256; }
	inline bool SkipPruning() { return (Flags & MSF_SKIP_PRUNING) != 0; }
	inline bool IsTheoreticalDraw() { return (Flags & MSF_THEORETICAL_DRAW) != 0; }
#ifdef TB
	inline bool IsTablebaseEntry() { return (Flags & MSF_TABLEBASE_ENTRY) != 0; }
#endif
	inline PieceType GetMostExpensivePiece(Color color) const { return PieceType((MostValuedPiece >> (4 * (int)color)) & 15); }
	void setMostValuedPiece(Color color, PieceType pt) { 
		MostValuedPiece &= color == BLACK ? 15 : 240;
		MostValuedPiece |= color == BLACK ? pt << 4 : pt;
	}
};

extern MaterialTableEntry MaterialTable[MATERIAL_KEY_MAX + 2];

//Phase is 0 in starting position and grows up to 256 when only kings are left
inline const Phase_t Phase(int nWQ, int nBQ, int nWR, int nBR, int nWB, int nBB, int nWN, int nBN) {
	int phase = 24 - (nWQ + nBQ) * 4
		- (nWR + nBR) * 2
		- nWB - nBB - nWN - nBN;
	phase = (phase * 256 + 12) / 24;
	return Phase_t(phase);
}

void InitializeMaterialTable();

inline MaterialTableEntry * probe(MaterialKey_t key) { return &MaterialTable[key]; }

Value calculateMaterialScore(position &pos);

MaterialKey_t calculateMaterialKey(int * pieceCounts);
