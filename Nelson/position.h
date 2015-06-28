#pragma once
#include "types.h"
#include "board.h"
#include "material.h"
#include "hashtables.h"
#include <string>

struct evaluation;

const MoveGenerationType generationPhases[26] = { HASHMOVE, NON_LOOSING_CAPTURES, KILLER, LOOSING_CAPTURES, QUIETS_POSITIVE, QUIETS_NEGATIVE, UNDERPROMOTION, NONE, //Main Search Phases
HASHMOVE, NON_LOOSING_CAPTURES, LOOSING_CAPTURES, NONE,                                   //QSearch Phases
HASHMOVE, CHECK_EVASION, UNDERPROMOTION, NONE, //Check Evasion
HASHMOVE, NON_LOOSING_CAPTURES, LOOSING_CAPTURES, QUIET_CHECKS, UNDERPROMOTION, NONE, //QSearch with Checks
REPEAT_ALL, NONE,
ALL, NONE };
const int generationPhaseOffset[] = { 0, 8, 12, 16, 22, 24 };

struct position
{
public:
	position();
	position(std::string fen);
	position(position &pos);
	~position();

	static int AppliedMovesBeforeRoot;

	Bitboard PieceBB(const PieceType pt, const Color c) const;
	Bitboard ColorBB(const Color c) const;
	Bitboard ColorBB(const int c) const;
	Bitboard PieceTypeBB(const PieceType pt) const;
	Bitboard OccupiedBB() const;
	Bitboard NonPawnMaterial(const Color c) const;
	std::string print();
	std::string printGeneratedMoves();
	std::string fen() const;
	void setFromFEN(const std::string& fen);
	bool ApplyMove(Move move); //Applies a pseudo-legal move and returns true if move is legal
	static inline position UndoMove(position &pos) { return *pos.previous; }
	inline position * Previous() { return previous; }
	template<MoveGenerationType MGT> ValuatedMove * GenerateMoves();
	inline uint64_t GetHash() const { return Hash; }
	inline MaterialKey_t GetMaterialKey() const { return MaterialKey; }
	inline PawnKey_t GetPawnKey() const { return PawnKey; }
	template<StagedMoveGenerationType SMGT> void InitializeMoveIterator(HistoryStats *history, DblHistoryStats *dblHistoryStats, ExtendedMove * killerMove, Move * counterMoves, Move hashmove = MOVE_NONE, Value limit = -VALUE_MATE);
	Move NextMove();
	const Value SEE(Square from, const Square to) const;
	Value SEE_Sign(Move move) const;
	inline bool IsCheck() const { return (attackedByThem & PieceBB(KING, SideToMove)) != EMPTY; }
	inline bool Checked() { return (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1)))) && IsCheck(); }
	friend evaluation evaluate(position& pos);
	friend evaluation evaluateFromScratch(position &pos);
	inline Value evaluate();
	inline Value evaluateFinalPosition();
	inline int GeneratedMoveCount() const { return movepointer - 1; }
	inline int GetPliesFromRoot() const { return pliesFromRoot; }
	inline Color GetSideToMove() const { return SideToMove; }
	inline Piece GetPieceOnSquare(Square square) const { return Board[square]; }
	inline Square GetEPSquare() const { return EPSquare; }
	inline MoveGenerationType GetMoveGenerationType() const { return generationPhases[generationPhase]; }
	inline ValuatedMove * GetMovesOfCurrentPhase() { return &moves[phaseStartIndex]; }
	inline int GetMoveNumberInPhase() const { return moveIterationPointer; }
	inline Value GetMaterialScore() const { return material->Score; }
	inline MaterialTableEntry * GetMaterialTableEntry() const { return material; }
	inline pawn::Entry * GetPawnEntry() const { return pawn; }
	inline void InitMaterialPointer() { material = &MaterialTable[MaterialKey]; }
	inline Value PawnStructureScore() const { return pawn->Score; }
	Result GetResult();
	inline Bitboard GetAttacksFrom(Square square) const { return attacks[square]; }
	inline void SetPrevious(position &pos) { previous = &pos; }
	inline void SetPrevious(position *pos) { previous = pos; }
	inline void ResetPliesFromRoot() { pliesFromRoot = 0; }
	inline Bitboard AttacksByPieceType(Color color, PieceType pieceType) const;
	inline Bitboard AttacksByColor(Color color) const { return (SideToMove == color) * attackedByUs + (SideToMove != color) * attackedByThem; }
	bool checkRepetition();
	inline void SwitchSideToMove() { SideToMove ^= 1; Hash ^= ZobristMoveColor; }
	inline unsigned char GetDrawPlyCount() const { return DrawPlyCount; }
	void NullMove(Square epsquare = OUTSIDE);
	void deleteParents();
	inline Move GetLastAppliedMove() { return lastAppliedMove; }
	inline Piece GetPreviousMovingPiece() { if (previous) return previous->GetPieceOnSquare(to(lastAppliedMove)); else return BLANK; }
	inline Piece getCapturedInLastMove() { return capturedInLastMove; }
	inline bool IsQuiet(const Move move) const {
		return (Board[to(move)] == BLANK) && (type(move) == NORMAL || type(move) == CASTLING);
	}
	inline bool IsQuietAndNoCastles(const Move move) const {
		return type(move) == NORMAL && Board[to(move)] == BLANK;
	}
	inline bool IsTactical(const ValuatedMove move) const {
		return Board[to(move.move)] != BLANK || type(move.move) == ENPASSANT || type(move.move) == PROMOTION;
	}
	inline Value GetStaticEval() { return StaticEval; }
	inline PieceType GetMostValuablePieceType(Color col) const;
	inline bool PawnOn7thRank() { return (PieceBB(PAWN, SideToMove) & RANKS[6 - 5 * SideToMove]) != 0; } //Side to Move has pawn on 7th Rank
	void copy(const position &pos);
	inline bool CastlingAllowed(CastleFlag castling) { return (CastlingOptions & castling) != 0; }
	std::string toSan(Move move);
	Move parseSan(std::string move);
	inline bool IsAdvancedPawnMove(Move move) const { Square toSquare = to(move); return GetPieceType(Board[toSquare]) == PAWN && ((toSquare >> 5) & GetSideToMove()) == 0; };
private:
	Bitboard OccupiedByColor[2];
	Bitboard OccupiedByPieceType[6];
	uint64_t Hash = ZobristMoveColor;
	MaterialKey_t MaterialKey;
	PawnKey_t PawnKey;
	Square EPSquare;
	unsigned char CastlingOptions;
	unsigned char DrawPlyCount;
	Color SideToMove;
	int pliesFromRoot;
	Piece Board[64];

	position * previous = nullptr;
	MaterialTableEntry * material;
	pawn::Entry * pawn;
	ValuatedMove moves[MAX_MOVE_COUNT];
	int movepointer = 0;
	Bitboard attacks[64];
	Bitboard attackedByThem;
	Bitboard attackedByUs;
	Bitboard attacksByPt[12];
	int moveIterationPointer;
	int phaseStartIndex;
	int generationPhase;
	Move hashMove = MOVE_NONE;
	Result result = RESULT_UNKNOWN;
	Value StaticEval = VALUE_NOTYETDETERMINED;
	ExtendedMove *killer;
	Move lastAppliedMove = MOVE_NONE;
	Piece capturedInLastMove = BLANK;
	ValuatedMove * firstNegative;
	HistoryStats * history;
	DblHistoryStats * dblHistory;
	Move * CounterMoves = nullptr;
	Value minMoveValue = -VALUE_MATE;
	bool canPromote = false;

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
	inline int PawnStep() const { return 8 - 16 * SideToMove; }
	inline void AddMove(Move move) {
		moves[movepointer].move = move;
		moves[movepointer].score = VALUE_NOTYETDETERMINED;
		++movepointer;
	}
	inline void AddNullMove() { moves[movepointer].move = MOVE_NONE; moves[movepointer].score = VALUE_NOTYETDETERMINED; ++movepointer; }
	//inline void AddMove(Move move, Value score) { moves[movepointer].move = move; moves[movepointer].score = VALUE_NOTYETDETERMINED; ++movepointer; }
	void updateCastleFlags(Square fromSquare, Square toSquare);
	Bitboard calculateAttacks(Color color);
	Bitboard checkBlocker(Color colorOfBlocker, Color kingColor);
	MaterialKey_t calculateMaterialKey();
	PawnKey_t calculatePawnKey();
	void evaluateByCaptureScore(int startIndex = 0);
	void evaluateByMVVLVA(int startIndex = 0);
	void evaluateBySEE(int startIndex);
	void evaluateCheckEvasions(int startIndex);
	void evaluateByHistory(int startIndex);
	Move getBestMove(int startIndex);
	void insertionSort(ValuatedMove* begin, ValuatedMove* end);
	const Bitboard considerXrays(const Bitboard occ, const Square to, const Bitboard fromSet, const Square from) const;
	const Bitboard AttacksOfField(const Square targetField) const;
	const Bitboard AttacksOfField(const Square targetField, const Color attackingSide) const;
	const Bitboard getSquareOfLeastValuablePiece(const Bitboard attadef, const int side) const;
	inline bool isValid(Move move) { position next(*this); return next.ApplyMove(move); }
	bool validateMove(Move move);
	bool validateMove(ExtendedMove move);
	template<bool CHECKED> bool CheckValidMoveExists();
	bool checkMaterialIsUnusual();
};

template<> inline ValuatedMove* position::GenerateMoves<QUIET_CHECKS>();
template<> inline ValuatedMove* position::GenerateMoves<LEGAL>();

Move parseMoveInUCINotation(const std::string& uciMove, const position& pos);


inline Bitboard position::PieceBB(const PieceType pt, const Color c) const { return OccupiedByColor[c] & OccupiedByPieceType[pt]; }
inline Bitboard position::ColorBB(const Color c) const { return OccupiedByColor[c]; }
inline Bitboard position::ColorBB(const int c) const { return OccupiedByColor[c]; }
inline Bitboard position::OccupiedBB() const { return OccupiedByColor[WHITE] | OccupiedByColor[BLACK]; }
inline Bitboard position::NonPawnMaterial(const Color c) const { return OccupiedByColor[c] & ~OccupiedByPieceType[PAWN] & ~OccupiedByPieceType[KING]; }
inline Bitboard position::PieceTypeBB(const PieceType pt) const { return OccupiedByPieceType[pt]; }

inline PieceType position::GetMostValuablePieceType(Color color) const {
	for (PieceType pt = QUEEN; pt < KING; ++pt) {
		if (PieceBB(pt, color)) return pt;
	}
	return KING;
}

inline Value position::evaluate() {
	if (StaticEval != VALUE_NOTYETDETERMINED) return StaticEval = material->EvaluationFunction(*this);
	if (GetResult() == OPEN) {
		return StaticEval = material->EvaluationFunction(*this);
	}
	else if (result == DRAW) return StaticEval = VALUE_DRAW;
	else return StaticEval = Value((2 - int(result)) * (VALUE_MATE - pliesFromRoot));
}

inline Value position::evaluateFinalPosition() {
	if (result == DRAW) return VALUE_DRAW;
	else return Value((2 - int(result)) * (VALUE_MATE - pliesFromRoot));
}

//Tries to find one valid move as fast as possible
template<bool CHECKED> bool position::CheckValidMoveExists() {
	assert(attackedByThem); //should have been already calculated
	//Start with king (Castling need not be considered - as there is always another legal move available with castling
	//In Chess960 this might be different
	Square kingSquare = lsb(PieceBB(KING, SideToMove));
	Bitboard kingTargets = KingAttacks[kingSquare] & ~OccupiedByColor[SideToMove] & ~attackedByThem;
	if (CHECKED && kingTargets) {
		if (popcount(kingTargets) > 2) return true; //unfortunately 8/5p2/5kp1/8/4p3/R4n2/1r3K2/4q3 w - - shows that king can even block 2 sliders
		else {
			//unfortunately king could be "blocker" e.g. in 8/8/1R2k3/K7/8/8/8/8 w square f6 is not attacked by white
			//however if king moves to f6 it's still check
			Square to = lsb(kingTargets);
			kingTargets &= kingTargets - 1;
			if (isValid(createMove(kingSquare, to)) || (kingTargets && isValid(createMove(kingSquare, lsb(kingTargets))))) return true;
		}
	}
	else if (kingTargets) return true; //No need to check
	if (CHECKED) {
		Bitboard checker = AttacksOfField(kingSquare, Color(SideToMove ^ 1));
		if (popcount(checker) != 1) return false; //double check and no king move => MATE
		//All valid moves are now either capturing the checker or blocking the check
		Bitboard blockingSquares = checker | InBetweenFields[kingSquare][lsb(checker)];
		Bitboard pinned = checkBlocker(SideToMove, SideToMove);
		//Sliders and knights can't move if pinned (as we are in check) => therefore only check the unpinned pieces
		Bitboard sliderAndKnight = OccupiedByColor[SideToMove] & ~OccupiedByPieceType[KING] & ~OccupiedByPieceType[PAWN] & ~pinned;
		while (sliderAndKnight) {
			if (attacks[lsb(sliderAndKnight)] & ~OccupiedByColor[SideToMove] & blockingSquares) return true;
			sliderAndKnight &= sliderAndKnight - 1;
		}
		//Pawns
		Bitboard singleStepTargets;
		Bitboard pawns = PieceBB(PAWN, SideToMove) & ~pinned;
		if (SideToMove == WHITE) {
			if ((singleStepTargets = ((pawns << 8) & ~OccupiedBB())) & blockingSquares) return true;
			if (((singleStepTargets & RANK3) << 8) & ~OccupiedBB() & blockingSquares) return true;
		}
		else {
			if ((singleStepTargets = ((pawns >> 8) & ~OccupiedBB())) & blockingSquares) return true;
			if (((singleStepTargets&RANK6) >> 8) & ~OccupiedBB() & blockingSquares) return true;
		}
		//Pawn captures
		while (pawns) {
			Square from = lsb(pawns);
			if (checker & attacks[from]) return true;
			pawns &= pawns - 1;
		}
	}
	else {
		//Now we need the pinned pieces
		Bitboard pinned = checkBlocker(SideToMove, SideToMove);
		//Now first check all unpinned pieces
		Bitboard sliderAndKnight = OccupiedByColor[SideToMove] & ~OccupiedByPieceType[KING] & ~OccupiedByPieceType[PAWN] & ~pinned;
		while (sliderAndKnight) {
			if (attacks[lsb(sliderAndKnight)] & ~OccupiedByColor[SideToMove]) return true;
			sliderAndKnight &= sliderAndKnight - 1;
		}
		//Pawns
		Bitboard pawns = PieceBB(PAWN, SideToMove) & ~pinned;
		Bitboard pawnTargets;
		//normal pawn move
		if (SideToMove == WHITE) pawnTargets = (pawns << 8) & ~OccupiedBB(); else pawnTargets = (pawns >> 8) & ~OccupiedBB();
		if (pawnTargets) return true;
		//pawn capture
		if (SideToMove == WHITE) pawnTargets = (((pawns << 9) & NOT_A_FILE) | ((pawns << 7) & NOT_H_FILE)) & OccupiedByColor[SideToMove ^ 1];
		else pawnTargets = (((pawns >> 9) & NOT_H_FILE) | ((pawns >> 7) & NOT_A_FILE)) & OccupiedByColor[SideToMove ^ 1];
		if (pawnTargets) return true;
		//Now let's deal with pinned pieces
		Bitboard pinnedSlider = (PieceBB(QUEEN, SideToMove) | PieceBB(ROOK, SideToMove) | PieceBB(BISHOP, SideToMove)) & pinned;
		while (pinnedSlider) {
			Square from = lsb(pinnedSlider);
			if (attacks[from] & (InBetweenFields[from][kingSquare] | ShadowedFields[kingSquare][from])) return true;
			pinnedSlider &= pinnedSlider - 1;
		}
		//pinned knights must not move and pinned kings don't exist => remains pinned pawns
		Bitboard pinnedPawns = PieceBB(PAWN, SideToMove) & pinned;
		Bitboard pinnedPawnsAllowedToMove = pinnedPawns & FILES[kingSquare & 7];
		if (pinnedPawnsAllowedToMove) {
			if (SideToMove == WHITE && ((pinnedPawnsAllowedToMove << 8) & ~OccupiedBB())) return true;
			else if (SideToMove == BLACK && ((pinnedPawnsAllowedToMove >> 8) & ~OccupiedBB())) return true;
		}
		//Now there remains only pinned pawn captures
		while (pinnedPawns) {
			Square from = lsb(pinnedPawns);
			pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from];
			while (pawnTargets) {
				if (isolateLSB(pawnTargets) & (InBetweenFields[from][kingSquare] | ShadowedFields[kingSquare][from])) return true;
				pawnTargets &= pawnTargets - 1;
			}
			pinnedPawns &= pinnedPawns - 1;
		}
	}
	//ep-captures are difficult as 3 squares are involved => therefore simply apply and check it
	Bitboard epAttacker;
	if (EPSquare != OUTSIDE && (epAttacker = (GetEPAttackersForToField(EPSquare - PawnStep()) & PieceBB(PAWN, SideToMove)))) {
		while (epAttacker) {
			if (isValid(createMove<ENPASSANT>(lsb(epAttacker), EPSquare))) return true;
			epAttacker &= epAttacker - 1;
		}
	}
	return false;
}


// Generates all legal moves (by first generating all pseudo-legal moves and then eliminating all invalid moves
//Should only be used at the root as implementation is slow!
template<> ValuatedMove* position::GenerateMoves<LEGAL>() {
	GenerateMoves<ALL>();
	for (int i = 0; i < movepointer - 1; ++i) {
		if (!isValid(moves[i].move)) {
			moves[i] = moves[movepointer - 2];
			--movepointer;
			--i;
		}
	}
	return &moves[0];
}

//Generates all quiet moves giving check
template<> ValuatedMove* position::GenerateMoves<QUIET_CHECKS>() {
	movepointer -= (movepointer != 0);
	ValuatedMove * result = &moves[movepointer];
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
			&& (RookTargets(opposedKingSquare, ~targets & ~PieceBB(KING, SideToMove)) & RookSquareAfterCastling[2 * SideToMove])) //Rook is giving check after castling
		{
			if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove])); else AddMove(createMove<CASTLING>(kingSquare, Square(G1 + SideToMove * 56)));
		}
		//Queen-side castles
		if ((CastlingOptions & (1 << (2 * SideToMove + 1))) //Short castle allowed
			&& (InitialRookSquareBB[2 * SideToMove + 1] & PieceBB(ROOK, SideToMove)) //Rook on initial square
			&& !(SquaresToBeEmpty[2 * SideToMove + 1] & OccupiedBB()) //Fields between Rook and King are empty
			&& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
			&& !(SquaresToBeUnattacked[2 * SideToMove + 1] & attackedByThem) //Fields passed by the king are unattacked
			&& (RookTargets(opposedKingSquare, ~targets & ~PieceBB(KING, SideToMove)) & RookSquareAfterCastling[2 * SideToMove + 1])) //Rook is giving check after castling
		{
			if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove + 1])); else AddMove(createMove<CASTLING>(kingSquare, Square(C1 + SideToMove * 56)));
		}
	}
	AddNullMove();
	return result;
}

template<MoveGenerationType MGT> ValuatedMove * position::GenerateMoves() {
	movepointer -= (movepointer != 0);
	ValuatedMove * result = &moves[movepointer];
	//Rooksliders
	Bitboard targets;
	if (MGT == ALL || MGT == TACTICAL || MGT == QUIETS || MGT == CHECK_EVASION) {
		Square kingSquare = lsb(PieceBB(KING, SideToMove));
		Bitboard checkBlocker = 0;
		bool doubleCheck = false;
		if (MGT == CHECK_EVASION) {
			Bitboard checker = AttacksOfField(kingSquare, Color(SideToMove ^ 1));
			if (popcount(checker) == 1) {
				checkBlocker = checker | InBetweenFields[kingSquare][lsb(checker)];
			}
			else doubleCheck = true;
		} 
		Bitboard empty = ~ColorBB(BLACK) & ~ColorBB(WHITE);
		if (MGT == ALL) targets = ~ColorBB(SideToMove);
		else if (MGT == TACTICAL) targets = ColorBB(SideToMove ^ 1);
		else if (MGT == QUIETS) targets = empty;
		else if (MGT == CHECK_EVASION && !doubleCheck) targets = ~ColorBB(SideToMove) & checkBlocker;
		else targets = 0;
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
				{
					if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove]));
					else AddMove(createMove<CASTLING>(kingSquare, Square(G1 + SideToMove * 56)));
				}
				//Queen-side castles
				if ((CastlingOptions & (1 << (2 * SideToMove + 1))) //Short castle allowed
					&& (InitialRookSquareBB[2 * SideToMove + 1] & PieceBB(ROOK, SideToMove)) //Rook on initial square
					&& !(SquaresToBeEmpty[2 * SideToMove + 1] & OccupiedBB()) //Fields between Rook and King are empty
					&& (attackedByThem || (attackedByThem = calculateAttacks(Color(SideToMove ^ 1))))
					&& !(SquaresToBeUnattacked[2 * SideToMove + 1] & attackedByThem)) //Fields passed by the king are unattacked
				{
					if (Chess960) AddMove(createMove<CASTLING>(kingSquare, InitialRookSquare[2 * SideToMove + 1]));
					else AddMove(createMove<CASTLING>(kingSquare, Square(C1 + SideToMove * 56)));
				}
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
						//AddMove(createMove<PROMOTION>(from, to, ROOK));
						//AddMove(createMove<PROMOTION>(from, to, BISHOP));
						//AddMove(createMove<PROMOTION>(from, to, KNIGHT));
						canPromote = true;
						pawnTargets &= pawnTargets - 1;
					}
					pawns &= pawns - 1;
				}
			}
			Bitboard singleStepTarget = 0;
			Bitboard doubleSteptarget = 0;
			Bitboard promotionTarget = 0;
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
					//AddMove(createMove<PROMOTION>(from, to, ROOK));
					//AddMove(createMove<PROMOTION>(from, to, BISHOP));
					//AddMove(createMove<PROMOTION>(from, to, KNIGHT));
					canPromote = true;
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
		Bitboard sliders;
		if (MGT == EQUAL_CAPTURES || MGT == LOOSING_CAPTURES || MGT == NON_LOOSING_CAPTURES) { //Queen Captures are never winning
			sliders = PieceBB(QUEEN, SideToMove);
			if (MGT == EQUAL_CAPTURES || MGT == NON_LOOSING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1));
			else if (MGT == LOOSING_CAPTURES) targets = PieceBB(ROOK, Color(SideToMove ^ 1)) | PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(PAWN, Color(SideToMove ^ 1));
			else targets = 0;
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
		else if (MGT == NON_LOOSING_CAPTURES) targets = PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1));
		else targets = 0;
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
		else if (MGT == NON_LOOSING_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1));
		else targets = 0;
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
		else if (MGT == NON_LOOSING_CAPTURES) targets = PieceBB(BISHOP, Color(SideToMove ^ 1)) | PieceBB(KNIGHT, Color(SideToMove ^ 1)) | PieceBB(QUEEN, Color(SideToMove ^ 1)) | PieceBB(ROOK, Color(SideToMove ^ 1));
		else targets = 0;
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
					//AddMove(createMove<PROMOTION>(from, to, ROOK));
					//AddMove(createMove<PROMOTION>(from, to, BISHOP));
					//AddMove(createMove<PROMOTION>(from, to, KNIGHT));
					canPromote = true;
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
		else if (MGT == NON_LOOSING_CAPTURES) {
			Bitboard pawns = PieceBB(PAWN, SideToMove);
			while (pawns) {
				Square from = lsb(pawns);
				Bitboard pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & ~RANK1and8;
				while (pawnTargets) {
					AddMove(createMove(from, lsb(pawnTargets)));
					pawnTargets &= pawnTargets - 1;
				}
				//Promotion captures
				pawnTargets = ColorBB(SideToMove ^ 1) & attacks[from] & RANK1and8;
				while (pawnTargets) {
					Square to = lsb(pawnTargets);
					AddMove(createMove<PROMOTION>(from, to, QUEEN));
					//AddMove(createMove<PROMOTION>(from, to, ROOK));
					//AddMove(createMove<PROMOTION>(from, to, BISHOP));
					//AddMove(createMove<PROMOTION>(from, to, KNIGHT));
					canPromote = true;
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
		if (MGT == WINNING_CAPTURES || MGT == NON_LOOSING_CAPTURES) {
			Bitboard promotionTarget;
			if (SideToMove == WHITE)  promotionTarget = (PieceBB(PAWN, WHITE) << 8) & ~OccupiedBB() & RANK8; else promotionTarget = (PieceBB(PAWN, BLACK) >> 8) & ~OccupiedBB() & RANK1;
			while (promotionTarget) {
				Square to = lsb(promotionTarget);
				Square from = Square(to - PawnStep());
				AddMove(createMove<PROMOTION>(from, to, QUEEN));
				//AddMove(createMove<PROMOTION>(from, to, ROOK));
				//AddMove(createMove<PROMOTION>(from, to, BISHOP));
				//AddMove(createMove<PROMOTION>(from, to, KNIGHT));
				canPromote = true;
				promotionTarget &= promotionTarget - 1;
			}
			//King Captures are always winning as kings can only capture uncovered pieces
			Square kingSquare = lsb(PieceBB(KING, SideToMove));
			Bitboard kingTargets = KingAttacks[kingSquare] & ColorBB(SideToMove ^ 1) & ~attackedByThem;
			while (kingTargets) {
				AddMove(createMove(kingSquare, lsb(kingTargets)));
				kingTargets &= kingTargets - 1;
			}
		}
	}
	AddNullMove();
	return result;
}


template<StagedMoveGenerationType SMGT> void position::InitializeMoveIterator(HistoryStats * historyStats, DblHistoryStats * dblHistoryStats, ExtendedMove* killerMove, Move * counterMoves, Move hashmove, Value limit) {
	if (SMGT == REPETITION) {
		moveIterationPointer = 0;
		generationPhase = generationPhaseOffset[SMGT];
		return;
	}
	if (SMGT == ALL_MOVES) {
		moveIterationPointer = -1;
		generationPhase = generationPhaseOffset[SMGT];
		return;
	}
	if (SMGT == MAIN_SEARCH) killer = killerMove; else killer = nullptr;
	CounterMoves = counterMoves;
	if (!attackedByThem) attackedByThem = calculateAttacks(Color(SideToMove ^ 1));
	moveIterationPointer = -1;
	movepointer = 0;
	phaseStartIndex = 0;
	history = historyStats;
	dblHistory = dblHistoryStats;
	hashmove ? hashMove = hashmove : hashMove = MOVE_NONE;
	if (IsCheck()) generationPhase = generationPhaseOffset[CHECK] + (hashMove == MOVE_NONE);
	else generationPhase = generationPhaseOffset[SMGT] + (hashMove == MOVE_NONE);
}

inline Bitboard position::AttacksByPieceType(Color color, PieceType pieceType) const {
	return attacksByPt[GetPiece(pieceType, color)];
}


