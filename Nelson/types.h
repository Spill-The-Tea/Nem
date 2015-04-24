#pragma once
#include <assert.h>
#include <string>
#include <algorithm>
#ifdef _MSC_VER
#include <intrin.h>
#define NOMINMAX
#include <Windows.h>
#endif
#include <inttypes.h>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

#define CACHE_LINE 64
#ifdef _MSC_VER
#define CACHE_ALIGN __declspec(align(CACHE_LINE))
#endif

typedef uint64_t Bitboard;

/* Move Encoding as unsigned short (copied from Stockfish)
* from                  = bits 0-5
* to                    = bits 6-11
* conversion piece type = bits 12-13
* type                  = bits 14-15
*/
typedef uint16_t Move;

const Move MOVE_NONE = 0;

struct position;

enum MoveType {
	NORMAL, PROMOTION = 1 << 14, ENPASSANT = 2 << 14, CASTLING = 3 << 14
};

enum Color : unsigned char {
	WHITE, BLACK
};

enum Piece : unsigned char {
	WQUEEN, BQUEEN, WROOK, BROOK, WBISHOP, BBISHOP, WKNIGHT, BKNIGHT, WPAWN, BPAWN, WKING, BKING, BLANK
};

enum PieceType : unsigned char {
	QUEEN, ROOK, BISHOP, KNIGHT, PAWN, KING, NO_TYPE
};

inline PieceType GetPieceType(Piece piece) { return PieceType(piece >> 1); }
inline Piece GetPiece(PieceType pieceType, Color color) { return Piece(2 * pieceType + color); }
inline Color GetColor(Piece piece) { return Color(piece & 1); }

enum Square : unsigned char {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	OUTSIDE
};

inline Bitboard operator&(Bitboard b, Square s) {
	return b & (1ull << s);
}

inline Color operator~(Color c) { return Color(c ^ 1); }

enum Rank {
	Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8
};
enum File {
	FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH
};

enum CastleFlag {
	NoCastles, W0_0, W0_0_0, B0_0 = 4, B0_0_0 = 8
};

const int CastlesbyColor[] = { W0_0 | W0_0_0, B0_0 | B0_0_0 };

enum MoveGenerationType {
	WINNING_CAPTURES, EQUAL_CAPTURES, LOOSING_CAPTURES, TACTICAL, QUIETS, CHECK_EVASION, QUIET_CHECKS, ALL, LEGAL, FIND_ANY, FIND_ANY_CHECKED, HASHMOVE, KILLER, QUIETS_POSITIVE, QUIETS_NEGATIVE, NONE, REPEAT_ALL, UNDERPROMOTION
};

enum StagedMoveGenerationType {
	MAIN_SEARCH, QSEARCH, CHECK, QSEARCH_WITH_CHECKS, REPETITION
};

enum Result { RESULT_UNKNOWN, OPEN, DRAW, MATE };

enum Value : int16_t {
	VALUE_NOTYETDETERMINED = 0 - 32767,
	VALUE_MIN = SHRT_MIN + 2,
	VALUE_DRAW = 0,
	VALUE_ZERO = 0,
	VALUE_KNOWN_WIN = 10000,
	VALUE_MATE = 32000,
	VALUE_INFINITE = 32001,
	VALUE_MATE_THRESHOLD = 31000,
	VALUE_MAX = SHRT_MAX - 2
};

inline Value operator*(const float f, const Value v) { return Value(int(round(int(v) * f))); }

typedef uint32_t MaterialKey_t;
typedef uint64_t PawnKey_t;
typedef uint16_t Phase_t;

#define ENABLE_BASE_OPERATORS_ON(T)                                             \
	inline T operator+(const T d1, const T d2) { return T(int(d1) + int(d2)); } \
	inline T operator-(const T d1, const T d2) { return T(int(d1) - int(d2)); } \
	inline T operator*(int i, const T d) { return T(i * int(d)); }              \
	inline T operator*(const T d, int i) { return T(int(d) * i); }              \
	inline T operator-(const T d) { return T(-int(d)); }                        \
	inline T& operator+=(T& d1, const T d2) { return d1 = d1 + d2; }            \
	inline T& operator-=(T& d1, const T d2) { return d1 = d1 - d2; }            \
	inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }

#define ENABLE_FULL_OPERATORS_ON(T)                                             \
	ENABLE_BASE_OPERATORS_ON(T)                                                 \
	inline T& operator++(T& d) { return d = T(int(d) + 1); }                    \
	inline T& operator--(T& d) { return d = T(int(d) - 1); }                    \
	inline T operator/(const T d, int i) { return T(int(d) / i); }              \
	inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Rank)
ENABLE_FULL_OPERATORS_ON(File)
ENABLE_FULL_OPERATORS_ON(Square);
ENABLE_FULL_OPERATORS_ON(Value);
ENABLE_FULL_OPERATORS_ON(PieceType);
ENABLE_FULL_OPERATORS_ON(Piece);

inline Value operator-(Value v, int i) { return Value(int(v) - i); }

inline Color& operator^=(Color& col, int i) { return col = Color(((int)col) ^ 1); }

inline Square createSquare(Rank rank, File file) { return Square((rank << 3) + file); }
inline char toChar(File f, bool tolower = true) { return char(f - FileA + 'a'); }
inline char toChar(Rank r) { return char(r - Rank1 + '1'); }

inline std::string toString(Square square) {
	char ch[] = { toChar(File(square & 7)), toChar(Rank(square >> 3)), 0 };
	return ch;
}

template<MoveType T>
inline Move createMove(Square from, Square to,
	PieceType pieceType = QUEEN) {
	return Move(to | (from << 6) | T | ((pieceType - QUEEN) << 12));
}

inline Move createMove(Square from, Square to) { return Move(to | (from << 6)); }
inline Move createMove(int from, int to) { return Move(to | (from << 6)); }

inline Square from(Move move) { return Square((move >> 6) & 0x3F); }

inline Square to(Move move) { return Square(move & 0x3F); }

inline MoveType type(Move move) { return MoveType(move & (3 << 14)); }

inline PieceType promotionType(Move move) { return PieceType((move >> 12) & 3); }

const uint64_t EPAttackersForToField[] = { 0x2000000ull,
0x5000000ull, 0xa000000ull, 0x14000000ull, 0x28000000ull,
0x50000000ull, 0xa0000000ull, 0x40000000ull, 0x200000000ull,
0x500000000ull, 0xa00000000ull, 0x1400000000ull, 0x2800000000ull,
0x5000000000ull, 0xa000000000ull, 0x4000000000ull };

inline uint64_t GetEPAttackersForToField(Square to) { return EPAttackersForToField[to - A4]; }
inline uint64_t GetEPAttackersForToField(int to) { return EPAttackersForToField[to - A4]; }

#ifdef _MSC_VER
inline int popcount(Bitboard bb) { return (int)_mm_popcnt_u64(bb); }

inline Square lsb(Bitboard bb) {
	unsigned long  index;
	_BitScanForward64(&index, bb);
#pragma warning(suppress: 6102)
	return Square(index);
}

inline int msb(int n) {
	unsigned long result;
	_BitScanReverse64(&result, n);
#pragma warning(suppress: 6102)
	return result;
}

#endif

#ifdef __GNUC__
inline int popcount(Bitboard bb) { return __builtin_popcountll(bb); }

inline Square lsb(Bitboard b) {  return Square(__builtin_ctzll(b)); }

inline Square msb(int n) { return Square(__builtin_clzll(n)); }
//#define offsetof(type, member)  __builtin_offsetof (type, member)
#endif // __GNUC__

inline Bitboard isolateLSB(Bitboard bb) { return bb & (0 - bb); }

//inline Square lsb(Bitboard bb) { return Square(popcount((bb & (0 - bb)) - 1)); }

inline Bitboard ToBitboard(Square square) { return 1ull << square; }
inline Bitboard ToBitboard(int square) { return 1ull << square; }

struct eval {

	Value mgScore = Value(0);
	Value egScore = Value(0);

	eval() {

	}

	eval(Value mgValue, Value egValue) {
		mgScore = mgValue;
		egScore = egValue;
	}

	eval(int mgValue, int egValue) {
		mgScore = Value(mgValue);
		egScore = Value(egValue);
	}

	eval(int value) {
		mgScore = Value(value);
		egScore = Value(value);
	}

	eval(Value value) {
		mgScore = value;
		egScore = value;
	}

	inline Value getScore(Phase_t phase) {
		return Value(((((int)mgScore) * (256 - phase)) + (phase * (int)egScore)) / 256);
	}
};

const eval EVAL_ZERO;

inline eval operator-(const eval e) { return eval(-e.mgScore, -e.egScore); }
inline eval operator+(const eval e1, const eval e2) { return eval(e1.mgScore + e2.mgScore, e1.egScore + e2.egScore); }
inline eval operator+=(eval& e1, const eval e2) {
	e1.mgScore += e2.mgScore;
	e1.egScore += e2.egScore;
	return e1;
}

inline eval operator+=(eval& e1, const Value v) {
	e1.mgScore += v;
	e1.egScore += v;
	return e1;
}

inline eval operator-=(eval& e1, const eval e2) {
	e1.mgScore -= e2.mgScore;
	e1.egScore -= e2.egScore;
	return e1;
}

inline eval operator-=(eval& e1, const Value v) {
	e1.mgScore -= v;
	e1.egScore -= v;
	return e1;
}

inline eval operator-(const eval& e1, const eval e2) {
	return eval(e1.mgScore - e2.mgScore, e1.egScore - e2.egScore);
}
inline eval operator*(const eval e, const int i) { return eval(e.mgScore * i, e.egScore * i); }
inline eval operator*(const int i, const eval e) { return eval(e.mgScore * i, e.egScore * i); }
inline eval operator/(const eval e, const int i) { return eval(e.mgScore / i, e.egScore / i); }
inline eval operator*(const float f, const eval e) { return eval(f * e.mgScore, f * e.egScore); }

struct ValuatedMove {
	Move move;
	Value score;
};

struct ExtendedMove {
	Piece piece = BLANK;
	Move move = MOVE_NONE;

	ExtendedMove() { }
	ExtendedMove(Piece p, Move m) { piece = p; move = m; }
};

const ExtendedMove EXTENDED_MOVE_NONE(BLANK, MOVE_NONE);

inline bool operator==(const ExtendedMove& em1, const ExtendedMove& em2) { return em1.move == em2.move && em1.piece == em2.piece; }

inline bool positiveScore(const ValuatedMove& vm) { return vm.score > 0; }

inline bool operator<(const ValuatedMove& f, const ValuatedMove& s) {
	return f.score < s.score;
}

inline bool sortByScore(const ValuatedMove m1, const ValuatedMove m2) { return m1.score > m2.score; }

struct SearchStopCriteria {
	int64_t MaxNumberOfNodes = INT64_MAX;
	int MaxDepth = 128;
	int64_t StartTime = 0;
	int64_t HardStopTime = INT64_MAX;
	int64_t SoftStopTime = INT64_MAX;
};


struct position;

#ifdef _MSC_VER
inline int64_t now() {
	LARGE_INTEGER s_frequency;
	QueryPerformanceFrequency(&s_frequency);
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (1000LL * now.QuadPart) / s_frequency.QuadPart;
}
#else
inline int64_t now() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	//return GetTickCount64();
}
#endif

const Value MAX_HISTORY_VALUE = Value(2000);
struct HistoryStats {
public:
	inline void update(Value v, Piece p, Square s) {
		if (abs(int(Table[p][s])) < MAX_HISTORY_VALUE) {
			Table[p][s] += v;
		}
	}
	inline Value const getValue(const Piece p, const Square s) { return Table[p][s]; }
	inline void initialize() {
		for (int p = 0; p < 12; ++p) {
			for (int s = 0; s < 64; ++s) Table[p][s] = VALUE_ZERO;
		}
	}
private:
	Value Table[12][64];
};

struct DblHistoryStats {
public:
	inline void update(Value v, Piece p1, Square s1, Piece p2, Square s2) {
		if (abs(int(Table[p1][s1][p2][s2])) < MAX_HISTORY_VALUE) {
			Table[p1][s1][p2][s2] += v;
		}
	}
	inline Value const getValue(const Piece p1, const Square s1, const Piece p2, const Square s2) { return Table[p1][s1][p2][s2]; }
	inline void initialize() { std::memset(Table, 0, sizeof(Table)); }
private:
	Value Table[12][64][12][64];
};
