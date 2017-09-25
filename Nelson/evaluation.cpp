#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "evaluation.h"
#include "position.h"
#include "settings.h"

#ifdef WPF
struct wpf_entry {
	std::string fen;
	Phase_t phase;
	Value score;
	Evaluation evaluation;
	};

std::vector<wpf_entry> wpf_data;

int wpf_count = 0;

void writeWPF() {
	std::ofstream of;
	of.open("wpf.txt", std::ios::out | std::ios_base::app);
	for (auto it = wpf_data.begin(); it != wpf_data.end(); ++it) {
		wpf_entry e = *it;
		Evaluation v = e.evaluation;
		of << e.fen << ";" << e.score << ";" << v.KingSafety.getScore(e.phase) << ";" << v.Material.getScore(e.phase)
			<< ";" << v.Mobility.getScore(e.phase) << ";" << v.PawnStructure.getScore(e.phase) << ";" << v.Pieces.getScore(e.phase)
			<< ";" << v.PsqEval.getScore(e.phase) << ";" << v.Threats.getScore(e.phase) << std::endl;
	}
	of.close();
}
#endif

Value evaluateDefault(const Position& pos) {
	Evaluation result;
	result.Material = pos.GetMaterialTableEntry()->Evaluation;
	result.Mobility = evaluateMobility(pos);
	result.KingSafety = evaluateKingSafety(pos);
	result.PawnStructure = pos.PawnStructureScore();
	result.Threats = evaluateThreats<WHITE>(pos) - evaluateThreats<BLACK>(pos);
	result.Pieces = evaluatePieces<WHITE>(pos) - evaluatePieces<BLACK>(pos);
	result.PsqEval = pos.GetPsqEval();
	//result.Space = evaluateSpace<WHITE>(pos) -evaluateSpace<BLACK>(pos);
#ifdef WPF
	Value r = result.GetScore(pos);
	if (std::abs((int)r) < 100) {
		if (wpf_count % 10000 == 0) {
			wpf_entry e;
			e.fen = pos.fen();
			e.phase = pos.GetMaterialTableEntry()->Phase;
			e.score = r;
			e.evaluation = result;
			wpf_data.push_back(e);
		}
		++wpf_count;
	}
	return r;
#else
	return result.GetScore(pos);
#endif
}

std::string printDefaultEvaluation(const Position& pos) {
	std::stringstream ss;
	Evaluation result;
	result.Material = pos.GetMaterialTableEntry()->Evaluation;
	result.Mobility = evaluateMobility(pos);
	result.KingSafety = evaluateKingSafety(pos);
	result.PawnStructure = pos.PawnStructureScore();
	result.Threats = evaluateThreats<WHITE>(pos) - evaluateThreats<BLACK>(pos);
	result.Pieces = evaluatePieces<WHITE>(pos) - evaluatePieces<BLACK>(pos);
	result.PsqEval = pos.GetPsqEval();
	ss << "       Material: " << result.Material.print() << std::endl;
	ss << "       Mobility: " << result.Mobility.print() << std::endl;
	ss << "    King Safety: " << result.KingSafety.print() << std::endl;
	ss << " Pawn Structure: " << result.PawnStructure.print() << std::endl;
	ss << "Threats (White): " << evaluateThreats<WHITE>(pos).print() << std::endl;
	ss << "Threats (Black): " << evaluateThreats<BLACK>(pos).print() << std::endl;
	ss << " Pieces (White): " << evaluatePieces<WHITE>(pos).print() << std::endl;
	ss << " Pieces (Black): " << evaluatePieces<BLACK>(pos).print() << std::endl;
	ss << "           PSQT: " << pos.GetPsqEval().print() << std::endl;
	return ss.str();
}

Value evaluateDraw(const Position& pos) {
	return pos.GetSideToMove() == settings::parameter.EngineSide ? -settings::parameter.Contempt : settings::parameter.Contempt;
}

Eval evaluateKingSafety(const Position& pos) {
	Eval result;
	//Areas around the king
	Bitboard kingRingWhite = pos.PieceBB(KING, WHITE) | KingAttacks[pos.KingSquare(WHITE)];
	Bitboard kingRingBlack = pos.PieceBB(KING, BLACK) | KingAttacks[pos.KingSquare(BLACK)];
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
		if ((attackCount = popcount(pos.GetAttacksFrom(lsb(pieceBB))&kingZoneBlack))) {
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
	//safe queen contact checks
	Bitboard safeContactChecks = pos.AttacksByPieceType(WHITE, QUEEN) & kingRingBlack
		& (pos.AttacksByPieceType(WHITE, KING) | pos.AttacksByPieceType(WHITE, PAWN) | pos.AttacksByPieceType(WHITE, KNIGHT) | pos.AttacksByPieceType(WHITE, ROOK) | pos.AttacksByPieceType(WHITE, BISHOP));
	if (safeContactChecks) attackUnits += settings::parameter.ATTACK_UNITS_SAFE_CONTACT_CHECK * popcount(safeContactChecks);
	if (attackerCount > 1) result.mgScore = settings::parameter.KING_SAFETY[std::min(attackUnits, 99)];
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
	safeContactChecks = pos.AttacksByPieceType(BLACK, QUEEN) & kingRingWhite 
		& (pos.AttacksByPieceType(BLACK, KING) | pos.AttacksByPieceType(BLACK, PAWN) | pos.AttacksByPieceType(BLACK, KNIGHT) | pos.AttacksByPieceType(BLACK, ROOK) | pos.AttacksByPieceType(BLACK, BISHOP));
	if (safeContactChecks) attackUnits += settings::parameter.ATTACK_UNITS_SAFE_CONTACT_CHECK * popcount(safeContactChecks);
	if (attackerCount > 1) result.mgScore -= settings::parameter.KING_SAFETY[std::min(attackUnits, 99)];
	Bitboard bbWhite = pos.PieceBB(PAWN, WHITE);
	Bitboard bbBlack = pos.PieceBB(PAWN, BLACK);
	//Pawn shelter/storm
	Eval pawnStorm;
	if (pos.PieceBB(KING, WHITE) & SaveSquaresForKing & HALF_OF_WHITE) { //Bonus only for castled king
		pawnStorm += settings::parameter.PAWN_SHELTER_2ND_RANK * popcount(bbWhite & kingRingWhite & ShelterPawns2ndRank);
		pawnStorm += settings::parameter.PAWN_SHELTER_3RD_RANK * popcount(bbWhite & kingZoneWhite & ShelterPawns3rdRank);
		pawnStorm += settings::parameter.PAWN_SHELTER_4TH_RANK * popcount(bbWhite & (kingZoneWhite << 8) & ShelterPawns4thRank);
		bool kingSide = (pos.KingSquare(WHITE) & 7) > 3;
		Bitboard pawnStormArea = kingSide ? bbKINGSIDE : bbQUEENSIDE;
		Bitboard stormPawns = pos.PieceBB(PAWN, BLACK) & pawnStormArea & (HALF_OF_WHITE | RANK5);
		while (stormPawns) {
			Square sq = lsb(stormPawns);
			stormPawns &= stormPawns - 1;
			Piece blocker = pos.GetPieceOnSquare(Square(sq - 8));
			if ((blocker == WKING || GetPieceType(blocker) == PAWN) && (pos.GetAttacksFrom(sq) & pos.ColorBB(WHITE)) == EMPTY) 
				continue;//blocked
			pawnStorm -= settings::parameter.PAWN_STORM[(sq >> 3) - 1];
		}
	}
	if (pos.PieceBB(KING, BLACK) & SaveSquaresForKing & HALF_OF_BLACK) {
		pawnStorm -= settings::parameter.PAWN_SHELTER_2ND_RANK * popcount(bbBlack & kingRingBlack & ShelterPawns2ndRank);
		pawnStorm -= settings::parameter.PAWN_SHELTER_3RD_RANK * popcount(bbBlack & kingZoneBlack & ShelterPawns3rdRank);
		pawnStorm -= settings::parameter.PAWN_SHELTER_4TH_RANK * popcount(bbBlack & (kingZoneBlack >> 8) & ShelterPawns4thRank);
		bool kingSide = (pos.KingSquare(BLACK) & 7) > 3;
		Bitboard pawnStormArea = kingSide ? bbKINGSIDE : bbQUEENSIDE;
		Bitboard stormPawns = pos.PieceBB(PAWN, WHITE) & pawnStormArea & (HALF_OF_BLACK | RANK4);
		while (stormPawns) {
			Square sq = lsb(stormPawns);
			stormPawns &= stormPawns - 1;
			Piece blocker = pos.GetPieceOnSquare(Square(sq + 8));
			if ((blocker == BKING || GetPieceType(blocker) == PAWN) && (pos.GetAttacksFrom(sq) & pos.ColorBB(BLACK)) == EMPTY) 
				continue; //blocked
			pawnStorm += settings::parameter.PAWN_STORM[6 - (sq >> 3)];
		}
	}
	result += pawnStorm;
	return result;
}

Eval evaluateMobility(const Position& pos) {
	Eval result;
	//Create attack bitboards
	Bitboard abbWPawn = pos.AttacksByPieceType(WHITE, PAWN);
	Bitboard abbBPawn = pos.AttacksByPieceType(BLACK, PAWN);
	//Leichtfiguren (N+B)
	Bitboard abbWMinor = abbWPawn | pos.AttacksByPieceType(WHITE, KNIGHT) | pos.AttacksByPieceType(WHITE, BISHOP);
	Bitboard abbBMinor = abbBPawn | pos.AttacksByPieceType(BLACK, KNIGHT) | pos.AttacksByPieceType(BLACK, BISHOP);
	//Rooks
	Bitboard abbWRook = abbWMinor | pos.AttacksByPieceType(WHITE, ROOK);
	Bitboard abbBRook = abbBMinor | pos.AttacksByPieceType(BLACK, ROOK);
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
		result += settings::parameter.MOBILITY_BONUS_QUEEN[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(QUEEN, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWRook);
		result -= settings::parameter.MOBILITY_BONUS_QUEEN[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	//Rooks can move to all unattacked squares and if protected to all squares attacked by rooks or less important pieces
	pieceBB = pos.PieceBB(ROOK, WHITE);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBlack | (abbWhite & ~abbBMinor);
		result += settings::parameter.MOBILITY_BONUS_ROOK[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(ROOK, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWMinor);
		result -= settings::parameter.MOBILITY_BONUS_ROOK[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	//Leichtfiguren
	pieceBB = pos.PieceBB(BISHOP, WHITE);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBlack | (abbWhite & ~abbBPawn);
		result += settings::parameter.MOBILITY_BONUS_BISHOP[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(BISHOP, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWPawn);
		result -= settings::parameter.MOBILITY_BONUS_BISHOP[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(KNIGHT, WHITE);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedWhite;
		targets &= ~abbBlack | (abbWhite & ~abbBPawn);
		result += settings::parameter.MOBILITY_BONUS_KNIGHT[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	pieceBB = pos.PieceBB(KNIGHT, BLACK);
	while (pieceBB) {
		Square square = lsb(pieceBB);
		Bitboard targets = pos.GetAttacksFrom(square) & allowedBlack;
		targets &= ~abbWhite | (abbBlack & ~abbWPawn);
		result -= settings::parameter.MOBILITY_BONUS_KNIGHT[popcount(targets)];
		pieceBB &= pieceBB - 1;
	}
	//Pawn mobility
	Bitboard pawnTargets = abbWPawn & pos.ColorBB(BLACK);
	pawnTargets |= (pos.PieceBB(PAWN, WHITE) << 8) & ~pos.OccupiedBB();
	result += Eval(10, 10) * popcount(pawnTargets);
	pawnTargets = abbBPawn & pos.ColorBB(WHITE);
	pawnTargets |= (pos.PieceBB(PAWN, BLACK) >> 8) & ~pos.OccupiedBB();
	result -= Eval(10, 10) * popcount(pawnTargets);
	return result;
}

Value evaluateFromScratch(const Position& pos) {
	Evaluation result;
	MaterialTableEntry * material = pos.GetMaterialTableEntry();
	for (PieceType pt = QUEEN; pt <= PAWN; ++pt) {
		int diff = popcount(pos.PieceBB(pt, WHITE)) - popcount(pos.PieceBB(pt, BLACK));
		material->Evaluation += diff * settings::parameter.PieceValues[pt];
	}
	Phase_t phase = Phase(popcount(pos.PieceBB(QUEEN, WHITE)), popcount(pos.PieceBB(QUEEN, BLACK)),
		popcount(pos.PieceBB(ROOK, WHITE)), popcount(pos.PieceBB(ROOK, BLACK)),
		popcount(pos.PieceBB(BISHOP, WHITE)), popcount(pos.PieceBB(BISHOP, BLACK)),
		popcount(pos.PieceBB(KNIGHT, WHITE)), popcount(pos.PieceBB(KNIGHT, BLACK)));
	material->Phase = phase;
	result.Material = material->Evaluation;
	material->EvaluationFunction = &evaluateDefault;
	return result.GetScore(pos);
}

int scaleEG(const Position & pos)
{
	if (pos.GetMaterialTableEntry()->NeedsScaling() && pos.oppositeColoredBishops()) {
		if (pos.PieceTypeBB(ROOK) == EMPTY) {
			if (popcount(pos.PieceTypeBB(PAWN)) <= 1) return 18; return 62;
		}
		else return 92;
	}
	return 128;
}

Value evaluatePawnEnding(const Position& pos) {
	//try to find unstoppable pawns
	Value ppEval = VALUE_ZERO;
	if (pos.GetPawnEntry()->passedPawns) {
		int minUnstoppable[2] = { 999, 999 };
		int distant[2] = { 0, 0 };
		int distcount[2] = { 0, 0 };
		Bitboard wpassed = pos.GetPawnEntry()->passedPawns & pos.PieceBB(PAWN, WHITE);
		while (wpassed) {			
			Square passedPawnSquare = lsb(wpassed);
			Square convSquare = ConversionSquare<WHITE>(passedPawnSquare);
			int distToConv = std::min(7 - (passedPawnSquare >> 3), 5);
			if (distToConv < (ChebishevDistance(pos.KingSquare(BLACK), convSquare) - (pos.GetSideToMove() == BLACK))) {
				minUnstoppable[0] = std::min(minUnstoppable[0], distToConv);
			}
			else {
				distcount[0]++;
				Square blockSquare = Square(passedPawnSquare + 8);
				int dtcSquare = (6 - distToConv) * (6 - distToConv);
				int distPoints = 2 * dtcSquare * ChebishevDistance(pos.KingSquare(BLACK), blockSquare) + dtcSquare * ChebishevDistance(pos.KingSquare(WHITE), blockSquare);
				distant[0] += distPoints;
			}
			wpassed &= wpassed - 1;
		}
		Bitboard bpassed = pos.GetPawnEntry()->passedPawns & pos.PieceBB(PAWN, BLACK);
		while (bpassed) {
			Square passedPawnSquare = lsb(bpassed);
			Square convSquare = ConversionSquare<BLACK>(passedPawnSquare);
			int distToConv = std::min((passedPawnSquare >> 3), 5);
			if (distToConv < (ChebishevDistance(pos.KingSquare(WHITE), convSquare) - (pos.GetSideToMove() == WHITE))) {
				minUnstoppable[1] = std::min(minUnstoppable[1], distToConv);
			}
			else {
				distcount[1]++;
				Square blockSquare = Square(passedPawnSquare - 8);
				int dtcSquare = (6 - distToConv) * (6 - distToConv);
				int distPoints = 2 * dtcSquare * ChebishevDistance(pos.KingSquare(WHITE), blockSquare) + dtcSquare * ChebishevDistance(pos.KingSquare(BLACK), blockSquare);
				distant[1] += distPoints;
			}
			bpassed &= bpassed - 1;
		}
		if (minUnstoppable[0] < (minUnstoppable[1] - (pos.GetSideToMove() == BLACK))) {
			ppEval += Value(settings::parameter.PieceValues[QUEEN].egScore - 100 * minUnstoppable[0]);
		}
		else if (minUnstoppable[1] < (minUnstoppable[0] - (pos.GetSideToMove() == WHITE))) {
			ppEval -= Value(settings::parameter.PieceValues[QUEEN].egScore - 100 * minUnstoppable[1]);
		}
		else {
			ppEval += Value(11 *(distant[0]*distcount[0] - distant[1]*distcount[1])/5);
		}
	}
	return Value((pos.GetMaterialScore() + pos.GetPawnEntry()->Score.egScore + pos.GetPsqEval().egScore + ppEval) * (1 - 2 * pos.GetSideToMove()));
}

Value evaluateKBPxKBPx(const Position& pos) {
	Evaluation result;
	result.Material = pos.GetMaterialTableEntry()->Evaluation;
	result.Mobility = evaluateMobility(pos);
	result.PawnStructure = pos.PawnStructureScore();
	//result.Space = evaluateSpace<WHITE>(pos) -evaluateSpace<BLACK>(pos);
	Bitboard darkSquareBishops = pos.PieceTypeBB(BISHOP) & DARKSQUARES;
	if (darkSquareBishops != 0 && (darkSquareBishops & (darkSquareBishops - 1)) == 0) {
		//oposite colored bishops
		result.Material.egScore -= result.Material.egScore > 0 ? settings::parameter.PieceValues[PAWN].egScore : -settings::parameter.PieceValues[PAWN].egScore;
	}
	return result.GetScore(pos);
}



