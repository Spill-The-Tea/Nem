#include "material.h"
#include "settings.h"
#include "evaluation.h"
#include "bbEndings.h"
#include "position.h"


MaterialTableEntry MaterialTable[MATERIAL_KEY_MAX + 2];

MaterialKey_t calculateMaterialKey(int * pieceCounts) {
	MaterialKey_t key = MATERIAL_KEY_OFFSET;
	for (int i = WQUEEN; i <= BPAWN; ++i)
		key += materialKeyFactors[i] * pieceCounts[i];
	return key;
}

//Calculation (only used for special situations like 3 Queens, ...)
Value calculateMaterialScore(position &pos) {
	int diffQ = popcount(pos.PieceBB(QUEEN, WHITE)) - popcount(pos.PieceBB(QUEEN, BLACK));
	int diffR = popcount(pos.PieceBB(ROOK, WHITE)) - popcount(pos.PieceBB(ROOK, BLACK));
	int diffB = popcount(pos.PieceBB(BISHOP, WHITE)) - popcount(pos.PieceBB(BISHOP, BLACK));
	int diffN = popcount(pos.PieceBB(KNIGHT, WHITE)) - popcount(pos.PieceBB(KNIGHT, BLACK));
	int diffP = popcount(pos.PieceBB(PAWN, WHITE)) - popcount(pos.PieceBB(PAWN, BLACK));
	return diffQ*PieceValuesMG[QUEEN] + diffR*PieceValuesMG[ROOK] + diffB*PieceValuesMG[BISHOP] + diffN * PieceValuesMG[KNIGHT] + diffP * PieceValuesMG[PAWN];

}

void InitializeMaterialTable() {
	MaterialTableEntry undetermined;
	undetermined.Score = VALUE_NOTYETDETERMINED;
	undetermined.Phase = 128;
	undetermined.EvaluationFunction = nullptr;
	undetermined.Flags = MSF_DEFAULT;
	std::fill_n(MaterialTable, MATERIAL_KEY_MAX + 2, undetermined);
	MaterialTable[MATERIAL_KEY_UNUSUAL].EvaluationFunction = &evaluateDefault;
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
											if (nWB == 2 && nBB < 2) evaluation += BONUS_BISHOP_PAIR; else if (nBB == 2 && nWB < 2) evaluation -= BONUS_BISHOP_PAIR;
											assert(MaterialTable[key].Score == VALUE_NOTYETDETERMINED);
											MaterialTable[key].Score = evaluation.getScore(phase);
											MaterialTable[key].Phase = phase;
											if (nWQ == 0 && nBQ == 0 && nWR == 0 && nBR == 0 && nWB == 0 && nBB == 0 && nWN == 0 && nBN == 0) MaterialTable[key].EvaluationFunction = &evaluatePawnEnding;
											else if (nWP == 0 && nWN == 0 && nWB == 0 && nWR == 0 && nWQ == 0) MaterialTable[key].EvaluationFunction = &easyMate < BLACK >;
											else if (nBP == 0 && nBN == 0 && nBB == 0 && nBR == 0 && nBQ == 0) MaterialTable[key].EvaluationFunction = &easyMate < WHITE > ;										
											else MaterialTable[key].EvaluationFunction = &evaluateDefault;
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
	//Add theoretically drawn endgames
	for (int i = 0; i < 10; ++i) pieceCounts[i] = 0;
	//KK
	MaterialKey_t key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	MaterialTable[key].Flags |= MSF_THEORETICAL_DRAW;
	//KBK
	pieceCounts[WBISHOP] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	MaterialTable[key].Flags |= MSF_THEORETICAL_DRAW;
	pieceCounts[WBISHOP] = 0; pieceCounts[BBISHOP] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	MaterialTable[key].Flags |= MSF_THEORETICAL_DRAW;
	pieceCounts[BBISHOP] = 0;
	//KNK 
	pieceCounts[WKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	MaterialTable[key].Flags |= MSF_THEORETICAL_DRAW;
	pieceCounts[WKNIGHT] = 0; pieceCounts[BKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	MaterialTable[key].Flags |= MSF_THEORETICAL_DRAW;
	pieceCounts[BKNIGHT] = 0;
	//KNNK 
	pieceCounts[WKNIGHT] = 2;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	MaterialTable[key].Flags |= MSF_THEORETICAL_DRAW;
	pieceCounts[WKNIGHT] = 0; pieceCounts[BKNIGHT] = 2;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	MaterialTable[key].Flags |= MSF_THEORETICAL_DRAW;
	pieceCounts[BKNIGHT] = 0;
	for (int i = 0; i < 10; ++i) pieceCounts[i] = 0;
	//KPK
	pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &kpk::EvaluateKPK < WHITE > ;
	pieceCounts[WPAWN] = 0;
	pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &kpk::EvaluateKPK < BLACK > ;
	//KQK, KRK
	pieceCounts[BPAWN] = 0;
	pieceCounts[WQUEEN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE > ;
	pieceCounts[BQUEEN] = 1;
	pieceCounts[WQUEEN] = 0;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK > ;
	pieceCounts[BQUEEN] = 0;
	pieceCounts[WROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE > ;
	pieceCounts[WROOK] = 0;
	pieceCounts[BROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK > ;
	pieceCounts[BROOK] = 0;
	//KBBK
	pieceCounts[WBISHOP] = 2;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE > ;
	pieceCounts[WBISHOP] = 0;
	pieceCounts[BBISHOP] = 2;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK > ;
	pieceCounts[BBISHOP] = 0;
	//KBNK
	pieceCounts[WBISHOP] = 1;
	pieceCounts[WKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKNBK < WHITE > ;
	pieceCounts[WBISHOP] = 0;
	pieceCounts[WKNIGHT] = 0;
	pieceCounts[BBISHOP] = 1;
	pieceCounts[BKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKNBK < BLACK > ;
	//KQKR
	pieceCounts[BBISHOP] = 0;
	pieceCounts[BKNIGHT] = 0;
	pieceCounts[WQUEEN] = 1;
	pieceCounts[BROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE > ;
	pieceCounts[WQUEEN] = 0;
	pieceCounts[BROOK] = 0;
	pieceCounts[BQUEEN] = 1;
	pieceCounts[WROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK > ;
	pieceCounts[BQUEEN] = 0;
	pieceCounts[WROOK] = 0;
	//Simple KxKx Endgames should all be draws (x <> PAWN)
	for (PieceType pt = QUEEN; pt < PAWN; ++pt) {
		pieceCounts[GetPiece(pt, WHITE)] = pieceCounts[GetPiece(pt, BLACK)] = 1;
		key = calculateMaterialKey(&pieceCounts[0]);
		MaterialTable[key].EvaluationFunction = &evaluateDraw;
		pieceCounts[GetPiece(pt, WHITE)] = pieceCounts[GetPiece(pt, BLACK)] = 0;
	}
	//KBPK
	pieceCounts[WBISHOP] = 1;
	pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKBPK < WHITE > ;
	pieceCounts[WBISHOP] = 0;
	pieceCounts[WPAWN] = 0;
	pieceCounts[BBISHOP] = 1;
	pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKBPK < BLACK > ;
	pieceCounts[BBISHOP] = 0;
	pieceCounts[BPAWN] = 0;
	//KQKP
	pieceCounts[WQUEEN] = pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQKP < WHITE > ;
	MaterialTable[key].Flags |= MSF_SKIP_PRUNING;
	pieceCounts[WQUEEN] = pieceCounts[BPAWN] = 0;
	pieceCounts[BQUEEN] = pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQKP < BLACK > ;
	MaterialTable[key].Flags |= MSF_SKIP_PRUNING;
	pieceCounts[BQUEEN] = pieceCounts[WPAWN] = 0;
	//KRKP
	pieceCounts[WROOK] = pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKRKP<WHITE>;
	pieceCounts[WROOK] = pieceCounts[BPAWN] = 0;
	pieceCounts[BROOK] = pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKRKP<BLACK>;
	pieceCounts[BROOK] = pieceCounts[WPAWN] = 0;
	//KNKP
	pieceCounts[WKNIGHT] = pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKNKP<WHITE>;
	pieceCounts[WKNIGHT] = pieceCounts[BPAWN] = 0;
	pieceCounts[BKNIGHT] = pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKNKP<BLACK>;
	pieceCounts[BKNIGHT] = pieceCounts[WPAWN] = 0;
	//KBKP
	pieceCounts[WBISHOP] = pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKBKP<WHITE>;
	pieceCounts[WBISHOP] = pieceCounts[BPAWN] = 0;
	pieceCounts[BBISHOP] = pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKBKP<BLACK>;
	pieceCounts[BBISHOP] = pieceCounts[WPAWN] = 0;
	//KNKPx
	pieceCounts[WKNIGHT] = 1;
	for (int pawnCount = 2; pawnCount < 9; ++pawnCount) {
		pieceCounts[BPAWN] = pawnCount;
		key = calculateMaterialKey(&pieceCounts[0]);
		MaterialTable[key].EvaluationFunction = &evaluateKNKPx<WHITE>;
	}
	pieceCounts[WKNIGHT] = pieceCounts[BPAWN] = 0;
	pieceCounts[BKNIGHT] = 1;
	for (int pawnCount = 2; pawnCount < 9; ++pawnCount) {
		pieceCounts[WPAWN] = pawnCount;
		key = calculateMaterialKey(&pieceCounts[0]);
		MaterialTable[key].EvaluationFunction = &evaluateKNKPx<BLACK>;
	}
	pieceCounts[BKNIGHT] = pieceCounts[WPAWN] = 0;
	//KBKPx
	pieceCounts[WBISHOP] = 1;
	for (int pawnCount = 2; pawnCount < 9; ++pawnCount) {
		pieceCounts[BPAWN] = pawnCount;
		key = calculateMaterialKey(&pieceCounts[0]);
		MaterialTable[key].EvaluationFunction = &evaluateKBKPx<WHITE>;
	}
	pieceCounts[WBISHOP] = pieceCounts[BPAWN] = 0;
	pieceCounts[BBISHOP] = 1;
	for (int pawnCount = 2; pawnCount < 9; ++pawnCount) {
		pieceCounts[WPAWN] = pawnCount;
		key = calculateMaterialKey(&pieceCounts[0]);
		MaterialTable[key].EvaluationFunction = &evaluateKBKPx<BLACK>;
	}
	pieceCounts[BBISHOP] = pieceCounts[WPAWN] = 0;
	//KQKQP
	pieceCounts[WQUEEN] = pieceCounts[WPAWN] = pieceCounts[BQUEEN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQPKQ<WHITE>;
	//MaterialTable[key].Flags |= MSF_SKIP_PRUNING;
	pieceCounts[WPAWN] = 0;
	pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQPKQ<BLACK>;
	//MaterialTable[key].Flags |= MSF_SKIP_PRUNING;
	pieceCounts[WQUEEN] = pieceCounts[BPAWN] = pieceCounts[BQUEEN] = 0;
}