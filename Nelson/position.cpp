#include <sstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "position.h"
#include "material.h"
#include "settings.h"

static const string PieceToChar("QqRrBbNnPpKk ");

position::position()
{
	setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

position::position(string fen)
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

bool position::ApplyMove(Move move) {
	Square fromSquare = from(move);
	Square toSquare = to(move);
	Piece moving = Board[fromSquare];
	Piece captured = Board[toSquare];
	remove(fromSquare);
	MoveType moveType = type(move);
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
		if (GetPieceType(moving) == PAWN || captured != BLANK)
			DrawPlyCount = 0;
		else
			++DrawPlyCount;
		//update castlings
		if (CastlingOptions != 0) updateCastleFlags(fromSquare, toSquare);
		break;
	case ENPASSANT:
		set<true>(moving, toSquare);
		remove(Square(toSquare - PawnStep()));
		MaterialKey -= materialKeyFactors[BPAWN - SideToMove];
		SetEPSquare(OUTSIDE);
		DrawPlyCount = 0;
		break;
	case PROMOTION:
		set<false>(GetPiece(promotionType(move), SideToMove), toSquare);
		MaterialKey += materialKeyFactors[GetPiece(promotionType(move), SideToMove)] - materialKeyFactors[moving];
		if (CastlingOptions != 0) updateCastleFlags(fromSquare, toSquare);
		SetEPSquare(OUTSIDE);
		DrawPlyCount = 0;
		break;
	case CASTLING:
		if (toSquare == G1 + (SideToMove * 56)) {
			//short castling
			remove(InitialRookSquare[2 * SideToMove]); //remove rook
			set<true>(moving, toSquare); //Place king
			set<true>(Piece(WROOK + SideToMove),
				Square(toSquare - 1)); //Place rook
		}
		else {
			//long castling
			remove(InitialRookSquare[2 * SideToMove + 1]); //remove rook
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
	attackedByUs = calculateAttacks(SideToMove);
	//calculatePinned();
	attackedByThem = 0ull;
	assert(MaterialKey == calculateMaterialKey());
	return !(attackedByUs & PieceBB(KING, Color(SideToMove ^ 1)));
	//if (attackedByUs & PieceBB(KING, Color(SideToMove ^ 1))) return false;
	//attackedByThem = calculateAttacks(Color(SideToMove ^1));
	//return true;
}

Move position::NextMove() {
	if (generationPhases[generationPhase] == NONE) return MOVE_NONE;
	do {
		if (moveIterationPointer < 0) {
			switch (generationPhases[generationPhase]) {
			case WINNING_CAPTURES:
				GenerateMoves<WINNING_CAPTURES>();
				evaluateByMVVLVA();
				break;
			case EQUAL_CAPTURES:
				GenerateMoves<EQUAL_CAPTURES>(); 
				evaluateBySEE();
				break;
			case LOOSING_CAPTURES:
				GenerateMoves<LOOSING_CAPTURES>();
				evaluateBySEE();
				break;
			case QUIETS:
				GenerateMoves<QUIETS>(); break;
				break;
			}
			moveIterationPointer = 0;
		}
		Move move = getBestMove(moveIterationPointer);
		if (move) {
			++moveIterationPointer;
			return move;
		}
		else {
			++generationPhase;
			moveIterationPointer = -1;
		}
	} while (generationPhases[generationPhase] != NONE);
	return MOVE_NONE;
}

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

void position::evaluateBySEE() {
	for (int i = 0; i < movepointer - 1; ++i) moves[i].score = SEE(from(moves[i].move), to(moves[i].move));
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
		MaterialKey -= materialKeyFactors[captured];
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
	Bitboard result = 0ull;
	Bitboard rookSliders = PieceBB(ROOK, color) | PieceBB(QUEEN, color);
	while (rookSliders) {
		Square sq = lsb(rookSliders);
		attacks[sq] = RookTargets(sq, occupied);
		result |= attacks[sq];
		rookSliders &= rookSliders - 1;
	}
	Bitboard bishopSliders = PieceBB(BISHOP, color);
	while (bishopSliders) {
		Square sq = lsb(bishopSliders);
		attacks[sq] = BishopTargets(sq, occupied);
		result |= attacks[sq];
		bishopSliders &= bishopSliders - 1;
	}
	Bitboard queenSliders = PieceBB(QUEEN, color);
	while (queenSliders) {
		Square sq = lsb(queenSliders);
		attacks[sq] |= BishopTargets(sq, occupied);
		result |= attacks[sq];
		queenSliders &= queenSliders - 1;
	}
	Bitboard knights = PieceBB(KNIGHT, color);
	while (knights) {
		Square sq = lsb(knights);
		attacks[sq] = KnightAttacks[sq];
		result |= attacks[sq];
		knights &= knights - 1;
	}
	Square kingSquare = lsb(PieceBB(KING, color));
	attacks[kingSquare] = KingAttacks[kingSquare];
	result |= attacks[kingSquare];
	Bitboard pawns = PieceBB(PAWN, color);
	while (pawns) {
		Square sq = lsb(pawns);
		attacks[sq] = PawnAttacks[color][sq];
		result |= attacks[sq];
		pawns &= pawns - 1;
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

const Bitboard position::considerXrays(const Bitboard occ, const Square to, const Bitboard fromSet, const Square from)
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

const Bitboard position::AttacksOfField(const Bitboard occupancy, const Square targetField)
{
	//sliding attacks
	Bitboard attacks = SlidingAttacksRookTo[targetField] & (OccupiedByPieceType[ROOK] | OccupiedByPieceType[QUEEN]);
	attacks |= SlidingAttacksBishopTo[targetField] & (OccupiedByPieceType[BISHOP] | OccupiedByPieceType[QUEEN]);
	//Check for blockers
	Bitboard tmpAttacks = attacks;
	while (tmpAttacks != 0)
	{
		Square from = lsb(tmpAttacks);
		Bitboard blocker = InBetweenFields[from][targetField] & occupancy;
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

const Bitboard position::getSquareOfLeastValuablePiece(const Bitboard attadef, const int side)
{
	Color diff = Color((SideToMove + side) & 1);
     Bitboard subset = attadef & PieceBB(PAWN, diff); if (subset) return subset & (0 - subset); // single bit
	 subset = attadef & PieceBB(KNIGHT, diff); if (subset) return subset & (0 - subset); // single bit
	 subset = attadef & PieceBB(BISHOP, diff); if (subset) return subset & (0 - subset); // single bit
	 subset = attadef & PieceBB(ROOK, diff); if (subset) return subset & (0 - subset); // single bit
	 subset = attadef & PieceBB(QUEEN, diff); if (subset) return subset & (0 - subset); // single bit
	return 0; // empty set
}

const Value position::SEE(Square from, const Square to)
{
	Value gain[32];
	int d = 0;
	Bitboard mayXray = OccupiedByPieceType[BISHOP] | OccupiedByPieceType[ROOK] | OccupiedByPieceType[QUEEN];
	Bitboard fromSet = ToBitboard(from);
	Bitboard occ = OccupiedBB();
	Bitboard attadef = AttacksOfField(occ, to);
	gain[d] = PieceValuesMG[GetPieceType(Board[to])];
	do
	{
		d++; // next depth and side
		gain[d] = PieceValuesMG[GetPieceType(Board[from])] - gain[d - 1]; // speculative store, if defended
		if (max(-gain[d - 1], gain[d]) < 0) break; // pruning does not influence the result
		attadef ^= fromSet; // reset bit in set to traverse
		occ ^= fromSet; // reset bit in temporary occupancy (for x-Rays)
		if ((fromSet & mayXray) != 0)
			attadef |= considerXrays(occ, to, fromSet, from);
		fromSet = getSquareOfLeastValuablePiece(attadef, d & 1);
		from = lsb(fromSet);
	} while (fromSet != 0);
	while (--d != 0)
		gain[d - 1] = -max(-gain[d - 1], gain[d]);
	return gain[0];
}


//"Copied" from Stockfish source code
void position::setFromFEN(const string& fen) {
	std::fill_n(Board, 64, BLANK);
	OccupiedByColor[WHITE] = OccupiedByColor[BLACK] = 0ull;
	std::fill_n(OccupiedByPieceType, 6, 0ull);
	EPSquare = OUTSIDE;
	SideToMove = WHITE;
	Hash = ZobristMoveColor;
	istringstream ss(fen);
	ss >> noskipws;
	char token;

	//Piece placement
	size_t piece;
	char square = A8;
	while ((ss >> token) && !isspace(token)) {
		if (isdigit(token))
			square = square + (token - '0');
		else if (token == '/')
			square -= 16;
		else if ((piece = PieceToChar.find(token)) != string::npos) {
			set<true>((Piece)piece, (Square)square);
			square++;
		}
	}

	//Side to Move
	ss >> token;
	if (token != 'w') SwitchSideToMove();

	//Castles
	CastlingOptions = 0;
	Chess960 = false;
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
		Chess960 = (InitialKingSquare[WHITE] != E1) || (InitialRookSquare[0] != H1) || (InitialRookSquare[1] != A1);
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

	ss >> skipws >> DrawPlyCount;
	std::fill_n(attacks, 64, 0ull);
	MaterialKey = calculateMaterialKey();
	attackedByThem = calculateAttacks(Color(SideToMove ^ 1));
	attackedByUs = calculateAttacks(SideToMove);
}

string position::fen() const {

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

string position::print() const {
	std::ostringstream ss;

	ss << "\n +---+---+---+---+---+---+---+---+\n";

	for (int r = 7; r >= 0; --r) {
		for (int f = 0; f <= 7; ++f)
			ss << " | " << PieceToChar[Board[8 * r + f]];

		ss << " |\n +---+---+---+---+---+---+---+---+\n";
	}
	ss << "\nFen: " << fen()
		//<< "\nHash:            " << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << _hash
		//<< "\nNormalized Hash: " << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << GetNormalizedHash()
		//<< "\nMaterial Hash:   " << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << _material_hash 
		<< "\n";
	return ss.str();
}

MaterialKey_t position::calculateMaterialKey() {
	MaterialKey_t key = MATERIAL_KEY_OFFSET;
	for (int i = WQUEEN; i <= BPAWN; ++i)
		key += materialKeyFactors[i] * popcount(PieceBB(PieceType(i / 2), Color(i & 1)));
	return key;
}