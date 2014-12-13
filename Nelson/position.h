#pragma once
#include "types.h"
#include "board.h"
#include "material.h"
#include <string>

using namespace std;

const MoveGenerationType generationPhases[12] = { WINNING_CAPTURES, EQUAL_CAPTURES, LOOSING_CAPTURES, QUIETS_POSITIVE, QUIETS_NEGATIVE, NONE, //Main Search Phases
                                                  WINNING_CAPTURES, EQUAL_CAPTURES, LOOSING_CAPTURES, NONE,                                   //QSearch Phases
                                                  CHECK_EVASION, NONE};                                 
const int generationPhaseOffset[] = { 0, 6, 10 };

struct position
{
public:
	position();
	position(string fen);
	position(position &pos);
	~position();

	Bitboard PieceBB(const PieceType pt, const Color c) const;
	Bitboard ColorBB(const Color c) const;
	Bitboard ColorBB(const int c) const;
	Bitboard OccupiedBB() const;
	string print();
	string fen() const;
	void setFromFEN(const string& fen);
	bool ApplyMove(Move move); //Applies a pseudo-legal move and returns true if move is legal
	static inline position UndoMove(position &pos) { return *pos.previous; }
	template<MoveGenerationType MGT> ValuatedMove * GenerateMoves();
	template<> ValuatedMove* GenerateMoves<QUIET_CHECKS>();
	inline uint64_t GetHash() const { return Hash; }
	inline MaterialKey_t GetMaterialKey() const { return MaterialKey; }
	template<StagedMoveGenerationType SMGT> void InitializeMoveIterator();
	Move NextMove();
	const Value position::SEE(Square from, const Square to);
	inline bool Checked() { return (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))) && IsCheck()); }
private:
	Bitboard OccupiedByColor[2];
	Bitboard OccupiedByPieceType[6];
	uint64_t Hash = ZobristMoveColor;
	MaterialKey_t MaterialKey;
	Square EPSquare;
	unsigned char CastlingOptions;
	unsigned char DrawPlyCount;
	Color SideToMove;
	MaterialTableEntry * material;
	Piece Board[64];

	position * previous;

	ValuatedMove moves[256];
	int movepointer = 0;
	Bitboard attacks[64];
	Bitboard attackedByThem;
	Bitboard attackedByUs;
	int moveIterationPointer;
	int generationPhase;
	ValuatedMove * lastPositive;
	
	//Bitboard pinned;
	//Bitboard pinner;

	template<bool SquareIsEmpty> void set(const Piece piece, const Square square);
	void remove(const Square square);
	inline void AddCastlingOption(const CastleFlag castleFlag) { Hash ^= ZobristCastles[CastlingOptions]; CastlingOptions |= castleFlag; Hash ^= ZobristCastles[CastlingOptions]; }
	inline void RemoveCastlingOption(const CastleFlag castleFlag) { Hash ^= ZobristCastles[CastlingOptions]; CastlingOptions &= ~castleFlag; Hash ^= ZobristCastles[CastlingOptions]; }
	inline void SetEPSquare(const Square square) {
		if (EPSquare != square) {
			if (EPSquare != OUTSIDE) {
				Hash ^= ZobristEnPassant[EPSquare & 7];
				EPSquare = square;
				if (EPSquare != OUTSIDE)
					Hash ^= ZobristEnPassant[EPSquare & 7];
			}
			else {
				EPSquare = square;
				Hash ^= ZobristEnPassant[EPSquare & 7];
			}
		}
	}
	inline const int PawnStep() const { return 8 - 16 * SideToMove; }
	inline void AddMove(Move move) { moves[movepointer].move = move; moves[movepointer].score = VALUE_NOTYETDETERMINED; ++movepointer; }
	inline void AddMove(Move move, Value score) { moves[movepointer].move = move; moves[movepointer].score = VALUE_NOTYETDETERMINED; ++movepointer; }
	inline void SwitchSideToMove() { SideToMove ^= 1; Hash ^= ZobristMoveColor; }
	void updateCastleFlags(Square fromSquare, Square toSquare);
	Bitboard calculateAttacks(Color color);
	Bitboard checkBlocker(Color colorOfBlocker, Color kingColor);
	MaterialKey_t calculateMaterialKey();
	void evaluateByMVVLVA();
	void evaluateBySEE();
	void evaluateByPSQ();
	Move getBestMove(int startIndex);
	void insertionSort(ValuatedMove* begin, ValuatedMove* end);
	const Bitboard considerXrays(const Bitboard occ, const Square to, const Bitboard fromSet, const Square from);
	const Bitboard AttacksOfField(const Square targetField);
	const Bitboard AttacksOfField(const Square targetField, const Color attackingSide);
	const Bitboard getSquareOfLeastValuablePiece(const Bitboard attadef, const int side);
	inline bool IsCheck() { return (attackedByThem & PieceBB(KING, SideToMove)) != EMPTY; }
};

inline Bitboard position::PieceBB(const PieceType pt, const Color c) const { return OccupiedByColor[c] & OccupiedByPieceType[pt]; }
inline Bitboard position::ColorBB(const Color c) const { return OccupiedByColor[c]; }
inline Bitboard position::ColorBB(const int c) const { return OccupiedByColor[c]; }
inline Bitboard position::OccupiedBB() const { return OccupiedByColor[WHITE] | OccupiedByColor[BLACK]; }

//Generates all quiet moves giving check
template<> ValuatedMove* position::GenerateMoves<QUIET_CHECKS>() {
	movepointer = 0;
	//There are 2 options to give check: Either give check with the moving piece, or a discovered check by 
	//moving a check blocking piece
	Square opposedKingSquare = lsb(PieceBB(KING, Color(SideToMove ^ 1)));
	//1. Discovered Checks
	Bitboard discoveredCheckCandidates = checkBlocker(SideToMove, Color(SideToMove ^ 1));
	Bitboard targets = ~OccupiedBB();
	//1a Sliders
	Bitboard sliders = (PieceBB(ROOK, SideToMove) | PieceBB(QUEEN, SideToMove) | PieceBB(BISHOP, SideToMove)) & discoveredCheckCandidates;
	while (sliders) {
		Square from = lsb(sliders);
		Bitboard sliderTargets = attacks[from] & targets & (~InBetweenFields[opposedKingSquare][from] & ~ShadowedFields[opposedKingSquare][from]);
		while (sliderTargets) {
			AddMove(createMove(from, lsb(sliderTargets)));
			sliderTargets &= sliderTargets - 1;
		}
		sliders &= sliders - 1;
	}
	//1b Knights
	Bitboard knights = PieceBB(KNIGHT, SideToMove) & discoveredCheckCandidates;
	while (knights) {
		Square from = lsb(knights);
		Bitboard knightTargets = KnightAttacks[from] & targets;
		while (knightTargets) {
			AddMove(createMove(from, lsb(knightTargets)));
			knightTargets &= knightTargets - 1;
		}
		knights &= knights - 1;
	}
	//1c Kings
	Square kingSquare = lsb(PieceBB(KING, SideToMove));
	if (discoveredCheckCandidates & PieceBB(KING, SideToMove)) {
		Bitboard kingTargets = KingAttacks[kingSquare] & ~attackedByThem & targets & (~InBetweenFields[opposedKingSquare][kingSquare] & ~ShadowedFields[opposedKingSquare][kingSquare]);
		while (kingTargets) {
			AddMove(createMove(kingSquare, lsb(kingTargets)));
			kingTargets &= kingTargets - 1;
		}
	}
	//1d Pawns
	Bitboard pawns = PieceBB(PAWN, SideToMove) & discoveredCheckCandidates;
	if (pawns) {
		Bitboard singleStepTargets;
		Bitboard doubleStepTargets;
		if (SideToMove == WHITE) {
			singleStepTargets = (pawns << 8) & ~RANK8 & targets;
			doubleStepTargets = ((singleStepTargets & RANK3) << 8) & targets;
		}
		else {
			singleStepTargets = (pawns >> 8) & ~RANK1 & targets;
			doubleStepTargets = ((singleStepTargets & RANK6) >> 8) & targets;
		}
		while (singleStepTargets) {
			Square to = lsb(singleStepTargets);
			Square from = Square(to - PawnStep());
			Bitboard stillBlocking = InBetweenFields[from][opposedKingSquare] | ShadowedFields[opposedKingSquare][from];
			if (!(stillBlocking & ToBitboard(to))) AddMove(createMove(from, to));
			singleStepTargets &= singleStepTargets - 1;
		}
		while (doubleStepTargets) {
			Square to = lsb(doubleStepTargets);
			Square from = Square(to - 2 * PawnStep());
			Bitboard stillBlocking = InBetweenFields[from][opposedKingSquare] | ShadowedFields[opposedKingSquare][from];
			if (!(stillBlocking & ToBitboard(to))) AddMove(createMove(from, to));
			doubleStepTargets &= doubleStepTargets - 1;
		}
	}
	//2. Normal checks
	//2a "Rooks"
	Bitboard rookAttackstoKing = RookTargets(opposedKingSquare, OccupiedBB()) & targets;
	Bitboard rooks = (PieceBB(ROOK, SideToMove) | PieceBB(QUEEN, SideToMove)) & ~discoveredCheckCandidates;
	while (rooks) {
		Square from = lsb(rooks);
		Bitboard rookTargets = attacks[from] & rookAttackstoKing;
		while (rookTargets) {
			AddMove(createMove(from, lsb(rookTargets)));
			rookTargets &= rookTargets - 1;
		}
		rooks &= rooks - 1;
	}
	//2b "Bishops" 
	Bitboard bishopAttackstoKing = BishopTargets(opposedKingSquare, OccupiedBB()) & targets;
	Bitboard bishops = (PieceBB(BISHOP, SideToMove) | PieceBB(QUEEN, SideToMove)) & ~discoveredCheckCandidates;
	while (bishops) {
		Square from = lsb(bishops);
		Bitboard bishopTargets = attacks[from] & bishopAttackstoKing;
		while (bishopTargets) {
			AddMove(createMove(from, lsb(bishopTargets)));
			bishopTargets &= bishopTargets - 1;
		}
		bishops &= bishops - 1;
	}
	//2c Knights
	Bitboard knightAttacksToKing = KnightAttacks[opposedKingSquare];
	knights = PieceBB(KNIGHT, SideToMove) & ~discoveredCheckCandidates;
	while (knights) {
		Square from = lsb(knights);
		Bitboard knightTargets = KnightAttacks[from] & knightAttacksToKing & targets;
		while (knightTargets) {
			AddMove(createMove(from, lsb(knightTargets)));
			knightTargets &= knightTargets - 1;
		}
		knights &= knights - 1;
	}
	//2d Pawn
	Bitboard pawnTargets;
	Bitboard pawnFrom;
	Bitboard dblPawnFrom;
	if (SideToMove == WHITE) {
		pawnTargets = targets & (((PieceBB(KING, Color(SideToMove ^ 1)) >> 9) & NOT_H_FILE) | ((PieceBB(KING, Color(SideToMove ^ 1)) >> 7) & NOT_A_FILE));
		pawnFrom = pawnTargets >> 8;
		dblPawnFrom = ((pawnFrom & targets & RANK3) >> 8) & PieceBB(PAWN, WHITE);
		pawnFrom &= PieceBB(PAWN, WHITE);
	} 
	else {
		pawnTargets = targets & (((PieceBB(KING, Color(SideToMove ^ 1)) << 7) & NOT_H_FILE) | ((PieceBB(KING, Color(SideToMove ^ 1)) << 9) & NOT_A_FILE));
		pawnFrom = pawnTargets << 8;
		dblPawnFrom = ((pawnFrom & targets & RANK6) << 8) & PieceBB(PAWN, BLACK);
		pawnFrom &= PieceBB(PAWN, BLACK);
	}
	while (pawnFrom) {
		Square from = lsb(pawnFrom);
		AddMove(createMove(from, from + PawnStep()));
		pawnFrom &= pawnFrom - 1;
	}
	while (dblPawnFrom) {
		Square from = lsb(dblPawnFrom);
		AddMove(createMove(from, from + 2 * PawnStep()));
		dblPawnFrom &= dblPawnFrom - 1;
	}
	//2e Castles
	if (CastlingOptions & CastlesbyColor[SideToMove]) //King is on initial square
	{
		//King-side castles
		if ((CastlingOptions & (1 << (2 * SideToMove))) //Short castle allowed
			&& (InitialRookSquareBB[2 * SideToMove] & PieceBB(ROOK, SideToMove)) //Rook on initial square
			&& !(SquaresToBeEmpty[2 * SideToMove] & OccupiedBB()) //Fields between Rook and King are empty
			&& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
			&& !(SquaresToBeUnattacked[2 * SideToMove] & attackedByThem) //Fields passed by the king are unattacked
			&& (RookTargets(opposedKingSquare, ~targets) & RookSquareAfterCastling[2 * SideToMove])) //Rook is giving check after castling
			AddMove(createMove<CASTLING>(kingSquare, Square(G1 + SideToMove * 56)));
		//Queen-side castles
		if ((CastlingOptions & (1 << (2 * SideToMove + 1))) //Short castle allowed
			&& (InitialRookSquareBB[2 * SideToMove + 1] & PieceBB(ROOK, SideToMove)) //Rook on initial square
			&& !(SquaresToBeEmpty[2 * SideToMove + 1] & OccupiedBB()) //Fields between Rook and King are empty
			& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
			&& !(SquaresToBeUnattacked[2 * SideToMove + 1] & attackedByThem) //Fields passed by the king are unattacked
			&& (RookTargets(opposedKingSquare, ~targets) & RookSquareAfterCastling[2 * SideToMove + 1])) //Rook is giving check after castling
			AddMove(createMove<CASTLING>(kingSquare, Square(C1 + SideToMove * 56)));
	}
	AddMove(MOVE_NONE);
	return &moves[0];
}

template<MoveGenerationType MGT> ValuatedMove * position::GenerateMoves() {
	//Rooksliders
	Bitboard targets;
	movepointer = 0;
	if (MGT == ALL || MGT == TACTICAL || MGT == QUIETS || MGT == CHECK_EVASION) {
		Square kingSquare = lsb(PieceBB(KING, SideToMove));
		Bitboard checkBlocker;
		bool doubleCheck = false;
		if (MGT == CHECK_EVASION) {
			Bitboard checker = AttacksOfField(kingSquare, Color(SideToMove ^ 1));
			if (popcount(checker) == 1) {
				checkBlocker = checker | InBetweenFields[kingSquare][lsb(checker)];
			}
		}
		Bitboard empty = ~ColorBB(BLACK) & ~ColorBB(WHITE);
		if (MGT == ALL) targets = ~ColorBB(SideToMove);
		else if (MGT == TACTICAL) targets = ColorBB(SideToMove ^ 1);
		else if (MGT == QUIETS) targets = empty;
		else if (MGT == CHECK_EVASION) targets = ~ColorBB(SideToMove) & checkBlocker;
		//Kings
		Bitboard kingTargets;
		if (MGT == CHECK_EVASION) kingTargets = KingAttacks[kingSquare] & ~OccupiedByColor[SideToMove] & ~attackedByThem;
		else kingTargets = KingAttacks[kingSquare] & targets & ~attackedByThem;
		while (kingTargets) {
			AddMove(createMove(kingSquare, lsb(kingTargets)));
			kingTargets &= kingTargets - 1;
		}
		if (MGT == ALL || MGT == QUIETS) {
			if (CastlingOptions & CastlesbyColor[SideToMove]) //King is on initial square
			{
				//King-side castles
				if ((CastlingOptions & (1 << (2 * SideToMove))) //Short castle allowed
					&& (InitialRookSquareBB[2 * SideToMove] & PieceBB(ROOK, SideToMove)) //Rook on initial square
					&& !(SquaresToBeEmpty[2 * SideToMove] & OccupiedBB()) //Fields between Rook and King are empty
					&& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
					&& !(SquaresToBeUnattacked[2 * SideToMove] & attackedByThem)) //Fields passed by the king are unattacked
					AddMove(createMove<CASTLING>(kingSquare, Square(G1 + SideToMove * 56)));
				//Queen-side castles
				if ((CastlingOptions & (1 << (2 * SideToMove + 1))) //Short castle allowed
					&& (InitialRookSquareBB[2 * SideToMove + 1] & PieceBB(ROOK, SideToMove)) //Rook on initial square
					&& !(SquaresToBeEmpty[2 * SideToMove + 1] & OccupiedBB()) //Fields between Rook and King are empty
					& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
					&& !(SquaresToBeUnattacked[2 * SideToMove + 1] & attackedByThem)) //Fields passed by the king are unattacked
					AddMove(createMove<CASTLING>(kingSquare, Square(C1 + SideToMove * 56)));
			}
		}
		if (!doubleCheck) {
			Bitboard sliders = PieceBB(ROOK, SideToMove) | PieceBB(QUEEN, SideToMove) | PieceBB(BISHOP, SideToMove);
			while (sliders) {
				Square from = lsb(sliders);
				Bitboard sliderTargets = attacks[from] & targets;
				while (sliderTargets) {
					AddMove(createMove(from, lsb(sliderTargets)));
					sliderTargets &= sliderTargets - 1;
				}
				sliders &= sliders - 1;
			}
			////Bishopsliders
			//Bitboard bishopSliders = PieceBB(QUEEN, SideToMove) | PieceBB(BISHOP, SideToMove);
			//while (bishopSliders) {
			//	Square from = lsb(bishopSliders);
			//	Bitboard bishopTargets = attacks[from] & targets;
			//	while (bishopTargets) {
			//		AddMove(createMove(from, lsb(bishopTargets)));
			//		bishopTargets &= bishopTargets - 1;
			//	}
			//	bishopSliders &= bishopSliders - 1;
			//}
			//Knights
			Bitboard knights = PieceBB(KNIGHT, SideToMove);
			while (knights) {
				Square from = lsb(knights);
				Bitboard knightTargets;
				if (MGT == CHECK_EVASION) knightTargets = attacks[from] & targets & checkBlocker; else knightTargets = attacks[from] & targets;
				while (knightTargets) {
					AddMove(createMove(from, lsb(knightTargets)));
					knightTargets &= knightTargets - 1;
				}
				knights &= knights - 1;
			}
			//Pawns
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			//Captures
			if (MGT == ALL || MGT == TACTICAL || MGT == CHECK_EVASION) {
				while (pawns) {
					Square from = lsb(pawns);
					Bitboard pawnTargets;
					if (MGT == CHECK_EVASION) pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & ~RANK1and8 & checkBlocker; else pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & ~RANK1and8;
					while (pawnTargets) {
						AddMove(createMove(from, lsb(pawnTargets)));
						pawnTargets &= pawnTargets - 1;
					}
					//Promotion Captures
					if (MGT == CHECK_EVASION) pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8 & checkBlocker; else pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
					while (pawnTargets) {
						Square to = lsb(pawnTargets);
						AddMove(createMove<PROMOTION>(from, to, QUEEN));
						AddMove(createMove<PROMOTION>(from, to, ROOK));
						AddMove(createMove<PROMOTION>(from, to, BISHOP));
						AddMove(createMove<PROMOTION>(from, to, KNIGHT));
						pawnTargets &= pawnTargets - 1;
					}
					pawns &= pawns - 1;
				}
			}
			Bitboard singleStepTarget;
			Bitboard doubleSteptarget;
			Bitboard promotionTarget;
			if (SideToMove == WHITE) {
				if (MGT == ALL || MGT == QUIETS) {
					singleStepTarget = (PieceBB(PAWN, WHITE) << 8) & empty & ~RANK8;
					doubleSteptarget = ((singleStepTarget & RANK3) << 8) & empty;
				}
				else if (MGT == CHECK_EVASION) {
					singleStepTarget = (PieceBB(PAWN, WHITE) << 8) & empty & ~RANK8;
					doubleSteptarget = ((singleStepTarget & RANK3) << 8) & empty & checkBlocker;
					singleStepTarget &= checkBlocker;
				}
				if (MGT == ALL || MGT == TACTICAL) promotionTarget = (PieceBB(PAWN, WHITE) << 8) & empty & RANK8;
				else if (MGT == CHECK_EVASION) promotionTarget = (PieceBB(PAWN, WHITE) << 8) & empty & RANK8 & checkBlocker;
			}
			else {
				if (MGT == ALL || MGT == QUIETS) {
					singleStepTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & ~RANK1;
					doubleSteptarget = ((singleStepTarget & RANK6) >> 8) & empty;
				}
				else if (MGT == CHECK_EVASION) {
					singleStepTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & ~RANK1;
					doubleSteptarget = ((singleStepTarget & RANK6) >> 8) & empty & checkBlocker;
					singleStepTarget &= checkBlocker;
				}
				if (MGT == ALL || MGT == TACTICAL) promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & RANK1;
				else if (MGT == CHECK_EVASION) promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & empty & RANK1 & checkBlocker;
			}
			if (MGT == ALL || MGT == QUIETS || MGT == CHECK_EVASION) {
				while (singleStepTarget) {
					Square to = lsb(singleStepTarget);
					AddMove(createMove(to - PawnStep(), to));
					singleStepTarget &= singleStepTarget - 1;
				}
				while (doubleSteptarget) {
					Square to = lsb(doubleSteptarget);
					AddMove(createMove(to - 2 * PawnStep(), to));
					doubleSteptarget &= doubleSteptarget - 1;
				}
			}
			//Promotions
			if (MGT == ALL || MGT == TACTICAL || MGT == CHECK_EVASION) {
				while (promotionTarget) {
					Square to = lsb(promotionTarget);
					Square from = Square(to - PawnStep());
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					AddMove(createMove<PROMOTION>(from, to, ROOK));
					AddMove(createMove<PROMOTION>(from, to, BISHOP));
					AddMove(createMove<PROMOTION>(from, to, KNIGHT));
					promotionTarget &= promotionTarget - 1;
				}
				Bitboard epAttacker;
				if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
					while (epAttacker) {
						AddMove(createMove<ENPASSANT>(lsb(epAttacker), EPSquare));
						epAttacker &= epAttacker - 1;
					}
				}
			}
		}
	}
	else { //Winning, Equal and loosing captures
		Bitboard targets;
		Bitboard sliders;
		if (MGT == EQUAL_CAPTURES || MGT == LOOSING_CAPTURES) { //Queen Captures are never winning
			sliders = PieceBB(QUEEN, SideToMove);
			if (MGT == EQUAL_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1));
			else if (MGT == LOOSING_CAPTURES) targets = PieceBB(ROOK, Color(SideToMove ^ 1)) | PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(PAWN, Color(SideToMove ^ 1));
			while (sliders) {
				Square from = lsb(sliders);
				Bitboard sliderTargets = attacks[from] & targets;
				while (sliderTargets) {
					AddMove(createMove(from, lsb(sliderTargets)));
					sliderTargets &= sliderTargets - 1;
				}
				sliders &= sliders - 1;
			}
		}
		//Rook Captures
		sliders = PieceBB(ROOK, SideToMove);
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1));
		else if (MGT == EQUAL_CAPTURES) targets = PieceBB(ROOK, Color(SideToMove ^ 1));
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(PAWN, Color(SideToMove ^ 1));
		while (sliders) {
			Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
		//Bishop Captures
		sliders = PieceBB(BISHOP, SideToMove);
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1));
		else if (MGT == EQUAL_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1));
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(PAWN, Color(SideToMove ^ 1));
		while (sliders) {
			Square from = lsb(sliders);
			Bitboard sliderTargets = attacks[from] & targets;
			while (sliderTargets) {
				AddMove(createMove(from, lsb(sliderTargets)));
				sliderTargets &= sliderTargets - 1;
			}
			sliders &= sliders - 1;
		}
		//Knight Captures
		if (MGT == WINNING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1));
		else if (MGT == EQUAL_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1));
		else if (MGT == LOOSING_CAPTURES) targets = PieceBB(PAWN, Color(SideToMove ^ 1));
		Bitboard knights = PieceBB(KNIGHT, SideToMove);
		while (knights) {
			Square from = lsb(knights);
			Bitboard knightTargets = attacks[from] & targets;
			while (knightTargets) {
				AddMove(createMove(from, lsb(knightTargets)));
				knightTargets &= knightTargets - 1;
			}
			knights &= knights - 1;
		}
		//Pawn Captures
		if (MGT == WINNING_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				Square from = lsb(pawns);
				Bitboard pawnTargets = ColorBB(SideToMove ^ 1) & ~PieceBB(PAWN, Color(SideToMove ^ 1)) & attacks[from] & ~RANK1and8;
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				//Promotion Captures
				pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
				while (pawnTargets) {
					Square to = lsb(pawnTargets);
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					AddMove(createMove<PROMOTION>(from, to, ROOK));
					AddMove(createMove<PROMOTION>(from, to, BISHOP));
					AddMove(createMove<PROMOTION>(from, to, KNIGHT));
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
		}
		else if (MGT == EQUAL_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				Square from = lsb(pawns);
				Bitboard pawnTargets = PieceBB(PAWN, Color(SideToMove ^ 1)) & attacks[from];
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				pawns &= pawns - 1;
			}
			Bitboard epAttacker;
			if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
				while (epAttacker) {
					AddMove(createMove<ENPASSANT>(lsb(epAttacker), EPSquare));
					epAttacker &= epAttacker - 1;
				}
			}
		}
		if (MGT == WINNING_CAPTURES) {
			Bitboard promotionTarget;
			if (SideToMove == WHITE)  promotionTarget = (PieceBB(PAWN, WHITE) << 8) & ~OccupiedBB() & RANK8; else promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & ~OccupiedBB() & RANK1;
			while (promotionTarget) {
				Square to = lsb(promotionTarget);
				Square from = Square(to - PawnStep());
				AddMove(createMove<PROMOTION>(from, to, QUEEN));
				AddMove(createMove<PROMOTION>(from, to, ROOK));
				AddMove(createMove<PROMOTION>(from, to, BISHOP));
				AddMove(createMove<PROMOTION>(from, to, KNIGHT));
				promotionTarget &= promotionTarget - 1;
			}
			//King Captures are always winning as kings can only capture uncovered pieces
			Square kingSquare = lsb(PieceBB(KING, SideToMove));
			Bitboard kingTargets = KingAttacks[kingSquare] & ColorBB(SideToMove ^1) & ~attackedByThem;
			while (kingTargets) {
				AddMove(createMove(kingSquare, lsb(kingTargets)));
				kingTargets &= kingTargets - 1;
			}
		}
	}
	AddMove(MOVE_NONE);
	return &moves[0];
}


template<StagedMoveGenerationType SMGT> void position::InitializeMoveIterator() {
	if (!attackedByThem) attackedByThem = calculateAttacks(Color(SideToMove ^ 1));
	moveIterationPointer = -1;
	if (IsCheck()) generationPhase = generationPhaseOffset[CHECK];
	else generationPhase = generationPhaseOffset[SMGT];
}

