#include "material.h"
#include "settings.h"
#include "evaluation.h"
#include "bbEndings.h"
#include "position.h"

#ifdef TB
#include "tablebase.h"
#endif


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
	return diffQ*PieceValues[QUEEN].mgScore + diffR*PieceValues[ROOK].mgScore + diffB*PieceValues[BISHOP].mgScore + diffN * PieceValues[KNIGHT].mgScore + diffP * PieceValues[PAWN].mgScore;

}

void InitializeMaterialTable() {
	MaterialTableEntry undetermined;
	undetermined.Score = VALUE_NOTYETDETERMINED;
	undetermined.Phase = 128;
	undetermined.EvaluationFunction = nullptr;
	undetermined.Flags = MSF_DEFAULT;
	undetermined.MostValuedPiece = 0;
	std::fill_n(MaterialTable, MATERIAL_KEY_MAX + 2, undetermined);
	MaterialTable[MATERIAL_KEY_UNUSUAL].EvaluationFunction = &evaluateDefault;
	int pieceCounts[10];
	int imbalance[5];
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
											eval evaluation(0);
											for (int i = 0; i < 5; ++i) {
												imbalance[i] = pieceCounts[2 * i] - pieceCounts[2 * i + 1];
												evaluation += imbalance[i] * PieceValues[i];
											}
											if ((nWB == 2 || nBB == 2) && imbalance[BISHOP] != 0) {
												//Bonus for bishop pair
												if (nWB == 2) {//White has Bishop pair
													evaluation += BONUS_BISHOP_PAIR + (9 - nWP - nBP)*settings::SCALE_BISHOP_PAIR_WITH_PAWNS;
													if (nBN == 0 && nBB == 0) evaluation += settings::BONUS_BISHOP_PAIR_NO_OPP_MINOR;
												}
												else {
													evaluation -= BONUS_BISHOP_PAIR + (9 - nWP - nBP)*settings::SCALE_BISHOP_PAIR_WITH_PAWNS;
													if (nWN == 0 && nWB == 0) evaluation -= settings::BONUS_BISHOP_PAIR_NO_OPP_MINOR;
												}
											}
											if (imbalance[ROOK] != 0 && ((imbalance[ROOK] + imbalance[KNIGHT] + imbalance[BISHOP]) == 0)) {
												evaluation += imbalance[ROOK] * ((3-nWQ-nBQ-nWR-nBR) * settings::SCALE_EXCHANGE_WITH_MAJORS
													                + (8-nWP-nBP)*settings::SCALE_EXCHANGE_WITH_PAWNS);
											}
											assert(MaterialTable[key].Score == VALUE_NOTYETDETERMINED);
											MaterialTable[key].Score = evaluation.getScore(phase);
											MaterialTable[key].Phase = phase;
											if (nWQ == 0 && nBQ == 0 && nWR == 0 && nBR == 0 && nWB == 0 && nBB == 0 && nWN == 0 && nBN == 0) MaterialTable[key].EvaluationFunction = &evaluatePawnEnding;
											else if (nWP == 0 && nWN == 0 && nWB == 0 && nWR == 0 && nWQ == 0) MaterialTable[key].EvaluationFunction = &easyMate < BLACK >;
											else if (nBP == 0 && nBN == 0 && nBB == 0 && nBR == 0 && nBQ == 0) MaterialTable[key].EvaluationFunction = &easyMate < WHITE >;
											else MaterialTable[key].EvaluationFunction = &evaluateDefault;
											if (nWQ > 0) MaterialTable[key].setMostValuedPiece(WHITE, QUEEN);
											else if (nWR > 0) MaterialTable[key].setMostValuedPiece(WHITE, ROOK);
											else if (nWB > 0) MaterialTable[key].setMostValuedPiece(WHITE, BISHOP);
											else if (nWN > 0) MaterialTable[key].setMostValuedPiece(WHITE, KNIGHT);
											else if (nWP > 0) MaterialTable[key].setMostValuedPiece(WHITE, PAWN);
											else MaterialTable[key].setMostValuedPiece(WHITE, KING);
											if (nBQ > 0) MaterialTable[key].setMostValuedPiece(BLACK, QUEEN);
											else if (nBR > 0) MaterialTable[key].setMostValuedPiece(BLACK, ROOK);
											else if (nBB > 0) MaterialTable[key].setMostValuedPiece(BLACK, BISHOP);
											else if (nBN > 0) MaterialTable[key].setMostValuedPiece(BLACK, KNIGHT);
											else if (nBP > 0) MaterialTable[key].setMostValuedPiece(BLACK, PAWN);
											else MaterialTable[key].setMostValuedPiece(BLACK, KING);
											if (MaterialTable[key].EvaluationFunction == &evaluateDefault) {
												if (MaterialTable[key].GetMostExpensivePiece(BLACK) != KING && nWQ == 0 && nWR == 0 && nWB == 1 && nWN == 0 && nWP == 1) 
													MaterialTable[key].EvaluationFunction = &evaluateKBPKx<WHITE>;
												else if (MaterialTable[key].GetMostExpensivePiece(WHITE) != KING && nBQ == 0 && nBR == 0 && nBB == 1 && nBN == 0 && nBP == 1) 
													MaterialTable[key].EvaluationFunction = &evaluateKBPKx<BLACK>;
											}
#ifdef TB
											if (Tablebases::MaxCardinality > 0) {
												int totalPieceCount = 2;
												for (int i = 0; i < 10; ++i) totalPieceCount += pieceCounts[i];
												if (totalPieceCount <= Tablebases::MaxCardinality) 
													MaterialTable[key].Flags |= MSF_TABLEBASE_ENTRY;
											}
#endif
											assert(nWQ == (MaterialTable[key].GetMostExpensivePiece(WHITE) == QUEEN));
											assert(nBQ == (MaterialTable[key].GetMostExpensivePiece(BLACK) == QUEEN));
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
	//KBPxKBPx
	pieceCounts[WBISHOP] = pieceCounts[BBISHOP] = 1;
	for (int wP = 0; wP <= 8; ++wP) {
		for (int bP = 0; bP <= 8; ++bP) {
			pieceCounts[WPAWN] = wP;
			pieceCounts[BPAWN] = bP;
			MaterialKey_t key = calculateMaterialKey(&pieceCounts[0]);
			MaterialTable[key].EvaluationFunction = &evaluateKBPxKBPx;
		}
	}
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
	pieceCounts[WKNIGHT] = 0; pieceCounts[BKNIGHT] = 2;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = VALUE_DRAW;
	MaterialTable[key].EvaluationFunction = &evaluateDraw;
	pieceCounts[BKNIGHT] = 0;
	for (int i = 0; i < 10; ++i) pieceCounts[i] = 0;
	//KPK
	pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &kpk::EvaluateKPK < WHITE >;
	pieceCounts[WPAWN] = 0;
	pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &kpk::EvaluateKPK < BLACK >;
	//KQK, KRK
	pieceCounts[BPAWN] = 0;
	pieceCounts[WQUEEN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE >;
	pieceCounts[BQUEEN] = 1;
	pieceCounts[WQUEEN] = 0;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK >;
	pieceCounts[BQUEEN] = 0;
	pieceCounts[WROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE >;
	pieceCounts[WROOK] = 0;
	pieceCounts[BROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK >;
	pieceCounts[BROOK] = 0;
	//KBBK
	pieceCounts[WBISHOP] = 2;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE >;
	pieceCounts[WBISHOP] = 0;
	pieceCounts[BBISHOP] = 2;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK >;
	pieceCounts[BBISHOP] = 0;
	//KBNK
	pieceCounts[WBISHOP] = 1;
	pieceCounts[WKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKNBK < WHITE >;
	pieceCounts[WBISHOP] = 0;
	pieceCounts[WKNIGHT] = 0;
	pieceCounts[BBISHOP] = 1;
	pieceCounts[BKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKNBK < BLACK >;
	//KQKR
	pieceCounts[BBISHOP] = 0;
	pieceCounts[BKNIGHT] = 0;
	pieceCounts[WQUEEN] = 1;
	pieceCounts[BROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < WHITE >;
	pieceCounts[WQUEEN] = 0;
	pieceCounts[BROOK] = 0;
	pieceCounts[BQUEEN] = 1;
	pieceCounts[WROOK] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &easyMate < BLACK >;
	pieceCounts[BQUEEN] = 0;
	pieceCounts[WROOK] = 0;
	//Simple KxKx Endgames should all be draws (x <> PAWN)
	for (PieceType pt = QUEEN; pt < PAWN; ++pt) {
		pieceCounts[GetPiece(pt, WHITE)] = pieceCounts[GetPiece(pt, BLACK)] = 1;
		key = calculateMaterialKey(&pieceCounts[0]);
		MaterialTable[key].EvaluationFunction = &evaluateDraw;
		pieceCounts[GetPiece(pt, WHITE)] = pieceCounts[GetPiece(pt, BLACK)] = 0;
	}
	//KRKN and KRKB is also very drawish
	pieceCounts[WROOK] = pieceCounts[BBISHOP] = 1;
	key = calculateMaterialKey(&pieceCounts[0]); //KRKB
	MaterialTable[key].Score = Value(20);
	pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]); //KRKBP
	MaterialTable[key].Score = Value(10);
	pieceCounts[BBISHOP] = 0; pieceCounts[BKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);  //KRKNP
	MaterialTable[key].Score = Value(10);
	pieceCounts[BPAWN] = 0;
	key = calculateMaterialKey(&pieceCounts[0]); //KRKN
	MaterialTable[key].Score = Value(20);
	pieceCounts[BKNIGHT] = 0; pieceCounts[WROOK] = 0;
	pieceCounts[BROOK] = pieceCounts[WBISHOP] = 1;
	key = calculateMaterialKey(&pieceCounts[0]); //KBKR
	MaterialTable[key].Score = -Value(20);
	pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]); //KBPKR
	MaterialTable[key].Score = -Value(10);
	pieceCounts[WBISHOP] = 0; pieceCounts[WKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]); //KNPKR
	MaterialTable[key].Score = -Value(10);
	pieceCounts[WPAWN] = 0;
	key = calculateMaterialKey(&pieceCounts[0]); //KNKR
	MaterialTable[key].Score = -Value(20);
	pieceCounts[WKNIGHT] = 0; pieceCounts[BROOK] = 0;
	//KRNKR and KRBKR are also drawish
	pieceCounts[WROOK] = pieceCounts[BROOK] = pieceCounts[WBISHOP] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = Value(20);
	pieceCounts[WBISHOP] = 0; pieceCounts[WKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = Value(10);
	pieceCounts[WKNIGHT] = 0; pieceCounts[BBISHOP] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = -Value(20);
	pieceCounts[BBISHOP] = 0; pieceCounts[BKNIGHT] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].Score = -Value(10);
	pieceCounts[WROOK] = pieceCounts[BROOK] = pieceCounts[BKNIGHT] = 0;
	//KBPK
	pieceCounts[WBISHOP] = 1;
	pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKBPK < WHITE >;
	pieceCounts[WBISHOP] = 0;
	pieceCounts[WPAWN] = 0;
	pieceCounts[BBISHOP] = 1;
	pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKBPK < BLACK >;
	pieceCounts[BBISHOP] = 0;
	pieceCounts[BPAWN] = 0;
	//KQKP
	pieceCounts[WQUEEN] = pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQKP < WHITE >;
	MaterialTable[key].Flags |= MSF_SKIP_PRUNING;
	pieceCounts[WQUEEN] = pieceCounts[BPAWN] = 0;
	pieceCounts[BQUEEN] = pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQKP < BLACK >;
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
	//KQKRP
	pieceCounts[WQUEEN] = pieceCounts[BROOK] = pieceCounts[BPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQKRP<WHITE>;
	pieceCounts[WQUEEN] = pieceCounts[BROOK] = pieceCounts[BPAWN] = 0;
	pieceCounts[BQUEEN] = pieceCounts[WROOK] = pieceCounts[WPAWN] = 1;
	key = calculateMaterialKey(&pieceCounts[0]);
	MaterialTable[key].EvaluationFunction = &evaluateKQKRP<BLACK>;
	pieceCounts[BQUEEN] = pieceCounts[WROOK] = pieceCounts[WPAWN] = 0;
	}