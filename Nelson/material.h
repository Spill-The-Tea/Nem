#pragma once

#include "types.h"
#include "evaluation.h"

const int materialKeyFactors[] = { 729, 1458, 486, -405, 279, -270, 246, -244, 2916, 26244 };
const int MATERIAL_KEY_OFFSET = 1839; //this assures that the Material Key is always > 0 => material key = 0 can be used for unusual material due to promotions (2 queens,..) 
const int MATERIAL_KEY_MAX = 729 + 1458 + 2 * (486 + 279 + 246) + 8 * (2916 + 26244) + MATERIAL_KEY_OFFSET;

struct MaterialTableEntry {
	Value Score;
	Phase_t Phase;
	EvalFunction EvaluationFunction;

	inline bool IsLateEndgame() { return EvaluationFunction != &evaluateDefault || Phase > 200; }
};

extern MaterialTableEntry MaterialTable[MATERIAL_KEY_MAX + 1];

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
