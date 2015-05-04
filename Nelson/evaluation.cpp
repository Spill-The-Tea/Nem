#include "evaluation.h"
#include "position.h"
#include "settings.h"
#include <algorithm>


Value evaluateDefault(const position& pos) {
	evaluation result;
	result.Material = pos.GetMaterialScore();
	result.Mobility = evaluateMobility(pos);
	result.KingSafety = evaluateKingSafety(pos);
	result.PawnStructure = pos.PawnStructureScore();
	result.Threats = evaluateThreats<WHITE>(pos) - evaluateThreats<BLACK>(pos);
	result.Pieces = evaluatePieces<WHITE>(pos) -evaluatePieces<BLACK>(pos);
	//result.Space = evaluateSpace<WHITE>(pos) -evaluateSpace<BLACK>(pos);
	return result.GetScore(pos.GetMaterialTableEntry()->Phase, pos.GetSideToMove());
}

Value evaluateDraw(const position& pos) {
	return VALUE_DRAW;
}

eval evaluateKingSafety(const position& pos) {
	eval result;
	//Areas around the king
	Bitboard kingRingWhite = pos.PieceBB(KING, WHITE) | KingAttacks[lsb(pos.PieceBB(KING, WHITE))];
	Bitboard kingRingBlack = pos.PieceBB(KING, BLACK) | KingAttacks[lsb(pos.PieceBB(KING, BLACK))];
	Bitboard kingZoneWhite = kingRingWhite | (kingRingWhite << 8);
	Bitboard kingZoneBlack = kingRingBlack | (kingRingBlack >> 8);
	int attackUnits = 0;
	int attackerCount = 0;
	int attackCount;
	Bitboard pieceBB = pos.PieceBB(BISHOP, WHITE) | pos.PieceBB(KNIGHT, WHITE);
	while (pieceBB) {
		if ((attackCount = popcount(pos.GetAttacksFrom(lsb(pieceBB))&kingZoneBlack))) {
			attackUnits += 2 * attackCount;
			++attackerCount;
		}
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(ROOK, WHITE);
	while (pieceBB) {
		if ((attackCount = popcount(pos.GetAttacksFrom(lsb(pieceBB))&kingZoneBlack)))  {
			attackUnits += 3 * attackCount;
			++attackerCount;
		}
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(QUEEN, WHITE);
	while (pieceBB) {
		if ((attackCount = popcount(pos.GetAttacksFrom(lsb(pieceBB))&kingZoneBlack))) {
			attackUnits += 5 * attackCount;
			++attackerCount;
		}
		pieceBB &= pieceBB - 1;
	}
	if (attackerCount > 2) result.mgScore = KING_SAFETY[std::min(attackUnits, 99)];
	attackUnits = 0;
	attackerCount = 0;
	pieceBB = pos.PieceBB(BISHOP, BLACK) | pos.PieceBB(KNIGHT, BLACK);
	while (pieceBB) {
		if ((attackCount = popcount(pos.GetAttacksFrom(lsb(pieceBB))&kingZoneWhite))) {
			attackUnits += 2 * attackCount;
			++attackerCount;
		}
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(ROOK, BLACK);
	while (pieceBB) {
		if ((attackCount = popcount(pos.GetAttacksFrom(lsb(pieceBB))&kingZoneWhite))) {
			attackUnits += 3 * attackCount;
			++attackerCount;
		}
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(QUEEN, BLACK);
	while (pieceBB) {
		if ((attackCount = popcount(pos.GetAttacksFrom(lsb(pieceBB))&kingZoneWhite))) {
			attackUnits += 5 * attackCount;
			++attackerCount;
		}
		pieceBB &= pieceBB - 1;
	}
	if (attackerCount > 2) result.mgScore -= KING_SAFETY[std::min(attackUnits, 99)];
	//Pawn shelter
	if (pos.PieceBB(KING, WHITE) & SaveSquaresForKing & HALF_OF_WHITE) { //Bonus only for castled king
		result.mgScore += PAWN_SHELTER_2ND_RANK * popcount(pos.PieceBB(PAWN, WHITE) & kingRingWhite & ShelterPawns2ndRank);
		result.mgScore += PAWN_SHELTER_3RD_RANK * popcount(pos.PieceBB(PAWN, WHITE) & kingZoneWhite & ShelterPawns3rdRank);
	}
	if (pos.PieceBB(KING, BLACK) & SaveSquaresForKing & HALF_OF_BLACK) {
		result.mgScore -= PAWN_SHELTER_2ND_RANK * popcount(pos.PieceBB(PAWN, BLACK) & kingRingBlack & ShelterPawns2ndRank);
		result.mgScore -= PAWN_SHELTER_3RD_RANK * popcount(pos.PieceBB(PAWN, BLACK) & kingZoneBlack & ShelterPawns3rdRank);
	}
	return result;
}

eval evaluateMobility(const position& pos) {
	eval result;
	//Create attack bitboards
	Bitboard abbWPawn = pos.AttacksByPieceType(WHITE, PAWN);
	Bitboard abbBPawn = pos.AttacksByPieceType(BLACK, PAWN);
	//Leichtfiguren (N+B)
	Bitboard abbWLeicht = abbWPawn | pos.AttacksByPieceType(WHITE, KNIGHT) | pos.AttacksByPieceType(WHITE, BISHOP);
	Bitboard abbBLeicht = abbBPawn | pos.AttacksByPieceType(BLACK, KNIGHT) | pos.AttacksByPieceType(BLACK, BISHOP);
	//Rooks
	Bitboard abbWRook = abbWLeicht | pos.AttacksByPieceType(WHITE, ROOK);
	Bitboard abbBRook = abbBLeicht | pos.AttacksByPieceType(BLACK, ROOK);
	//Total Attacks
	Bitboard abbWhite = pos.AttacksByColor(WHITE);
	Bitboard abbBlack = pos.AttacksByColor(BLACK);

	//excluded fields
	Bitboard allowedWhite = ~(pos.PieceBB(PAWN, WHITE) | pos.PieceBB(KING, WHITE));
	Bitboard allowedBlack = ~(pos.PieceBB(PAWN, BLACK) | pos.PieceBB(KING, BLACK));

	//Now calculate Mobility
	//Queens can move to all unattacked squares and if protected to all squares attacked by queens or kings
	Bitboard pieceBB = pos.PieceBB(QUEEN, WHITE);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBlack | (abbWhite & ~abbBRook);
		result += MOBILITY_BONUS_QUEEN[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(QUEEN, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWRook);
		result -= MOBILITY_BONUS_QUEEN[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	//Rooks can move to all unattacked squares and if protected to all squares attacked  attacked by rooks or less important pieces
	pieceBB = pos.PieceBB(ROOK, WHITE);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBlack | (abbWhite & ~abbBLeicht);
		result += MOBILITY_BONUS_ROOK[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(ROOK, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWLeicht);
		result -= MOBILITY_BONUS_ROOK[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	//Leichtfiguren
	pieceBB = pos.PieceBB(BISHOP, WHITE);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBlack | (abbWhite & ~abbBPawn);
		result += MOBILITY_BONUS_BISHOP[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(BISHOP, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWPawn);
		result -= MOBILITY_BONUS_BISHOP[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(KNIGHT, WHITE);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBlack | (abbWhite & ~abbBPawn);
		result += MOBILITY_BONUS_KNIGHT[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(KNIGHT, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWPawn);
		result -= MOBILITY_BONUS_KNIGHT[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	return result;
}

eval evaluateMobility2(const position& pos) {
	eval result;
	//Excluded fields (fields attacked by pawns or occupied by own king or pawns)
	for (int i = 0; i <= 1; ++i) {
		Color them = Color(i ^ 1);
		Color us = Color(i);
		int factor = 1 - 2 * i;
		Bitboard notExcluded = ~(pos.AttacksByPieceType(them, PAWN) | pos.PieceBB(KING, us) | pos.PieceBB(PAWN, us));
		//Knight mobility
		Bitboard knights = pos.PieceBB(KNIGHT, us);
		while (knights) {
			Bitboard targets = pos.GetAttacksFrom(lsb(knights)) & notExcluded;
			result += factor * MOBILITY_BONUS_KNIGHT[popcount(targets)];
			knights &= knights - 1;
		}
		Bitboard bishops = pos.PieceBB(BISHOP, us);
		while (bishops) {
			Bitboard targets = pos.GetAttacksFrom(lsb(bishops)) & notExcluded;
			result += factor * MOBILITY_BONUS_BISHOP[popcount(targets)];
			bishops &= bishops - 1;
		}
		Bitboard rooks = pos.PieceBB(ROOK, us);
		while (rooks) {
			Bitboard targets = pos.GetAttacksFrom(lsb(rooks)) & notExcluded;
			result += factor * MOBILITY_BONUS_ROOK[popcount(targets)];
			rooks &= rooks - 1;
		}
		Bitboard queens = pos.PieceBB(QUEEN, us);
		while (queens) {
			Bitboard targets = pos.GetAttacksFrom(lsb(queens)) & notExcluded;
			result += factor * MOBILITY_BONUS_QUEEN[popcount(targets)];
			queens &= queens - 1;
		}
	}

	return result;
}

Value evaluateFromScratch(const position& pos) {
	evaluation result;
	MaterialTableEntry * material = pos.GetMaterialTableEntry();
	material->Score = VALUE_ZERO;
	eval materialEvaluation;
	for (PieceType pt = QUEEN; pt <= PAWN; ++pt) {
		int diff = popcount(pos.PieceBB(pt, WHITE)) - popcount(pos.PieceBB(pt, BLACK));
		materialEvaluation.egScore += diff * PieceValuesEG[pt];
		materialEvaluation.mgScore += diff * PieceValuesMG[pt];
	}
	Phase_t phase = Phase(popcount(pos.PieceBB(QUEEN, WHITE)), popcount(pos.PieceBB(QUEEN, BLACK)),
		popcount(pos.PieceBB(ROOK, WHITE)), popcount(pos.PieceBB(ROOK, BLACK)),
		popcount(pos.PieceBB(BISHOP, WHITE)), popcount(pos.PieceBB(BISHOP, BLACK)),
		popcount(pos.PieceBB(KNIGHT, WHITE)), popcount(pos.PieceBB(KNIGHT, BLACK)));
	material->Phase = phase;
	result.Material = material->Score = materialEvaluation.getScore(phase);
	material->EvaluationFunction = &evaluateDefault;
	return result.GetScore(pos.GetMaterialTableEntry()->Phase, pos.GetSideToMove());
}

Value evaluatePawnEnding(const position& pos) {
	 //try to find unstoppable pawns
	Value unstoppable = VALUE_ZERO;
	if (pos.GetPawnEntry()->passedPawns) {
		Bitboard wpassed = pos.GetPawnEntry()->passedPawns & pos.PieceBB(PAWN, WHITE);
		while (wpassed) {
			Square passedPawnSquare = lsb(wpassed);
			Square convSquare = ConversionSquare<WHITE>(passedPawnSquare);
			int distToConv = std::min(7 - (passedPawnSquare >> 3), 5);
			if (distToConv < (ChebishevDistance(lsb(pos.PieceBB(KING, BLACK)), convSquare) - (pos.GetSideToMove() == BLACK))) {
				unstoppable += Value(PieceValuesEG[QUEEN] - ((distToConv+1 + (pos.GetSideToMove() == BLACK)) * PieceValuesEG[PAWN]));
			}
			wpassed &= wpassed - 1;
		}
		Bitboard bpassed = pos.GetPawnEntry()->passedPawns & pos.PieceBB(PAWN, BLACK);
		while (bpassed) {
			Square passedPawnSquare = lsb(bpassed);
			Square convSquare = ConversionSquare<BLACK>(passedPawnSquare);
			int distToConv = std::min((passedPawnSquare >> 3), 5);
			if (distToConv < (ChebishevDistance(lsb(pos.PieceBB(KING, WHITE)), convSquare) - (pos.GetSideToMove() == WHITE))) {
				unstoppable -= Value(PieceValuesEG[QUEEN] - ((distToConv + 1 + (pos.GetSideToMove() == WHITE)) * PieceValuesEG[PAWN]));
			}
			bpassed &= bpassed - 1;
		}
	}
	return (pos.GetMaterialScore() + pos.GetPawnEntry()->Score + unstoppable) * (1 - 2 * pos.GetSideToMove());
}



