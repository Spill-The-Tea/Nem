#include <sstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <ctype.h>
#include <regex>
#include "position.h"
#include "material.h"
#include "settings.h"
#include "hashtables.h"

static const std::string PieceToChar("QqRrBbNnPpKk ");

position::position()
{
	setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

position::position(std::string fen)
{
	setFromFEN(fen);
}

position::position(position &pos) {
	memcpy(this, &pos, offsetof(position, previous));
	previous = &pos;
}

position::~position()
{
}

void position::copy(const position &pos) {
	memcpy(this->attacks, pos.attacks, 64 * sizeof(Bitboard));
	memcpy(this->attacksByPt, pos.attacksByPt, 12 * sizeof(Bitboard));
	this->attackedByUs = pos.attackedByUs;
	this->attackedByThem = pos.attackedByThem;
	this->lastAppliedMove = pos.lastAppliedMove;
}

int position::AppliedMovesBeforeRoot = 0;

bool position::ApplyMove(Move move) {
	pliesFromRoot++;
	Square fromSquare = from(move);
	Square toSquare = to(move);
	Piece moving = Board[fromSquare];
	capturedInLastMove = Board[toSquare];
	remove(fromSquare);
	MoveType moveType = type(move);
	Piece convertedTo;
	switch (moveType) {
	case NORMAL:
		set<false>(moving, toSquare);
		//update epField
		if ((GetPieceType(moving) == PAWN)
			&& (abs(int(toSquare) - int(fromSquare)) == 16)
			&& (GetEPAttackersForToField(toSquare) & PieceBB(PAWN, Color(SideToMove ^ 1))))
		{
			SetEPSquare(Square(toSquare - PawnStep()));
		}
		else
			SetEPSquare(OUTSIDE);
		if (GetPieceType(moving) == PAWN || capturedInLastMove != BLANK) {
			DrawPlyCount = 0;
			if (GetPieceType(moving) == PAWN) {
				PawnKey ^= (ZobristKeys[moving][fromSquare]);
				PawnKey ^= (ZobristKeys[moving][toSquare]);
			}
			if (GetPieceType(capturedInLastMove) == PAWN) {
				PawnKey ^= ZobristKeys[capturedInLastMove][toSquare];
			}
		}
		else
			++DrawPlyCount;
		//update castlings
		if (CastlingOptions != 0) updateCastleFlags(fromSquare, toSquare);
		//Adjust material key
		if (capturedInLastMove != BLANK) {
			if (MaterialKey == MATERIAL_KEY_UNUSUAL) {
				if (checkMaterialIsUnusual()) material->Score = calculateMaterialScore(*this);
				else MaterialKey = calculateMaterialKey();
			}
			else MaterialKey -= materialKeyFactors[capturedInLastMove];
			material = &MaterialTable[MaterialKey];
		}
		break;
	case ENPASSANT:
		set<true>(moving, toSquare);
		remove(Square(toSquare - PawnStep()));
		capturedInLastMove = Piece(BPAWN - SideToMove);
		if (MaterialKey == MATERIAL_KEY_UNUSUAL) {
			material->Score = calculateMaterialScore(*this);
		}
		else {
			MaterialKey -= materialKeyFactors[BPAWN - SideToMove];
			material = &MaterialTable[MaterialKey];
		}
		SetEPSquare(OUTSIDE);
		DrawPlyCount = 0;
		PawnKey ^= ZobristKeys[moving][fromSquare];
		PawnKey ^= ZobristKeys[moving][toSquare];
		PawnKey ^= ZobristKeys[BPAWN - SideToMove][toSquare - PawnStep()];
		break;
	case PROMOTION:
		convertedTo = GetPiece(promotionType(move), SideToMove);
		set<false>(convertedTo, toSquare);
		if (CastlingOptions != 0) updateCastleFlags(fromSquare, toSquare);
		SetEPSquare(OUTSIDE);
		DrawPlyCount = 0;
		PawnKey ^= ZobristKeys[moving][fromSquare];
		//Adjust Material Key
		if (checkMaterialIsUnusual()) {
			MaterialKey = MATERIAL_KEY_UNUSUAL;
			material = &MaterialTable[MaterialKey];
			material->Score = calculateMaterialScore(*this);
		}
		else {
			if (MaterialKey == MATERIAL_KEY_UNUSUAL) MaterialKey = calculateMaterialKey();
			else
				MaterialKey = MaterialKey - materialKeyFactors[WPAWN + SideToMove] - materialKeyFactors[capturedInLastMove] + materialKeyFactors[convertedTo];
			material = &MaterialTable[MaterialKey];
		}
		break;
	case CASTLING:
		if (toSquare == G1 + (SideToMove * 56) || (toSquare == InitialRookSquare[2 * SideToMove])) {
			//short castling
			remove(InitialRookSquare[2 * SideToMove]); //remove rook
			toSquare = Square(G1 + (SideToMove * 56));
			set<true>(moving, toSquare); //Place king
			set<true>(Piece(WROOK + SideToMove),
				Square(toSquare - 1)); //Place rook
		}
		else {
			//long castling
			remove(InitialRookSquare[2 * SideToMove + 1]); //remove rook
			toSquare = Square(C1 + (SideToMove * 56));
			set<true>(moving, toSquare); //Place king
			set<true>(Piece(WROOK + SideToMove),
				Square(toSquare + 1)); //Place rook
		}
		RemoveCastlingOption(CastleFlag(W0_0 << (2 * SideToMove)));
		RemoveCastlingOption(CastleFlag(W0_0_0 << (2 * SideToMove)));
		SetEPSquare(OUTSIDE);
		++DrawPlyCount;
		break;
	}
	SwitchSideToMove();
	tt::prefetch(Hash);
	movepointer = 0;
	attackedByUs = calculateAttacks(SideToMove);
	//calculatePinned();
	attackedByThem = 0ull;
	assert((checkMaterialIsUnusual() && MaterialKey == MATERIAL_KEY_UNUSUAL) || MaterialKey == calculateMaterialKey());
	assert(PawnKey == calculatePawnKey());
	if (pawn->Key != PawnKey) pawn = pawn::probe(*this);
	lastAppliedMove = move;
	return !(attackedByUs & PieceBB(KING, Color(SideToMove ^ 1)));
	//if (attackedByUs & PieceBB(KING, Color(SideToMove ^ 1))) return false;
	//attackedByThem = calculateAttacks(Color(SideToMove ^1));
	//return true;
}

Move position::NextMove() {
	if (generationPhases[generationPhase] == NONE) return MOVE_NONE;
	do {
		if (moveIterationPointer < 0) {
			phaseStartIndex = movepointer - (movepointer != 0);
			switch (generationPhases[generationPhase]) {
			case KILLER:
				moveIterationPointer = 0;
				break;
			case WINNING_CAPTURES:
				GenerateMoves<WINNING_CAPTURES>();
				evaluateByMVVLVA();
				moveIterationPointer = 0;
				break;
			case EQUAL_CAPTURES:
				GenerateMoves<EQUAL_CAPTURES>();
				evaluateBySEE(phaseStartIndex);
				moveIterationPointer = 0;
				break;
			case LOOSING_CAPTURES:
				GenerateMoves<LOOSING_CAPTURES>();
				evaluateBySEE(phaseStartIndex);
				moveIterationPointer = 0;
				break;
			case QUIETS_POSITIVE:
				GenerateMoves<QUIETS>();
				evaluateByHistory(phaseStartIndex);
				lastPositive = std::partition(moves + phaseStartIndex, &moves[movepointer - 1], positiveScore);
				insertionSort(moves + phaseStartIndex, lastPositive);
				moveIterationPointer = 0;
				break;
			case CHECK_EVASION:
				GenerateMoves<CHECK_EVASION>();
				moveIterationPointer = 0;
				break;
			case QUIET_CHECKS:
				GenerateMoves<QUIET_CHECKS>();
				moveIterationPointer = 0;
				break;
			}

		}
		Move move;
		switch (generationPhases[generationPhase]) {
		case HASHMOVE:
			++generationPhase;
			moveIterationPointer = -1;
			if (validateMove(hashMove)) return hashMove;
			break;
		case KILLER:
			if (moveIterationPointer == 0) {
				moveIterationPointer++;
				if (!(killer->move == MOVE_NONE) && validateMove(*killer)) return killer->move;
			}
			if (moveIterationPointer == 1) {
				moveIterationPointer++;
				if (!((killer + 1)->move == MOVE_NONE) && validateMove(*(killer + 1))) return (killer + 1)->move;
			}
			++generationPhase;
			moveIterationPointer = -1;
			break;
		case WINNING_CAPTURES: case EQUAL_CAPTURES: case LOOSING_CAPTURES: case QUIETS_NEGATIVE:
			move = getBestMove(phaseStartIndex + moveIterationPointer);
			if (move) {
				++moveIterationPointer;
				return move;
			}
			else {
				++generationPhase;
				moveIterationPointer = -1;
			}
			break;
		case CHECK_EVASION: case QUIET_CHECKS:
#pragma warning(suppress: 6385)
			move = moves[phaseStartIndex + moveIterationPointer].move;
			++moveIterationPointer;
			generationPhase += (move == MOVE_NONE);
			return move;
		case QUIETS_POSITIVE:
			move = moves[phaseStartIndex + moveIterationPointer].move;
			++moveIterationPointer;
			generationPhase += (move == lastPositive->move);
			return move;
		}
	} while (generationPhases[generationPhase] != NONE);
	return MOVE_NONE;
}

//copied from Stockfish
void position::insertionSort(ValuatedMove* begin, ValuatedMove* end)
{
	ValuatedMove tmp, *p, *q;
	for (p = begin + 1; p < end; ++p)
	{
		tmp = *p;
		for (q = p; q != begin && *(q - 1) < tmp; --q)
			*q = *(q - 1);
		*q = tmp;
	}
}

void position::evaluateByMVVLVA() {
	for (int i = 0; i < movepointer - 1; ++i) {
		moves[i].score = (PieceValuesMG[GetPieceType(Board[to(moves[i].move)])] * 10) - PieceValuesMG[GetPieceType(Board[from(moves[i].move)])];
	}
}

void position::evaluateBySEE(int startIndex) {
	for (int i = startIndex; i < movepointer - 1; ++i) moves[i].score = SEE(from(moves[i].move), to(moves[i].move));
}

void position::evaluateByHistory(int startIndex) {
	Move counterMove = MOVE_NONE;
	if (CounterMoves && previous) {
		Square lastTo = to(previous->lastAppliedMove);
		counterMove = CounterMoves[(Board[lastTo] << 6) + lastTo];
	}
	for (int i = startIndex; i < movepointer - 1; ++i) {
		if (moves[i].move == counterMove) {
			moves[i].score = VALUE_MATE;
		}
		else {
			Square toSquare = to(moves[i].move);
			Piece p = Board[from(moves[i].move)];
			moves[i].score = history->getValue(p, toSquare);
			if (lastAppliedMove) moves[i].score += 2 * dblHistory->getValue(Board[to(lastAppliedMove)], to(lastAppliedMove), p, toSquare);
		}
	}
}

Move position::getBestMove(int startIndex) {
	ValuatedMove bestmove = moves[startIndex];
	for (int i = startIndex + 1; i < movepointer - 1; ++i) {
		if (bestmove < moves[i]) {
			moves[startIndex] = moves[i];
			moves[i] = bestmove;
			bestmove = moves[startIndex];
		}
	}
	return moves[startIndex].move;
}

template<bool SquareIsEmpty> void position::set(const Piece piece, const Square square) {
	Bitboard squareBB = 1ull << square;
	Piece captured;
	if (!SquareIsEmpty && (captured = Board[square]) != BLANK) {
		OccupiedByPieceType[GetPieceType(captured)] &= ~squareBB;
		OccupiedByColor[GetColor(captured)] &= ~squareBB;
		Hash ^= ZobristKeys[captured][square];
	}
	OccupiedByPieceType[GetPieceType(piece)] |= squareBB;
	OccupiedByColor[GetColor(piece)] |= squareBB;
	Board[square] = piece;
	Hash ^= ZobristKeys[piece][square];
}

void position::remove(const Square square){
	Bitboard NotSquareBB = ~(1ull << square);
	Piece piece = Board[square];
	Board[square] = BLANK;
	OccupiedByPieceType[GetPieceType(piece)] &= NotSquareBB;
	OccupiedByColor[GetColor(piece)] &= NotSquareBB;
	Hash ^= ZobristKeys[piece][square];
}

void position::updateCastleFlags(Square fromSquare, Square toSquare) {
	if (fromSquare == InitialKingSquare[SideToMove]) {
		RemoveCastlingOption(CastleFlag(W0_0 << (2 * SideToMove)));
		RemoveCastlingOption(CastleFlag(W0_0_0 << (2 * SideToMove)));
	}
	else if (fromSquare == InitialRookSquare[2 * SideToMove] || toSquare == InitialRookSquare[2 * SideToMove]) {
		RemoveCastlingOption(CastleFlag(W0_0 << (2 * SideToMove)));
	}
	else if (fromSquare == InitialRookSquare[2 * SideToMove + 1] || toSquare == InitialRookSquare[2 * SideToMove + 1]) {
		RemoveCastlingOption(CastleFlag(W0_0_0 << (2 * SideToMove)));
	}
}

Bitboard position::calculateAttacks(Color color) {
	Bitboard occupied = OccupiedBB();
	attacksByPt[GetPiece(ROOK, color)] = 0ull;
	Bitboard rookSliders = PieceBB(ROOK, color);
	while (rookSliders) {
		Square sq = lsb(rookSliders);
		attacks[sq] = RookTargets(sq, occupied);
		attacksByPt[GetPiece(ROOK, color)] |= attacks[sq];
		rookSliders &= rookSliders - 1;
	}
	attacksByPt[GetPiece(BISHOP, color)] = 0ull;
	Bitboard bishopSliders = PieceBB(BISHOP, color);
	while (bishopSliders) {
		Square sq = lsb(bishopSliders);
		attacks[sq] = BishopTargets(sq, occupied);
		attacksByPt[GetPiece(BISHOP, color)] |= attacks[sq];
		bishopSliders &= bishopSliders - 1;
	}
	attacksByPt[GetPiece(QUEEN, color)] = 0ull;
	Bitboard queenSliders = PieceBB(QUEEN, color);
	while (queenSliders) {
		Square sq = lsb(queenSliders);
		attacks[sq] = RookTargets(sq, occupied);
		attacks[sq] |= BishopTargets(sq, occupied);
		attacksByPt[GetPiece(QUEEN, color)] |= attacks[sq];
		queenSliders &= queenSliders - 1;
	}
	attacksByPt[GetPiece(KNIGHT, color)] = 0ull;
	Bitboard knights = PieceBB(KNIGHT, color);
	while (knights) {
		Square sq = lsb(knights);
		attacks[sq] = KnightAttacks[sq];
		attacksByPt[GetPiece(KNIGHT, color)] |= attacks[sq];
		knights &= knights - 1;
	}
	Square kingSquare = lsb(PieceBB(KING, color));
	attacks[kingSquare] = KingAttacks[kingSquare];
	attacksByPt[GetPiece(KING, color)] = attacks[kingSquare];
	attacksByPt[GetPiece(PAWN, color)] = 0ull;
	Bitboard pawns = PieceBB(PAWN, color);
	while (pawns) {
		Square sq = lsb(pawns);
		attacks[sq] = PawnAttacks[color][sq];
		attacksByPt[GetPiece(PAWN, color)] |= attacks[sq];
		pawns &= pawns - 1;
	}
	return attacksByPt[GetPiece(QUEEN, color)] | attacksByPt[GetPiece(ROOK, color)] | attacksByPt[GetPiece(BISHOP, color)]
		| attacksByPt[GetPiece(KNIGHT, color)] | attacksByPt[GetPiece(PAWN, color)] | attacksByPt[GetPiece(KING, color)];
}

Bitboard position::checkBlocker(Color colorOfBlocker, Color kingColor) {
	Bitboard result = EMPTY;
	Square kingSquare = lsb(PieceBB(KING, kingColor));
	Bitboard pinner = (OccupiedByPieceType[ROOK] | OccupiedByPieceType[QUEEN]) & SlidingAttacksRookTo[kingSquare];
	pinner |= (OccupiedByPieceType[BISHOP] | OccupiedByPieceType[QUEEN]) & SlidingAttacksBishopTo[kingSquare];
	pinner &= OccupiedByColor[kingColor ^ 1];
	Bitboard occ = OccupiedBB();
	while (pinner)
	{
		Bitboard blocker = InBetweenFields[lsb(pinner)][kingSquare] & occ;
		if (popcount(blocker) == 1) result |= blocker & OccupiedByColor[colorOfBlocker];
		pinner &= pinner - 1;
	}
	return result;
}

//Calculates the pinned Pieces as needed for move generation
//void position::calculatePinned() {
//	pinned = pinner = EMPTY;
//	Square kingSquare = lsb(PieceBB(KING, SideToMove));
//	Bitboard potentialPinner = (PieceBB(ROOK, Color(SideToMove ^ 1)) | PieceBB(QUEEN, Color(SideToMove ^ 1))) & SlidingAttacksRookTo[kingSquare];
//	potentialPinner |= (PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(QUEEN, Color(SideToMove ^ 1))) & SlidingAttacksBishopTo[kingSquare];
//	while (potentialPinner) {
//		Square pinnersSquare = lsb(potentialPinner);
//		Bitboard potentiallyPinned = InBetweenFields[pinnersSquare][kingSquare];
//		if ((potentiallyPinned & ColorBB(SideToMove)) && popcount(potentiallyPinned) == 1) {
//			pinned |= potentiallyPinned;
//			pinner |= ToBitboard(pinnersSquare);
//		}
//		potentialPinner &= potentialPinner - 1;
//	}
//}

const Bitboard position::considerXrays(const Bitboard occ, const Square to, const Bitboard fromSet, const Square from) const
{
	if ((fromSet & (SlidingAttacksRookTo[to] | SlidingAttacksBishopTo[to])) == 0) return 0;
	Bitboard shadowed = ShadowedFields[to][from];
	if ((shadowed & occ) == 0) return 0;
	Bitboard attack = SlidingAttacksRookTo[to] & (OccupiedByPieceType[ROOK] | OccupiedByPieceType[QUEEN]);
	attack |= SlidingAttacksBishopTo[to] & (OccupiedByPieceType[BISHOP] | OccupiedByPieceType[QUEEN]);
	attack &= shadowed;
	while (attack != 0)
	{
		int attackingField = lsb(attack);
		if ((InBetweenFields[attackingField][to] & occ) == 0) return ToBitboard(attackingField);
		attack &= attack - 1;
	}
	return 0;
}

const Bitboard position::AttacksOfField(const Square targetField) const
{
	//sliding attacks
	Bitboard attacks = SlidingAttacksRookTo[targetField] & (OccupiedByPieceType[ROOK] | OccupiedByPieceType[QUEEN]);
	attacks |= SlidingAttacksBishopTo[targetField] & (OccupiedByPieceType[BISHOP] | OccupiedByPieceType[QUEEN]);
	//Check for blockers
	Bitboard tmpAttacks = attacks;
	while (tmpAttacks != 0)
	{
		Square from = lsb(tmpAttacks);
		Bitboard blocker = InBetweenFields[from][targetField] & OccupiedBB();
		if (blocker) attacks &= ~ToBitboard(from);
		tmpAttacks &= tmpAttacks - 1;
	}
	//non-sliding attacks
	attacks |= KnightAttacks[targetField] & OccupiedByPieceType[KNIGHT];
	attacks |= KingAttacks[targetField] & OccupiedByPieceType[KING];
	Bitboard targetBB = ToBitboard(targetField);
	attacks |= ((targetBB >> 7) & NOT_A_FILE) & PieceBB(PAWN, WHITE);
	attacks |= ((targetBB >> 9) & NOT_H_FILE) & PieceBB(PAWN, WHITE);
	attacks |= ((targetBB << 7) & NOT_H_FILE) & PieceBB(PAWN, BLACK);
	attacks |= ((targetBB << 9) & NOT_A_FILE) & PieceBB(PAWN, BLACK);
	return attacks;
}

const Bitboard position::AttacksOfField(const Square targetField, const Color attackingSide) const {
	//sliding attacks
	Bitboard attacks = SlidingAttacksRookTo[targetField] & (OccupiedByPieceType[ROOK] | OccupiedByPieceType[QUEEN]);
	attacks |= SlidingAttacksBishopTo[targetField] & (OccupiedByPieceType[BISHOP] | OccupiedByPieceType[QUEEN]);
	attacks &= OccupiedByColor[attackingSide];
	//Check for blockers
	Bitboard tmpAttacks = attacks;
	while (tmpAttacks != 0)
	{
		Square from = lsb(tmpAttacks);
		Bitboard blocker = InBetweenFields[from][targetField] & OccupiedBB();
		if (blocker) attacks &= ~ToBitboard(from);
		tmpAttacks &= tmpAttacks - 1;
	}
	//non-sliding attacks
	attacks |= KnightAttacks[targetField] & OccupiedByPieceType[KNIGHT];
	attacks |= KingAttacks[targetField] & OccupiedByPieceType[KING];
	Bitboard targetBB = ToBitboard(targetField);
	attacks |= ((targetBB >> 7) & NOT_A_FILE) & PieceBB(PAWN, WHITE);
	attacks |= ((targetBB >> 9) & NOT_H_FILE) & PieceBB(PAWN, WHITE);
	attacks |= ((targetBB << 7) & NOT_H_FILE) & PieceBB(PAWN, BLACK);
	attacks |= ((targetBB << 9) & NOT_A_FILE) & PieceBB(PAWN, BLACK);
	attacks &= OccupiedByColor[attackingSide];
	return attacks;
}

const Bitboard position::getSquareOfLeastValuablePiece(const Bitboard attadef, const int side) const
{
	Color diff = Color((SideToMove + side) & 1);
	Bitboard subset = attadef & PieceBB(PAWN, diff); if (subset) return subset & (0 - subset); // single bit
	subset = attadef & PieceBB(KNIGHT, diff); if (subset) return subset & (0 - subset); // single bit
	subset = attadef & PieceBB(BISHOP, diff); if (subset) return subset & (0 - subset); // single bit
	subset = attadef & PieceBB(ROOK, diff); if (subset) return subset & (0 - subset); // single bit
	subset = attadef & PieceBB(QUEEN, diff); if (subset) return subset & (0 - subset); // single bit
	return 0; // empty set
}

//SEE with early return if SEE value can't be negative
Value position::SEE_Sign(Move move) const {
	Square fromSquare = from(move);
	Square toSquare = to(move);
	if (PieceValuesMG[GetPieceType(Board[fromSquare])] <= PieceValuesMG[GetPieceType(Board[toSquare])]) return VALUE_KNOWN_WIN;
	return SEE(fromSquare, toSquare);
}

const Value position::SEE(Square from, const Square to) const
{
	Value gain[32];
	int d = 0;
	Bitboard mayXray = OccupiedByPieceType[BISHOP] | OccupiedByPieceType[ROOK] | OccupiedByPieceType[QUEEN];
	Bitboard fromSet = ToBitboard(from);
	Bitboard occ = OccupiedBB();
	Bitboard attadef = AttacksOfField(to);
	gain[d] = PieceValuesMG[GetPieceType(Board[to])];
	do
	{
		d++; // next depth and side
		gain[d] = PieceValuesMG[GetPieceType(Board[from])] - gain[d - 1]; // speculative store, if defended
		if (std::max(-gain[d - 1], gain[d]) < 0) break; // pruning does not influence the result
		attadef ^= fromSet; // reset bit in set to traverse
		occ ^= fromSet; // reset bit in temporary occupancy (for x-Rays)
		if ((fromSet & mayXray) != 0)
			attadef |= considerXrays(occ, to, fromSet, from);
		fromSet = getSquareOfLeastValuablePiece(attadef, d & 1);
		from = lsb(fromSet);
	} while (fromSet != 0);
	while (--d != 0)
		gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
	return gain[0];
}


//"Copied" from Stockfish source code
void position::setFromFEN(const std::string& fen) {
	material = nullptr;
	std::fill_n(Board, 64, BLANK);
	OccupiedByColor[WHITE] = OccupiedByColor[BLACK] = 0ull;
	std::fill_n(OccupiedByPieceType, 6, 0ull);
	EPSquare = OUTSIDE;
	SideToMove = WHITE;
	DrawPlyCount = 0;
	AppliedMovesBeforeRoot = 0;
	Hash = ZobristMoveColor;
	std::istringstream ss(fen);
	ss >> std::noskipws;
	unsigned char token;

	//Piece placement
	size_t piece;
	char square = A8;
	while ((ss >> token) && !isspace(token)) {
		if (isdigit(token))
			square = square + (token - '0');
		else if (token == '/')
			square -= 16;
		else if ((piece = PieceToChar.find(token)) != std::string::npos) {
			set<true>((Piece)piece, (Square)square);
			square++;
		}
	}

	//Side to Move
	ss >> token;
	if (token != 'w') SwitchSideToMove();

	//Castles
	CastlingOptions = 0;
	ss >> token;
	while ((ss >> token) && !isspace(token)) {

		if (token == 'K') {
			AddCastlingOption(W0_0);
			InitialKingSquareBB[WHITE] = PieceBB(KING, WHITE);
			InitialKingSquare[WHITE] = lsb(InitialKingSquareBB[WHITE]);
			for (Square k = InitialKingSquare[WHITE]; k <= H1; ++k) {
				if (Board[k] == WROOK) {
					InitialRookSquare[0] = k;
					break;
				}
			}
		}
		else if (token == 'Q') {
			AddCastlingOption(W0_0_0);
			InitialKingSquareBB[WHITE] = PieceBB(KING, WHITE);
			InitialKingSquare[WHITE] = lsb(InitialKingSquareBB[WHITE]);
#pragma warning(suppress: 6295)
			for (Square k = InitialKingSquare[WHITE]; k >= A1; --k) {
				if (Board[k] == WROOK) {
					InitialRookSquare[1] = k;
					break;
				}
			}
		}
		else if (token == 'k') {
			AddCastlingOption(B0_0);
			InitialKingSquareBB[BLACK] = PieceBB(KING, BLACK);
			InitialKingSquare[BLACK] = lsb(InitialKingSquareBB[BLACK]);
			for (Square k = InitialKingSquare[BLACK]; k <= H8; ++k) {
				if (Board[k] == BROOK) {
					InitialRookSquare[2] = k;
					break;
				}
			}
		}
		else if (token == 'q') {
			AddCastlingOption(B0_0_0);
			InitialKingSquareBB[BLACK] = PieceBB(KING, BLACK);
			InitialKingSquare[BLACK] = lsb(InitialKingSquareBB[BLACK]);
			for (Square k = InitialKingSquare[BLACK]; k >= A8; --k) {
				if (Board[k] == BROOK) {
					InitialRookSquare[3] = k;
					break;
				}
			}

		}
		else if (token == '-') continue;
		else {
			File kingFile = File(lsb(PieceBB(KING, SideToMove)) & 7);
			if (token >= 'A' && token <= 'H') {
				InitialKingSquareBB[WHITE] = PieceBB(KING, WHITE);
				InitialKingSquare[WHITE] = lsb(InitialKingSquareBB[WHITE]);
				File rookFile = File((int)token - (int)'A');
				if (rookFile < kingFile) {
					AddCastlingOption(W0_0_0);
					InitialRookSquare[1] = Square(rookFile);
				}
				else {
					AddCastlingOption(W0_0);
					InitialRookSquare[0] = Square(rookFile);
				}
			}
			else if (token >= 'a' && token <= 'h') {
				InitialKingSquareBB[BLACK] = PieceBB(KING, BLACK);
				InitialKingSquare[BLACK] = lsb(InitialKingSquareBB[BLACK]);
				File rookFile = File((int)token - (int)'a');
				if (rookFile < kingFile) {
					AddCastlingOption(B0_0_0);
					InitialRookSquare[3] = Square(rookFile + 56);
				}
				else {
					AddCastlingOption(B0_0);
					InitialRookSquare[2] = Square(rookFile + 56);
				}
			}
		}
	}
	if (CastlingOptions) {
		Chess960 = Chess960 || (InitialKingSquare[WHITE] != E1) || (InitialRookSquare[0] != H1) || (InitialRookSquare[1] != A1);
		for (int i = 0; i < 4; ++i) InitialRookSquareBB[i] = 1ull << InitialRookSquare[i];
		Square ks;
		Square kt[4] = { G1, C1, G8, C8 };
		Square rt[4] = { F1, D1, F8, D8 };
		for (int i = 0; i < 4; ++i) {
			SquaresToBeEmpty[i] = 0ull;
			SquaresToBeUnattacked[i] = 0ull;
			if ((i & 1) == 0) ks = lsb(InitialKingSquareBB[i / 2]);
			for (int j = std::min(ks, kt[i]); j <= std::max(ks, kt[i]); ++j) SquaresToBeUnattacked[i] |= 1ull << j;
			for (int j = std::min(InitialRookSquare[i], rt[i]); j <= std::max(InitialRookSquare[i], rt[i]); ++j) {
				SquaresToBeEmpty[i] |= 1ull << j;
			}
			for (int j = std::min(ks, kt[i]); j <= std::max(ks, kt[i]); ++j) {
				SquaresToBeEmpty[i] |= 1ull << j;
			}
			SquaresToBeEmpty[i] &= ~InitialKingSquareBB[i / 2];
			SquaresToBeEmpty[i] &= ~InitialRookSquareBB[i];
		}
	}

	//EP-square
	char col, row;
	if (((ss >> col) && (col >= 'a' && col <= 'h'))
		&& ((ss >> row) && (row == '3' || row == '6'))) {
		EPSquare = (Square)(8 * (row - '0' - 1) + col - 'a');
		Square to;
		if (SideToMove == BLACK && EPSquare <= H4) {
			to = (Square)(EPSquare + 8);
			if ((GetEPAttackersForToField(to) & PieceBB(PAWN, BLACK)) == 0)
				EPSquare = OUTSIDE;
		}
		else if (SideToMove == WHITE && EPSquare >= A5) {
			to = (Square)(EPSquare - 8);
			if ((GetEPAttackersForToField(to) & PieceBB(PAWN, WHITE)) == 0)
				EPSquare = OUTSIDE;
		}
		if (EPSquare != OUTSIDE) Hash ^= ZobristEnPassant[EPSquare & 7];
	}
	std::string dpc;
	ss >> std::skipws >> dpc;
	if (dpc.length() > 0) {
		DrawPlyCount = atoi(dpc.c_str());
	}
	std::fill_n(attacks, 64, 0ull);
	PawnKey = calculatePawnKey();
	pawn = pawn::probe(*this);
	if (checkMaterialIsUnusual()) {
		MaterialKey = MATERIAL_KEY_UNUSUAL;
		material = &MaterialTable[MaterialKey];
		material->Score = calculateMaterialScore(*this);
	}
	else {
		MaterialKey = calculateMaterialKey();
		material = &MaterialTable[MaterialKey];
	}
	attackedByThem = calculateAttacks(Color(SideToMove ^ 1));
	attackedByUs = calculateAttacks(SideToMove);
	pliesFromRoot = 0;
}

std::string position::fen() const {

	int emptyCnt;
	std::ostringstream ss;

	for (Rank r = Rank8; r >= Rank1; --r) {
		for (File f = FileA; f <= FileH; ++f) {
			for (emptyCnt = 0; f <= FileH && Board[createSquare(r, f)] == BLANK; ++f)
				++emptyCnt;
			if (emptyCnt)
				ss << emptyCnt;

			if (f <= FileH)
				ss << PieceToChar[Board[createSquare(r, f)]];
		}

		if (r > Rank1)
			ss << '/';
	}

	ss << (SideToMove == WHITE ? " w " : " b ");

	if (CastlingOptions == NoCastles)
		ss << '-';
	else if (Chess960) {
		if (W0_0 & CastlingOptions) {
			Bitboard sideOfKing = 0ull;
			for (int i = InitialKingSquare[WHITE] + 1; i < 8; ++i) sideOfKing |= 1ull << i;
			sideOfKing &= ~InitialRookSquareBB[0];
			if (sideOfKing & PieceBB(ROOK, WHITE)) ss << char((int)'A' + InitialRookSquare[0]); else ss << 'K';
		}
		if (W0_0_0 & CastlingOptions) {
			Bitboard sideOfKing = 0ull;
			for (int i = InitialKingSquare[WHITE] + 1; i >= 0; --i) sideOfKing |= 1ull << i;
			sideOfKing &= ~InitialRookSquareBB[1];
			if (sideOfKing & PieceBB(ROOK, WHITE)) ss << char((int)'A' + InitialRookSquare[1]); else ss << 'Q';
		}
		if (B0_0 & CastlingOptions) {
			Bitboard sideOfKing = 0ull;
			for (int i = InitialKingSquare[BLACK] + 1; i < 64; ++i) sideOfKing |= 1ull << i;
			sideOfKing &= ~InitialRookSquareBB[2];
			if (sideOfKing & PieceBB(ROOK, BLACK)) ss << char((int)'a' + (InitialRookSquare[2] & 7)); else ss << 'k';
		}
		if (B0_0_0 & CastlingOptions) {
			Bitboard sideOfKing = 0ull;
			for (int i = InitialKingSquare[BLACK] + 1; i >= A8; --i) sideOfKing |= 1ull << i;
			sideOfKing &= ~InitialRookSquareBB[3];
			if (sideOfKing & PieceBB(ROOK, BLACK)) ss << char((int)'a' + (InitialRookSquare[3] & 7)); else ss << 'q';
		}
	}
	else {
		if (W0_0 & CastlingOptions)
			ss << 'K';
		if (W0_0_0 & CastlingOptions)
			ss << 'Q';
		if (B0_0 & CastlingOptions)
			ss << 'k';
		if (B0_0_0 & CastlingOptions)
			ss << 'q';
	}
	ss << (EPSquare == OUTSIDE ? " - " : " " + toString(EPSquare) + " ")
		<< int(DrawPlyCount) << " "
		//<< 1 + (_ply - (_sideToMove == BLACK)) / 2;
		<< "1";
	return ss.str();
}

std::string position::print() {
	std::ostringstream ss;

	ss << "\n +---+---+---+---+---+---+---+---+\n";

	for (int r = 7; r >= 0; --r) {
		for (int f = 0; f <= 7; ++f)
			ss << " | " << PieceToChar[Board[8 * r + f]];

		ss << " |\n +---+---+---+---+---+---+---+---+\n";
	}
	ss << "\nChecked:         " << std::boolalpha << IsCheck() << std::noboolalpha
		<< "\nEvaluation:      " << this->evaluate()
		<< "\nFen:             " << fen()
		<< "\nHash:            " << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << Hash
		//<< "\nNormalized Hash: " << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << GetNormalizedHash()
		<< "\nMaterial Key:    " << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << MaterialKey
		<< "\n";
	return ss.str();
}

std::string position::printGeneratedMoves() {
	std::ostringstream ss;
	for (int i = 0; i < movepointer - 1; ++i) {
		ss << toString(moves[i].move) << "\t" << moves[i].score << "\n";
	}
	return ss.str();
}

MaterialKey_t position::calculateMaterialKey() {
	MaterialKey_t key = MATERIAL_KEY_OFFSET;
	for (int i = WQUEEN; i <= BPAWN; ++i)
		key += materialKeyFactors[i] * popcount(PieceBB(GetPieceType(Piece(i)), Color(i & 1)));
	return key;
}

PawnKey_t position::calculatePawnKey() {
	PawnKey_t key = 0;
	Bitboard pawns = PieceBB(PAWN, WHITE);
	while (pawns) {
		key ^= ZobristKeys[WPAWN][lsb(pawns)];
		pawns &= pawns - 1;
	}
	pawns = PieceBB(PAWN, BLACK);
	while (pawns) {
		key ^= ZobristKeys[BPAWN][lsb(pawns)];
		pawns &= pawns - 1;
	}
	return key;
}

Result position::GetResult() {
	if (!result) {
		if (DrawPlyCount > 100 || checkRepetition()) result = DRAW;
		else if (Checked()) {
			if (CheckValidMoveExists<true>()) result = OPEN; else result = MATE;
		}
		else {
			if (CheckValidMoveExists<false>()) result = OPEN; else result = DRAW;
		}
	}
	return result;
}

bool position::checkRepetition() {
	position * prev = Previous();
	for (int i = 0; i < (std::min(pliesFromRoot + AppliedMovesBeforeRoot, int(DrawPlyCount)) >> 1); ++i) {
		prev = prev->Previous();
		if (prev->GetHash() == GetHash())
			return true;
		prev = prev->Previous();
	}
	return false;
}

//Hashmove isn't really reliable => therefore check is hashmove is a valid move
bool position::validateMove(Move move) {
	Square fromSquare = from(move);
	Piece movingPiece = Board[fromSquare];
	Square toSquare = to(move);
	bool result = (movingPiece != BLANK) && (GetColor(movingPiece) == SideToMove) //from field is occuppied by piece of correct color
		&& ((Board[toSquare] == BLANK) || (GetColor(Board[toSquare]) != SideToMove));
	if (result) {
		PieceType pt = GetPieceType(movingPiece);
		if (pt == PAWN) {
			switch (type(move)) {
			case NORMAL:
				result = !(ToBitboard(toSquare) & RANKS[7 - 7 * SideToMove]) && (((int(toSquare) - int(fromSquare)) == PawnStep() && Board[toSquare] == BLANK) ||
					(((int(toSquare) - int(fromSquare)) == 2 * PawnStep()) && ((fromSquare >> 3) == (1 + 5 * SideToMove)) && Board[toSquare] == BLANK && Board[toSquare - PawnStep()] == BLANK)
					|| (attacks[fromSquare] & OccupiedByColor[SideToMove ^ 1] & ToBitboard(toSquare)));
				break;
			case PROMOTION:
				result = (ToBitboard(toSquare) & RANKS[7 - 7 * SideToMove]) &&
					(((int(toSquare) - int(fromSquare)) == PawnStep() && Board[toSquare] == BLANK) || (attacks[fromSquare] & OccupiedByColor[SideToMove ^ 1] & ToBitboard(toSquare)));
				break;
			case ENPASSANT:
				result = toSquare == EPSquare;
			}
		}
		else if (pt == KING && type(move) == CASTLING) {
			result = (CastlingOptions & CastlesbyColor[SideToMove]) != 0
				&& ((InBetweenFields[fromSquare][InitialRookSquare[2 * SideToMove + (toSquare < fromSquare)]] & OccupiedBB()) == 0)
				&& (((InBetweenFields[fromSquare][toSquare] | ToBitboard(toSquare)) & attackedByThem) == 0)
				&& (InitialRookSquareBB[2 * SideToMove + (toSquare < fromSquare)] & PieceBB(ROOK, SideToMove))
				&& !IsCheck();
		}
		else if (type(move) == NORMAL) result = (attacks[fromSquare] & ToBitboard(toSquare)) != 0;
		else result = false;
	}
	return result;
#ifdef _DEBUG
	//	position checkPos(*this);
	//	checkPos.movepointer = 0;
	//	checkPos.attackedByThem = attackedByThem;
	//	checkPos.attackedByUs = attackedByUs;
	//	memcpy(checkPos.attacks, attacks, 64 * sizeof(Bitboard));
	//	ValuatedMove * moves = checkPos.GenerateMoves<ALL>();
	//	Move cmove = MOVE_NONE;
	//	bool found = false;
	//	while (cmove = moves->move) {
	//		if (cmove == move) {
	//			if (!result) __debugbreak();
	//			found = true;
	//			break;
	//		}
	//
	//		moves++;
	//	}
	//
	//	if (result && !found)
	//		__debugbreak();
#endif
	return result;
}

bool position::validateMove(ExtendedMove move) {
	Square fromSquare = from(move.move);
	return Board[fromSquare] == move.piece && validateMove(move.move);
}


void position::NullMove(Square epsquare) {
	SwitchSideToMove();
	SetEPSquare(epsquare);
	Bitboard tmp = attackedByThem;
	attackedByThem = attackedByUs;
	attackedByUs = tmp;
	if (StaticEval != VALUE_NOTYETDETERMINED) StaticEval = -StaticEval;
}

void position::deleteParents() {
	if (previous != nullptr) previous->deleteParents();
	delete(previous);
}

bool position::checkMaterialIsUnusual() {
	return popcount(PieceBB(QUEEN, WHITE)) > 1
		|| popcount(PieceBB(QUEEN, BLACK)) > 1
		|| popcount(PieceBB(ROOK, WHITE)) > 2
		|| popcount(PieceBB(ROOK, BLACK)) > 2
		|| popcount(PieceBB(BISHOP, WHITE)) > 2
		|| popcount(PieceBB(BISHOP, BLACK)) > 2
		|| popcount(PieceBB(KNIGHT, WHITE)) > 2
		|| popcount(PieceBB(KNIGHT, BLACK)) > 2;
}

std::string position::toSan(Move move) {
	Square toSquare = to(move);
	Square fromSquare = from(move);
	if (type(move) == CASTLING) {
		if (toSquare > fromSquare) return "O-O"; else return "O-O-O";
	}
	PieceType pt = GetPieceType(Board[from(move)]);
	bool isCapture = (Board[to(move)] != BLANK) || (type(move) == PROMOTION);
	if (pt == PAWN) {
		if (isCapture || type(move) == ENPASSANT) {
			if (type(move) == PROMOTION) {
				char ch[] = { toChar(File(fromSquare & 7)), 'x', toChar(File(toSquare & 7)), toChar(Rank(toSquare >> 3)), '=', "QRBN"[promotionType(move)], 0 };
				return ch;
			}
			else {
				char ch[] = { toChar(File(fromSquare & 7)), 'x', toChar(File(toSquare & 7)), toChar(Rank(toSquare >> 3)), 0 };
				return ch;
			}
		}
		else {
			if (type(move) == PROMOTION) {
				char ch[] = { toChar(File(toSquare & 7)), toChar(Rank(toSquare >> 3)), '=', "QRBN"[promotionType(move)], 0 };
				return ch;
			}
			else {
				char ch[] = { toChar(File(toSquare & 7)), toChar(Rank(toSquare >> 3)), 0 };
				return ch;
			}
		}
	}
	else if (pt == KING) {
		if (isCapture) {
			char ch[] = { 'K', 'x', toChar(File(toSquare & 7)), toChar(Rank(toSquare >> 3)), 0 };
			return ch;
		}
		else {
			char ch[] = { 'K', toChar(File(toSquare & 7)), toChar(Rank(toSquare >> 3)), 0 };
			return ch;
		}
	}
	//Check if diambiguation is needed
	Move dMove = MOVE_NONE;
	ValuatedMove * legalMoves = GenerateMoves<LEGAL>();
	while (legalMoves->move) {
		if (legalMoves->move != move && to(legalMoves->move) == toSquare && GetPieceType(Board[from(legalMoves->move)]) == pt) {
			dMove = legalMoves->move;
			break;
		}
	}
	char ch[6];
	ch[0] = "QRBN"[pt];
	int indx = 1;
	if (dMove != MOVE_NONE) {
		if ((from(dMove) & 7) == (fromSquare & 7)) ch[indx] = toChar(Rank(from(dMove) >> 3)); else ch[indx] = toChar(File(from(dMove) & 7));
		indx++;
	}
	if (isCapture) {
		ch[indx] = 'x';
		indx++;
	}
	ch[indx] = toChar(File(toSquare & 7));
	indx++;
	ch[indx] = toChar(Rank(toSquare >> 3));
	indx++ ;
	ch[indx] = 0;
	return ch;
}

Move position::parseSan(std::string move) {
	ValuatedMove * legalMoves = GenerateMoves<LEGAL>();
	while (legalMoves->move) {
		if (move.find(toSan(legalMoves->move)) != std::string::npos) return legalMoves->move;
	}
	return MOVE_NONE;
}
