#pragma once

#include "types.h"
#include "board.h"
#include "position.h"
#include "bbEndings.h"


struct evaluation
{
public:
	Value Material = VALUE_ZERO;
	eval Mobility = EVAL_ZERO;
	eval Threats = EVAL_ZERO;
	eval KingSafety = EVAL_ZERO;
	eval Pieces = EVAL_ZERO;
	//eval Space = EVAL_ZERO;
	Value PawnStructure = VALUE_ZERO;

	inline Value GetScore(const Phase_t phase, const Color sideToMove) {
		return (Material + PawnStructure + (Mobility + KingSafety + Threats + Pieces).getScore(phase)) * (1 - 2 * sideToMove);
	}
};

Value evaluateDraw(const position& pos);
Value evaluateFromScratch(const position& pos);
template<Color WinningSide> Value evaluateKBPK(const position& pos);
template <Color WinningSide> Value easyMate(const position& pos);
template <Color WinningSide> Value evaluateKQKP(const position& pos);
template <Color StrongerSide> Value evaluateKRKP(const position& pos);
template <Color StrongerSide> Value evaluateKNKP(const position& pos);
template <Color StrongerSide> Value evaluateKBKP(const position& pos);
template <Color SideWithoutPawns> Value evaluateKNKPx(const position& pos);
template <Color SideWithoutPawns> Value evaluateKBKPx(const position& pos);
template <Color StrongerSide> Value evaluateKQPKQ(const position& pos);
template <Color StrongerSide> Value evaluateKQKRP(const position& pos);
eval evaluateMobility(const position& pos);
eval evaluateMobility2(const position& pos);
eval evaluateKingSafety(const position& pos);
Value evaluatePawnEnding(const position& pos);
template <Color COL> eval evaluateThreats(const position& pos);

const int PSQ_GoForMate[64] = {
	100, 90, 80, 70, 70, 80, 90, 100,
	90, 70, 60, 50, 50, 60, 70, 90,
	80, 60, 40, 30, 30, 40, 60, 80,
	70, 50, 30, 20, 20, 30, 50, 70,
	70, 50, 30, 20, 20, 30, 50, 70,
	80, 60, 40, 30, 30, 40, 60, 80,
	90, 70, 60, 50, 50, 60, 70, 90,
	100, 90, 80, 70, 70, 80, 90, 100
};

const int BonusDistance[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

template <Color WinningSide> Value easyMate(const position& pos) {
	Value result = pos.GetMaterialScore();
	if (WinningSide == WHITE) {
		result += VALUE_KNOWN_WIN;
		result += Value(PSQ_GoForMate[lsb(pos.PieceBB(KING, BLACK))]);
		result += Value(BonusDistance[ChebishevDistance(lsb(pos.PieceBB(KING, BLACK)), lsb(pos.PieceBB(KING, WHITE)))]);
	}
	else {
		result -= VALUE_KNOWN_WIN;
		result -= Value(PSQ_GoForMate[lsb(pos.PieceBB(KING, WHITE))]);
		result -= Value(BonusDistance[ChebishevDistance(lsb(pos.PieceBB(KING, BLACK)), lsb(pos.PieceBB(KING, WHITE)))]);
	}

	return result * (1 - 2 * pos.GetSideToMove());
}

//If Pawn isn't on 7th Rank it's always a win for stronger side
//on 7th Rank it is a if pawn is no rook or bishop pawn
//for rook or bishop pawns there is a bitbase in bbEndings
template<Color WinningSide> Value evaluateKQKP(const position& pos) {
	Value result;
	//First check if it's a "clear" win
	Bitboard pawnBB = pos.PieceBB(PAWN, Color(WinningSide ^ 1));
	Square pawnSq = lsb(pawnBB);
	Square winningKingSquare = lsb(pos.PieceBB(KING, WinningSide));
	int relativeRank = pawnSq >> 3;
	if (WinningSide == WHITE) relativeRank = 7 - relativeRank;
	if ((pawnBB & (A_FILE | C_FILE | F_FILE | H_FILE)) == 0)
		result = VALUE_KNOWN_WIN + PieceValuesEG[QUEEN] - PieceValuesEG[PAWN] - ChebishevDistance(winningKingSquare, pawnSq) + Value(relativeRank);
	else {

		if (relativeRank != Rank7)
			result = VALUE_KNOWN_WIN + PieceValuesEG[QUEEN] - PieceValuesEG[PAWN] - ChebishevDistance(winningKingSquare, pawnSq) + Value(relativeRank);
		else {
			//Now we need to probe the bitbase => therefore normalize position (White is winning and pawn is on left hals of board)
			Square wKingSquare, bKingSquare, wQueenSquare;
			Color stm;
			if (WinningSide == WHITE) {
				wKingSquare = winningKingSquare;
				bKingSquare = lsb(pos.PieceBB(KING, BLACK));
				wQueenSquare = lsb(pos.PieceBB(QUEEN, WHITE));
				stm = pos.GetSideToMove();
			}
			else {
				//flip colors
				pawnSq = Square(pawnSq ^ 56);
				wKingSquare = Square(winningKingSquare ^ 56);
				bKingSquare = Square(lsb(pos.PieceBB(KING, WHITE)) ^ 56);
				wQueenSquare = Square(lsb(pos.PieceBB(QUEEN, BLACK)) ^ 56);
				stm = Color(pos.GetSideToMove() ^ 1);
			}
			if ((pawnSq & 7) > FileD) {
				pawnSq = Square(pawnSq ^ 7);
				wKingSquare = Square(wKingSquare ^ 7);
				bKingSquare = Square(bKingSquare ^ 7);
				wQueenSquare = Square(wQueenSquare ^ 7);
			}
			bool isWin = pawnSq == A2 ? kqkp::probeA2(wKingSquare, wQueenSquare, bKingSquare, stm) : kqkp::probeC2(wKingSquare, wQueenSquare, bKingSquare, stm);
			if (isWin) result = VALUE_KNOWN_WIN + PieceValuesEG[QUEEN] - PieceValuesEG[PAWN] - ChebishevDistance(winningKingSquare, pawnSq) + Value(relativeRank);
			else result = VALUE_DRAW;
		}
	}

	return WinningSide == pos.GetSideToMove() ? result : -result;
}

template<Color WinningSide> Value evaluateKBPK(const position& pos) {
	//Check for draw
	Value result;
	Bitboard bbPawn = pos.PieceBB(PAWN, WinningSide);
	Square pawnSquare = lsb(bbPawn);
	if ((bbPawn & (A_FILE | H_FILE)) != 0) { //Pawn is on rook file
		Square conversionSquare = WinningSide == WHITE ? Square(56 + (pawnSquare & 7)) : Square(pawnSquare & 7);
		if (oppositeColors(conversionSquare, lsb(pos.PieceBB(BISHOP, WinningSide)))) { //Bishop doesn't match conversion's squares color
			//Now check if weak king control the conversion square
			Bitboard conversionSquareControl = KingAttacks[conversionSquare] | (ToBitboard(conversionSquare));
			if (pos.PieceBB(KING, Color(WinningSide ^ 1)) & conversionSquareControl) return VALUE_DRAW;
			if ((pos.PieceBB(KING, WinningSide) & conversionSquareControl) == 0) { //Strong king doesn't control conversion square
				Square weakKing = lsb(pos.PieceBB(KING, Color(WinningSide ^ 1)));
				Square strongKing = lsb(pos.PieceBB(KING, WinningSide));
				Bitboard bbRank2 = WinningSide == WHITE ? RANK2 : RANK7;
				if ((ChebishevDistance(weakKing, conversionSquare) + (pos.GetSideToMove() != WinningSide) <= ChebishevDistance(pawnSquare, conversionSquare) - ((bbPawn & bbRank2) != 0)) //King is in Pawnsquare
					&& (ChebishevDistance(weakKing, conversionSquare) - (pos.GetSideToMove() != WinningSide) <= ChebishevDistance(strongKing, conversionSquare))) {
					//Weak King is in pawn square and at least as near to the conversion square as the Strong King => search must solve it
					if (WinningSide == WHITE) {
						//Todo: heck if factors 10 and 5can't be replaced with 2 and 1
						result = pos.GetMaterialScore()
							+ Value(10 * (ChebishevDistance(weakKing, conversionSquare) - ChebishevDistance(strongKing, conversionSquare)))
							+ Value(5 * (pawnSquare >> 3));
					}
					else {
						//Todo: heck if factors 10 and 5can't be replaced with 2 and 1
						result = pos.GetMaterialScore()
							- Value(10 * (ChebishevDistance(weakKing, conversionSquare) - ChebishevDistance(strongKing, conversionSquare)))
							- Value(5 * (7 - (pawnSquare >> 3)));
					}
					return result * (1 - 2 * pos.GetSideToMove());
				}
			}
		}
	}
	result = WinningSide == WHITE ? VALUE_KNOWN_WIN + pos.GetMaterialScore() + Value(pawnSquare >> 3)
		: pos.GetMaterialScore() - VALUE_KNOWN_WIN - Value(7 - (pawnSquare >> 3));
	return result * (1 - 2 * pos.GetSideToMove());
}

const int PSQ_MateInCorner[64] = {
	200, 190, 180, 170, 160, 150, 140, 130,
	190, 180, 170, 160, 150, 140, 130, 140,
	180, 170, 155, 140, 140, 125, 140, 150,
	170, 160, 140, 120, 110, 140, 150, 160,
	160, 150, 140, 110, 120, 140, 160, 170,
	150, 140, 125, 140, 140, 155, 170, 180,
	140, 130, 140, 150, 160, 170, 180, 190,
	130, 140, 150, 160, 170, 180, 190, 200
};

template <Color WinningSide> Value evaluateKNBK(const position& pos) {
	Square winnerKingSquare = lsb(pos.PieceBB(KING, WinningSide));
	Square loosingKingSquare = lsb(pos.PieceBB(KING, Color(WinningSide ^ 1)));

	if ((DARKSQUARES & pos.PieceBB(BISHOP, WinningSide)) == 0) {
		//transpose KingSquares
		winnerKingSquare = Square(winnerKingSquare ^ 56);
		loosingKingSquare = Square(loosingKingSquare ^ 56);
	}

	Value result = VALUE_KNOWN_WIN + Value(BonusDistance[ChebishevDistance(winnerKingSquare, loosingKingSquare)]) + Value(PSQ_MateInCorner[loosingKingSquare]);
	return WinningSide == pos.GetSideToMove() ? result : -result;
}

//KQP vs KQ: Try to centralize Queens and weaker side should try to get his king to the "safe" areas
template <Color StrongerSide> Value evaluateKQPKQ(const position& pos) {
	Value result = pos.GetMaterialScore() + pos.PawnStructureScore() + evaluateMobility(pos).egScore
		+ evaluateThreats<WHITE>(pos).egScore - evaluateThreats<BLACK>(pos).egScore;
	//result.Pieces = evaluatePieces<WHITE>(pos) -evaluatePieces<BLACK>(pos);
	if (StrongerSide == BLACK) result = -result;
	Square pawnSquare = lsb(pos.PieceTypeBB(PAWN));
	File pawnFile = File(pawnSquare & 7);
	Square conversionSquare = ConversionSquare<StrongerSide>(pawnSquare);
	Bitboard safeArea = KingAttacks[conversionSquare] & ToBitboard(conversionSquare);
	if (pawnFile <= FileB) {
		safeArea |= StrongerSide == BLACK ? KingAttacks[H8] | ToBitboard(H8) : KingAttacks[H1] | ToBitboard(H1);
	}
	else if (pawnFile >= FileG) {
		safeArea |= StrongerSide == BLACK ? KingAttacks[A8] | ToBitboard(A8) : KingAttacks[A1] | ToBitboard(A1);
	}
	if ((pos.PieceBB(KING, Color(StrongerSide ^ 1))&safeArea) != 0) result -= 30;
	if ((pos.PieceBB(QUEEN, StrongerSide) & CENTER) != 0) result += 10;
	//if ((pos.PieceBB(QUEEN, StrongerSide) & EXTENDED_CENTER) != 0) result += 10;
	//if ((pos.PieceBB(QUEEN, ~StrongerSide) & CENTER) != 0) result -= 5;
	//if ((pos.PieceBB(QUEEN, ~StrongerSide) & EXTENDED_CENTER) != 0) result -= 5;
	return StrongerSide == pos.GetSideToMove() ? result : -result;
	//Square pawnSquare = lsb(pos.PieceTypeBB(PAWN));
	//File pawnFile = File(pawnSquare & 7);
	//Square conversionSquare = ConversionSquare<StrongerSide>(pawnSquare);
	//Bitboard safeArea = KingAttacks[conversionSquare] & ToBitboard(conversionSquare);
	//if (pawnFile <= FileB) {
	//	safeArea |= StrongerSide == BLACK ? KingAttacks[H8] | ToBitboard(H8) : KingAttacks[H1] | ToBitboard(H1);
	//}
	//else if (pawnFile >= FileG) {
	//	safeArea |= StrongerSide == BLACK ? KingAttacks[A8] | ToBitboard(A8) : KingAttacks[A1] | ToBitboard(A1);
	//}
	//Value result = pos.PawnStructureScore() + pos.GetMaterialTableEntry()->Score;
	//if (StrongerSide == BLACK) result = -result;
	//if ((pos.PieceBB(QUEEN, StrongerSide) & CENTER) != 0) result += 1;
	//if ((pos.PieceBB(QUEEN, StrongerSide) & EXTENDED_CENTER) != 0) result += 1;
	//if ((pos.PieceBB(QUEEN, ~StrongerSide) & CENTER) != 0) result -= 1;
	//if ((pos.PieceBB(QUEEN, ~StrongerSide) & EXTENDED_CENTER) != 0) result -= 1;
	//if ((pos.PieceBB(KING, ~StrongerSide)&safeArea) != 0) result -= 4;
	////Strong King's best place is in front and near of his pawn
	//Bitboard kingArea = StrongerSide == WHITE ? KingAttacks[pawnSquare + 8] : KingAttacks[pawnSquare - 8];
	//if ((pos.PieceBB(KING, StrongerSide) & kingArea) != 0) result += 4;
	//else if ((kingArea & KingAttacks[lsb(pos.PieceBB(KING, StrongerSide))]) != 0) result += 2;
	//return StrongerSide == pos.GetSideToMove() ? result - 4 * pos.IsCheck() : -result;
}

//Stronger Side has no chance to win - this endgame is totally drawish, however if anybody can win, 
//it's the weaker side
template <Color StrongerSide> Value evaluateKNKP(const position& pos) {
	Square pawnSquare = lsb(pos.PieceBB(PAWN, Color(StrongerSide ^ 1)));
	Square conversionSquare = StrongerSide == WHITE ? Square(pawnSquare & 7) : Square((pawnSquare & 7) + 56);
	int dtc = ChebishevDistance(pawnSquare, conversionSquare);
	//max dtc is 6 =>
	Value result = Value(dtc * dtc - 37); //From stronger side's POV
	return StrongerSide == pos.GetSideToMove() ? result : -result;
}

//Stronger Side has no chance to win - this endgame is totally drawish, however if anybody can win, 
//it's the weaker side
template <Color StrongerSide> Value evaluateKBKP(const position& pos) {
	Value result;
	Square pawnSquare = lsb(pos.PieceBB(PAWN, Color(StrongerSide ^ 1)));
	Square bishopSquare = lsb(pos.PieceBB(BISHOP, StrongerSide));
	Square conversionSquare = StrongerSide == WHITE ? Square(pawnSquare & 7) : Square((pawnSquare & 7) + 56);
	Bitboard pawnPath = InBetweenFields[pawnSquare][conversionSquare] | ToBitboard(conversionSquare);
	if ((pawnPath & pos.PieceBB(KING, StrongerSide)) != 0 || (pawnPath & pos.GetAttacksFrom(bishopSquare)) != 0) {
		result = VALUE_DRAW - 1;
	}
	else {
		int dtc = ChebishevDistance(pawnSquare, conversionSquare);
		result = Value(dtc * dtc - 38); //From stronger side's POV
	}
	return StrongerSide == pos.GetSideToMove() ? result : -result;
}

//Knight vs 2 or more pawns => Knight can nearly never win
//might convert to KNKQPx-1 Which will be evaluated in the default way 
//=> Therefore evaluation should never exceed default evaluation for this ending
//Evaluation should be positive for SideWithoutPawns (but drawish for 2 pawns)
template <Color SideWithoutPawns> Value evaluateKNKPx(const position& pos) {
	Bitboard pawns = pos.PieceTypeBB(PAWN);
	int pawnCount = popcount(pawns);
	//Material with scaled down knight value
	//2 pawns => Knight counts half => total Material value = 0.5 Bishop - 2 Pawns = -40
	//3 pawns => Knight counts 2/3  => total Material Value = -80
	//4 pawns => Knight counts 3/4  => total Material Value = -160
	Value result = Value(int(int(PieceValuesEG[KNIGHT])*(1.0 - 1.0 / pawnCount))) - pawnCount * PieceValuesEG[PAWN]; //from Side without Pawns POV
	//King and Knight should try to control or block the pawns frontfill
	Bitboard frontspan = SideWithoutPawns == WHITE ? FrontFillSouth(pawns) : FrontFillNorth(pawns);
	//result += Value(5 * popcount(frontspan & (pos.AttacksByPieceType(SideWithoutPawns, KNIGHT) | pos.PieceTypeBB(KNIGHT)))
	//	          + 5 * popcount(frontspan & (pos.AttacksByPieceType(SideWithoutPawns, KING) | pos.PieceBB(KING, SideWithoutPawns)))
	//			  - 10 * popcount((frontspan | pos.PieceTypeBB(PAWN)) & pos.AttacksByPieceType(~SideWithoutPawns, KING))
	//	);
	result += Value(popcount(frontspan & (pos.AttacksByPieceType(SideWithoutPawns, KNIGHT) | pos.PieceTypeBB(KNIGHT) | pos.AttacksByPieceType(SideWithoutPawns, KING) | pos.PieceBB(KING, SideWithoutPawns))));
	result -= Value(2 * popcount(pos.PieceTypeBB(PAWN) & pos.AttacksByPieceType(Color(SideWithoutPawns ^ 1), KING)));
	if (SideWithoutPawns == BLACK) result = -result; //now White's POV
	result += pos.PawnStructureScore();
	return (1 - 2 * pos.GetSideToMove()) * result;
}

//Bishop vs 2 or more pawns => Bishop can nearly never win
//therefore evaluation should be always positive for Side with Pawns
//To keep evaluation continuity evluation should be less than standard evaluation
//with an additional Knight => we try to use default evaluation but reduce material value
//of Bishop
template <Color SideWithoutPawns> Value evaluateKBKPx(const position& pos) {
	Bitboard pawns = pos.PieceTypeBB(PAWN);
	//Bitboard frontspan = SideWithoutPawns == WHITE ? FrontFillSouth(pawns) : FrontFillNorth(pawns);
	int pawnCount = popcount(pawns);
	//Material with scaled down bishop value
	//2 pawns => Bishop counts half => total Material value = 0.5 Bishop - 2 Pawns = -40
	//3 pawns => Bishop counts 2/3  => total Material Value = -80
	//4 pawns => Bishop counts 3/4  => total Material Value = -160
	Value result = Value(int(int(PieceValuesEG[BISHOP])*(1.0 - 1.0 / pawnCount))) - pawnCount * PieceValuesEG[PAWN]; //from Side without Pawns POV
	Bitboard frontspan = SideWithoutPawns == WHITE ? FrontFillSouth(pawns) : FrontFillNorth(pawns);
	result += Value(popcount(frontspan & (pos.AttacksByPieceType(SideWithoutPawns, BISHOP) | pos.PieceTypeBB(BISHOP) | pos.AttacksByPieceType(SideWithoutPawns, KING) | pos.PieceBB(KING, SideWithoutPawns))));
	result -= Value(2 * popcount(pos.PieceTypeBB(PAWN) & pos.AttacksByPieceType(Color(SideWithoutPawns ^ 1), KING)));
	if (SideWithoutPawns == BLACK) result = -result; //now White's POV
	//result += evaluateMobility(pos).egScore + pos.PawnStructureScore() + evaluateThreats<WHITE>(pos).egScore - evaluateThreats<BLACK>(pos).egScore;
	result += pos.PawnStructureScore();
	return (1 - 2 * pos.GetSideToMove()) * result;
}

template <Color StrongerSide> Value evaluateKRKP(const position& pos) {
	Value result;
	//if the stronger King is in the front of the pawn it's a win
	Square pawnSquare = lsb(pos.PieceTypeBB(PAWN));
	Square strongerKingSquare = lsb(pos.PieceBB(KING, StrongerSide));
	int dtc = StrongerSide == WHITE ? pawnSquare >> 3 : 7 - (pawnSquare >> 3);
	Bitboard pfront = StrongerSide == WHITE ? FrontFillSouth(pos.PieceTypeBB(PAWN)) : FrontFillNorth(pos.PieceTypeBB(PAWN));
	if (pfront & pos.PieceBB(KING, StrongerSide)) {
		result = VALUE_KNOWN_WIN + PieceValuesEG[ROOK] - PieceValuesEG[PAWN] + Value(dtc - ChebishevDistance(strongerKingSquare, pawnSquare));
		return StrongerSide == pos.GetSideToMove() ? result : -result;
	}
	Color WeakerSide = Color(StrongerSide ^ 1);
	int pawnRank = pawnSquare >> 3;
	if (StrongerSide == WHITE && pawnRank == 6) pawnSquare = Square(pawnSquare - 8);
	else if (StrongerSide == BLACK && pawnRank == 1) pawnSquare = Square(pawnSquare + 8);
	//if the weaker king is far away from rook and pawn it's a win
	Square weakKingSquare = lsb(pos.PieceBB(KING, WeakerSide));
	Square rookSquare = lsb(pos.PieceTypeBB(ROOK));
	if (ChebishevDistance(weakKingSquare, pawnSquare) > (3 + (pos.GetSideToMove() == WeakerSide))
		&& ChebishevDistance(weakKingSquare, rookSquare) >= 3) {
		result = VALUE_KNOWN_WIN + PieceValuesEG[ROOK] - PieceValuesEG[PAWN] + Value(dtc - ChebishevDistance(strongerKingSquare, pawnSquare));
		return StrongerSide == pos.GetSideToMove() ? result : -result;
	}
	// If the pawn is far advanced and supported by the defending king,
	// the position is drawish
	Square conversionSquare = StrongerSide == WHITE ? Square(pawnSquare & 7) : Square((pawnSquare & 7) + 56);
	if (ChebishevDistance(pawnSquare, weakKingSquare) == 1
		&& ChebishevDistance(conversionSquare, weakKingSquare) <= 2
		&& ChebishevDistance(conversionSquare, strongerKingSquare) >= (3 + (pos.GetSideToMove() == StrongerSide))) {
		result = Value(80) - 8 * ChebishevDistance(strongerKingSquare, pawnSquare);
		return StrongerSide == pos.GetSideToMove() ? result : -result;
	}
	else {
		int pawnStep = StrongerSide == WHITE ? -8 : 8;
		result = Value(200) - 8 * (ChebishevDistance(strongerKingSquare, Square(pawnSquare + pawnStep))
			- ChebishevDistance(weakKingSquare, Square(pawnSquare + pawnStep))
			- dtc);
		return StrongerSide == pos.GetSideToMove() ? result : -result;
	}
}

template <Color StrongerSide> Value evaluateKQKRP(const position& pos) {
	//Check for fortress
	Color weak = StrongerSide == WHITE ? BLACK : WHITE;
	Bitboard pawn = StrongerSide == WHITE ? 0x7e424242424200 : 0x42424242427e00;
	Bitboard rook = pos.PieceTypeBB(ROOK);
	if ((pos.PieceTypeBB(PAWN) & pawn) //Pawn is on fortress location
		&& (rook & 0x3c24243c0000) //Rook is on "6th Rank"
		&& (rook & pos.AttacksByPieceType(weak, PAWN)) //rook is protected by pawn (and by this cutting off the other king
		&& (pos.PieceTypeBB(PAWN) & pos.AttacksByPieceType(weak, KING))) //king is protecting own pawn
	{
		//check if strong king is on other side of rook as weak king
		if ((rook & C_FILE) && (pos.PieceBB(KING, weak) & (A_FILE | B_FILE)) && (pos.PieceBB(KING, StrongerSide))& (bbKINGSIDE | E_FILE | D_FILE)) return VALUE_DRAW;
		if ((rook & F_FILE) && (pos.PieceBB(KING, weak) & (G_FILE | H_FILE)) && (pos.PieceBB(KING, StrongerSide))& (bbQUEENSIDE | E_FILE | D_FILE)) return VALUE_DRAW;
		if ((rook & RANK3) && (pos.PieceBB(KING, weak) & (RANK1 | RANK2)) && (pos.PieceBB(KING, StrongerSide))& (HALF_OF_BLACK | RANK4)) return VALUE_DRAW;
		if ((rook & RANK6) && (pos.PieceBB(KING, weak) & (RANK7 | RANK8)) && (pos.PieceBB(KING, StrongerSide))& (HALF_OF_WHITE | RANK5)) return VALUE_DRAW;
		//Then we have a fortress
		return VALUE_DRAW;
	}
    return evaluateDefault(pos);
}

template <Color COL> eval evaluateThreats(const position& pos) {
	enum { Defended, Weak };
	enum { Minor, Major };
	Bitboard b, weak, defended;
	eval result = EVAL_ZERO;
	Color OTHER = Color(COL ^ 1);
	// Non-pawn enemies defended by a pawn
	defended = (pos.ColorBB(OTHER) ^ pos.PieceBB(PAWN, OTHER)) & pos.AttacksByPieceType(OTHER, PAWN);
	// Add a bonus according to the kind of attacking pieces
	if (defended)
	{
		b = defended & (pos.AttacksByPieceType(COL, KNIGHT) | pos.AttacksByPieceType(COL, BISHOP));
		while (b) {
			result += Threat[Defended][Minor][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
		b = defended & pos.AttacksByPieceType(COL, ROOK);
		while (b) {
			result += Threat[Defended][Major][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
	}
	// Enemies not defended by a pawn and under our attack
	weak = pos.ColorBB(OTHER)
		& ~pos.AttacksByPieceType(OTHER, PAWN)
		& pos.AttacksByColor(COL);
	// Add a bonus according to the kind of attacking pieces
	if (weak)
	{
		b = weak & (pos.AttacksByPieceType(COL, KNIGHT) | pos.AttacksByPieceType(COL, BISHOP));
		while (b) {
			result += Threat[Weak][Minor][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
		b = weak & (pos.AttacksByPieceType(COL, ROOK) | pos.AttacksByPieceType(COL, QUEEN));
		while (b) {
			result += Threat[Weak][Major][GetPieceType(pos.GetPieceOnSquare(lsb(b)))];
			b &= b - 1;
		}
		b = weak & ~pos.AttacksByColor(OTHER);
		if (b) result += Hanging * popcount(b);
		b = weak & pos.AttacksByPieceType(COL, KING);
		if (b) result += (b & (b - 1)) ? KingOnMany : KingOnOne;
	}
	return result;
}

template <Color COL> eval evaluateSpace(const position& pos) {
	Bitboard space;
	Color OTHER = Color(COL ^ 1);
	if (COL == WHITE) space = 0x3c3c3c00; else space = 0x3c3c3c00000000;
	Bitboard safe = space & ~pos.PieceBB(PAWN, COL) & ~pos.AttacksByPieceType(OTHER, PAWN)
		& (pos.AttacksByColor(COL) | ~pos.AttacksByColor(OTHER));
	// Find all squares which are at most three squares behind some friendly pawn
	Bitboard behind = pos.PieceBB(PAWN, COL);
	behind |= (COL == WHITE ? behind >> 8 : behind << 8);
	behind |= (COL == WHITE ? behind >> 16 : behind << 16);
	int bonus = popcount(safe) + popcount(behind & safe);
	int weight = popcount(pos.PieceTypeBB(KNIGHT) | pos.PieceTypeBB(BISHOP));
	eval result((bonus * weight * weight) >> 2, 0);
	return result;
}

template <Color COL> eval evaluatePieces(const position& pos) {
	Color OTHER = Color(COL ^ 1);
	//Knights
	Bitboard outpostArea = COL == WHITE ? 0x3c3c00000000 : 0x3c3c0000;
	Bitboard outposts = pos.PieceBB(KNIGHT, COL) & outpostArea & pos.AttacksByPieceType(COL, PAWN);
	Value bonusKnightOutpost = BONUS_KNIGHT_OUTPOST * popcount(outposts);
	//Value bonus = VALUE_ZERO;
	//Value ptBonus = BONUS_BISHOP_OUTPOST;
	//for (PieceType pt = BISHOP; pt <= KNIGHT; ++pt) {
	//	Bitboard outposts = pos.PieceBB(pt, COL) & outpostArea & pos.AttacksByPieceType(COL, PAWN);
	//	while (outposts) {
	//		bonus += ptBonus;
	//		Bitboard outpostBB = isolateLSB(outposts);
	//		Square outpostSquare = lsb(outposts);
	//		Bitboard defendingPawnSquares = PawnAttacks[COL][outpostSquare];
	//		if (COL == WHITE) defendingPawnSquares |= defendingPawnSquares << 8; else defendingPawnSquares |= defendingPawnSquares >> 8;
	//		if (!(pos.PieceBB(PAWN, OTHER) & defendingPawnSquares)) bonus += ptBonus; //Outpost can't be attacked by a pawn
	//		if (!pos.PieceBB(KNIGHT, OTHER) && !(squaresOfSameColor(outpostSquare) & pos.PieceBB(BISHOP, OTHER))) bonus += ptBonus; //Outpost can't be exchanged
	//		outposts &= outposts - 1;
	//	}
	//	ptBonus = BONUS_KNIGHT_OUTPOST;
	//}
	//Rooks
	Bitboard seventhRank = COL == WHITE ? RANK7 : RANK2;
	Bitboard rooks = pos.PieceBB(ROOK, COL);
	eval bonusRook = popcount(rooks & seventhRank) * ROOK_ON_7TH;
	while (rooks) {
		Square rookSquare = lsb(rooks);
		Bitboard rookFile = FILES[rookSquare & 7];
		if ((pos.PieceBB(PAWN, COL) & rookFile) == 0) bonusRook += ROOK_ON_SEMIOPENFILE;
		//This seems to be wrong, as it also gives bonus to rooks behind own pawns
		//but fixing it didn't improve performance
		if ((pos.PieceBB(PAWN, OTHER) & rookFile) == 0) bonusRook += ROOK_ON_OPENFILE;
		rooks &= rooks - 1;
	}
	return bonusRook + eval(bonusKnightOutpost, 0);
}
