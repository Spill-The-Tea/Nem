#include "material.h"
#include "settings.h"
#include "evaluation.h"

using namespace std;

MaterialTableEntry MaterialTable[MATERIAL_KEY_MAX + 1];

MaterialKey_t calculateMaterialKey(int * pieceCounts) {
	MaterialKey_t key = MATERIAL_KEY_OFFSET;
	for (int i = WQUEEN; i <= BPAWN; ++i)
		key += materialKeyFactors[i] * pieceCounts[i];
	return key;
}

void InitializeMaterialTable() {
	MaterialTableEntry undetermined;
	undetermined.Score = VALUE_NOTYETDETERMINED;
	undetermined.Phase = 128;
	undetermined.EvaluationFunction = nullptr;
	std::fill_n(MaterialTable, MATERIAL_KEY_MAX + 1, undetermined);
	int pieceCounts[10];
	for (int nWQ = 0; nWQ <= 1; ++nWQ) {
		pieceCounts[0] = nWQ;
		for (int nBQ = 0; nBQ <= 1; ++nBQ) {
			pieceCounts[1] = nBQ;
			for (int nWR = 0; nWR <= 2; ++nWR) {
				pieceCounts[2] = nWR;
				for (int nBR = 0; nBR <= 2; ++nBR) {
					pieceCounts[3] = nBR;
					for (int nWB = 0; nWB <= 2; ++nWB) {
						pieceCounts[4] = nWB;
						for (int nBB = 0; nBB <= 2; ++nBB) {
							pieceCounts[5] = nBB;
							for (int nWN = 0; nWN <= 2; ++nWN) {
								pieceCounts[6] = nWN;
								for (int nBN = 0; nBN <= 2; ++nBN) {
									pieceCounts[7] = nBN;
									Phase_t phase = Phase(nWQ, nBQ, nWR, nBR, nWB, nBB, nWN, nBN);
									for (int nWP = 0; nWP <= 8; ++nWP) {
										pieceCounts[8] = nWP;
										for (int nBP = 0; nBP <= 8; ++nBP) {
											pieceCounts[9] = nBP;
											MaterialKey_t key = calculateMaterialKey(&pieceCounts[0]);
											assert(key <= MATERIAL_KEY_MAX);
											Value scoreMG = (nWQ - nBQ)*PieceValuesMG[QUEEN] + (nWR - nBR)*PieceValuesMG[ROOK] + (nWB - nBB)*PieceValuesMG[BISHOP] +
												(nWN - nBN) * PieceValuesMG[KNIGHT] + (nWP - nBP) * PieceValuesMG[PAWN];
											Value scoreEG = (nWQ - nBQ)*PieceValuesEG[QUEEN] + (nWR - nBR)*PieceValuesEG[ROOK] + (nWB - nBB)*PieceValuesEG[BISHOP] +
												(nWN - nBN) * PieceValuesEG[KNIGHT] + (nWP - nBP) * PieceValuesEG[PAWN];
											eval evaluation(scoreMG, scoreEG);
											assert(MaterialTable[key].Score == VALUE_NOTYETDETERMINED);
											MaterialTable[key].Score = evaluation.getScore(phase);
											MaterialTable[key].Phase = phase;
											MaterialTable[key].EvaluationFunction = &evaluate;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	for (int i = 0; i < MATERIAL_KEY_MAX + 1; ++i) {
		if (MaterialTable[i].EvaluationFunction == nullptr) MaterialTable[i].EvaluationFunction = &evaluateFromScratch;
	}

}