#include <sstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include "position.h"

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
		set(moving, toSquare);
		//update epField
		if ((GetPieceType(moving) == PAWN)
			&& (abs(int(toSquare) - int(fromSquare)) == 16)
			&& (GetEPAttackersForToField(toSquare) & PieceBB(PAWN, Color(SideToMove ^ 1))))
		{
			SetEPSquare(Square(toSquare - PawnStep()));
		}
		else
			SetEPSquare(EPSquare = OUTSIDE);
		if (GetPieceType(moving) == PAWN || captured != BLANK)
			DrawPlyCount = 0;
		else
			++DrawPlyCount;
		//update castlings
		if (CastlingOptions != 0) updateCastleFlags(fromSquare, toSquare);
		break;
	case ENPASSANT:
		set(moving, toSquare);
		remove(Square(toSquare - PawnStep()));
		SetEPSquare(OUTSIDE);
		DrawPlyCount = 0;
		break;
	case PROMOTION:
		set(GetPiece(promotionType(move), SideToMove), toSquare);
		if (CastlingOptions != 0) updateCastleFlags(fromSquare, toSquare);
		SetEPSquare(OUTSIDE);
		DrawPlyCount = 0;
		break;
	case CASTLING:
		if (toSquare > fromSquare) {
			//short castling
			set(moving, toSquare); //Place king
			remove(Square(toSquare + 1)); //remove rook
			set(Piece(WROOK + SideToMove),
				Square(toSquare - 1)); //Place rook
		}
		else {
			//long castling
			set(moving, toSquare); //Place king
			remove(Square(toSquare - 2)); //remove rook
			set(Piece(WROOK + SideToMove),
				Square(toSquare + 1)); //Place rook
		}
		RemoveCastlingOption(CastleFlag(W0_0 << (2 * SideToMove)));
		RemoveCastlingOption(CastleFlag(W0_0_0 << (2 * SideToMove)));
		SetEPSquare(OUTSIDE);
		++DrawPlyCount;
		break;
	}
	SideToMove ^= 1;
	attackedByUs = calculateAttacks(SideToMove);
	attackedByThem = 0ull;
	return !(attackedByUs & PieceBB(KING, Color(SideToMove ^ 1)));
	//if (attackedByUs & PieceBB(KING, Color(SideToMove ^ 1))) return false;
	//attackedByThem = calculateAttacks(Color(SideToMove ^1));
	//return true;
}

void position::set(const Piece piece, const Square square) {
	Bitboard squareBB = 1ull << square;
	Piece captured;
	if ((captured = Board[square]) != BLANK) {
		OccupiedByPieceType[GetPieceType(captured)] &= ~squareBB;
		OccupiedByColor[GetColor(captured)] &= ~squareBB;
	}
	OccupiedByPieceType[GetPieceType(piece)] |= squareBB;
	OccupiedByColor[GetColor(piece)] |= squareBB;
	Board[square] = piece;
}
void position::remove(const Square square){
	Bitboard NotSquareBB = ~(1ull << square);
	Piece piece = Board[square];
	Board[square] = BLANK;
	OccupiedByPieceType[GetPieceType(piece)] &= NotSquareBB;
	OccupiedByColor[GetColor(piece)] &= NotSquareBB;
}

void position::updateCastleFlags(Square fromSquare, Square toSquare) {
	if (fromSquare == E1 + (56 * SideToMove)) {
		RemoveCastlingOption(CastleFlag(W0_0 << (2 * SideToMove)));
		RemoveCastlingOption(CastleFlag(W0_0_0 << (2 * SideToMove)));
	}
	else if (fromSquare == H1 + 56 * SideToMove || toSquare == H1 + 56 * SideToMove) {
		RemoveCastlingOption(CastleFlag(W0_0 << (2 * SideToMove)));
	}
	else if (fromSquare == A1 + 56 * SideToMove || toSquare == A1 + 56 * SideToMove) {
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


//"Copied" from Stockfish source code
void position::setFromFEN(const string& fen) {
	std::fill_n(Board, 64, BLANK);
	OccupiedByColor[WHITE] = OccupiedByColor[BLACK] = 0ull;
	std::fill_n(OccupiedByPieceType, 6, 0ull);
	EPSquare = OUTSIDE;
	SideToMove = WHITE;
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
			set((Piece)piece, (Square)square);
			square++;
		}
	}

	//Side to Move
	ss >> token;
	if (token != 'w') SideToMove = BLACK;

	//Castles
	ss >> token;
	while ((ss >> token) && !isspace(token)) {

		if (token == 'K')
			AddCastlingOption(W0_0);
		else if (token == 'Q')
			AddCastlingOption(W0_0_0);
		else if (token == 'k')
			AddCastlingOption(B0_0);
		else if (token == 'q')
			AddCastlingOption(B0_0_0);
		else
			continue;
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
	}

	ss >> skipws >> DrawPlyCount;
	std::fill_n(attacks, 64, 0ull);
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