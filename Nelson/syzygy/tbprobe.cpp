/*
  Copyright (c) 2013 Ronald de Man
  This file may be redistributed and/or modified without restrictions.

  tbprobe.cpp contains the Stockfish-specific routines of the
  tablebase probing code. It should be relatively easy to adapt
  this code to other chess engines.
*/

/*
The probing code version here is copied from Stockfish and then adapted
*/

#define NOMINMAX

#include <algorithm>

#include "../position.h"

#include "tbprobe.h"
#include "tbcore.h"

#include "tbcore.cpp"

int Tablebases::MaxCardinality = 0;

// Given a position with 6 or fewer pieces, produce a text string
// of the form KQPvKRP, where "KQP" represents the white pieces if
// mirror == 0 and the black pieces if mirror == 1.
static void prt_str(position& pos, char *str, int mirror)
{
	Color color;
	PieceType pt;
	int i;

	color = !mirror ? WHITE : BLACK;
	*str++ = 'K';
	for (pt = QUEEN; pt <= PAWN; pt = PieceType(pt + 1))
		for (i = popcount(pos.PieceBB(pt, color)); i > 0; i--) *str++ = "QRBNP"[pt];
	*str++ = 'v';
	*str++ = 'K';
	color = Color(color ^ 1);
	for (pt = QUEEN; pt <= PAWN; pt = PieceType(pt + 1))
		for (i = popcount(pos.PieceBB(pt, color)); i > 0; i--) *str++ = "QRBNP"[pt];
	*str++ = 0;
}
//
//enum PieceType {
//	NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
//	ALL_PIECES = 0,
//	PIECE_TYPE_NB = 8
//};

//enum Piece {
//	NO_PIECE,
//	W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
//	B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
//	PIECE_NB = 16
//};

const int TBPieceTypeIndexedByPieceType[7] = { 5, 4, 3, 2, 1, 6, 0 };
const PieceType PieceTypeIndexedByTBPieceType[8] = { NO_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_TYPE };
const int TBPieceIndexedByPiece[16] = { 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 6, 12, 0, 0, 0, 0 };
const Piece PieceIndexedByTBPiece[16] = { BLANK, WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING, BLANK, BLANK, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BLANK };

inline int MapPieceTypeToTB(PieceType pt) { return TBPieceTypeIndexedByPieceType[pt]; }
inline PieceType MapPieceTypeFromTB(int tb_pt) { return PieceTypeIndexedByTBPieceType[tb_pt]; }
inline int MapPieceToTB(Piece p) { return TBPieceIndexedByPiece[p]; }
inline Piece MapPieceFromTB(int tb_p) { return PieceIndexedByTBPiece[tb_p]; }

inline Bitboard pieces(Color col, int tb_pieceType, position & pos) { return pos.PieceBB(MapPieceTypeFromTB(tb_pieceType), col); }

// Given a position, produce a 64-bit material signature key.
// If the engine supports such a key, it should equal the engine's key.
// However there are only the highest 10 bits of the key used for hashing
// so the engine's key can't be used (as it is limited to 18 bits)
// even shifting the engine key doesn't help much, as my key is a key and not
// a hash => So a zobrist key is used
static uint64 calc_key(position& pos, int mirror)
{
	//if (mirror) {
	//	int pCounts[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	//	for (int i = 0; i < 5; ++i) {
	//		pCounts[2*i] = popcount(pos.PieceBB(PieceType(i), BLACK));
	//		pCounts[2*i+1] = popcount(pos.PieceBB(PieceType(i), WHITE));			
	//	}
	//	return uint64_t(calculateMaterialKey(pCounts)) << 46;
	//} else return uint64_t(pos.GetMaterialKey()) << 46;
	uint64_t key = 0;
	if (mirror) {
		for (int i = 0; i < 5; ++i) {
			key ^= ZobristKeys[2 * i][popcount(pos.PieceBB(PieceType(i), BLACK))];
			key ^= ZobristKeys[2 * i+1][popcount(pos.PieceBB(PieceType(i), WHITE))];
		}
	}
	else {
		for (int i = 0; i < 5; ++i) {
			key ^= ZobristKeys[2 * i][popcount(pos.PieceBB(PieceType(i), WHITE))];
			key ^= ZobristKeys[2 * i + 1][popcount(pos.PieceBB(PieceType(i), BLACK))];
		}
	}
	return key;
}

// Produce a 64-bit material key corresponding to the material combination
// defined by pcs[16], where pcs[1], ..., pcs[6] is the number of white
// pawns, ..., kings and pcs[9], ..., pcs[14] is the number of black
// pawns, ..., kings.
static uint64 calc_key_from_pcs(int *pcs, int mirror)
{
	//int pCounts[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	//if (mirror) {
	//	for (int i = 1; i <= 5; ++i) pCounts[MapPieceFromTB(i+8)] = pcs[i];
	//	for (int i = 9; i <= 13; ++i) pCounts[MapPieceFromTB(i-8)] = pcs[i];
	//}
	//else {
	//	for (int i = 1; i <= 5; ++i) pCounts[MapPieceFromTB(i)] = pcs[i];
	//	for (int i = 9; i <= 13; ++i) pCounts[MapPieceFromTB(i)] = pcs[i];
	//}
	//return uint64_t(calculateMaterialKey(pCounts)) << 46;
	uint64_t key = 0;
	if (mirror) {
		for (int i = 1; i <= 5; ++i) key ^= ZobristKeys[MapPieceFromTB(i + 8)][pcs[i]];
		for (int i = 9; i <= 13; ++i) key ^= ZobristKeys[MapPieceFromTB(i - 8)][pcs[i]];
	}
	else {
		for (int i = 1; i <= 5; ++i) key ^= ZobristKeys[MapPieceFromTB(i)][pcs[i]];
		for (int i = 9; i <= 13; ++i) key ^= ZobristKeys[MapPieceFromTB(i)][pcs[i]];
	}
	return key;
}

bool is_little_endian() {
	union {
		int i;
		char c[sizeof(int)];
	} x;
	x.i = 1;
	return x.c[0] == 1;
}

static ubyte decompress_pairs(struct PairsData *d, uint64 idx)
{
	static const bool isLittleEndian = is_little_endian();
	return isLittleEndian ? decompress_pairs<true >(d, idx)
		: decompress_pairs<false>(d, idx);
}

// probe_wdl_table and probe_dtz_table require similar adaptations.
static int probe_wdl_table(position& pos, int *success)
{
	struct TBEntry *ptr;
	struct TBHashEntry *ptr2;
	uint64 idx;
	uint64 key;
	int i;
	ubyte res;
	int p[TBPIECES];

	// Obtain the position's material signature key.
	key = calc_key(pos, 0);

	// Test for KvK.
	if (key == 0) return 0;

	ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
	for (i = 0; i < HSHMAX; i++)
		if (ptr2[i].key == key) break;
	if (i == HSHMAX) {
		*success = 0;
		return 0;
	}

	ptr = ptr2[i].ptr;
	if (!ptr->ready) {
		LOCK(TB_mutex);
		if (!ptr->ready) {
			char str[16];
			prt_str(pos, str, ptr->key != key);
			if (!init_table_wdl(ptr, str)) {
				ptr2[i].key = 0ULL;
				*success = 0;
				UNLOCK(TB_mutex);
				return 0;
			}
			// Memory barrier to ensure ptr->ready = 1 is not reordered.
#ifdef _MSC_VER
			_ReadWriteBarrier();
#else
			__asm__ __volatile__("" ::: "memory");
#endif
			ptr->ready = 1;
		}
		UNLOCK(TB_mutex);
	}

	int bside, mirror, cmirror;
	if (!ptr->symmetric) {
		if (key != ptr->key) {
			cmirror = 8;
			mirror = 0x38;
			bside = (pos.GetSideToMove() == WHITE);
		}
		else {
			cmirror = mirror = 0;
			bside = !(pos.GetSideToMove() == WHITE);
		}
	}
	else {
		cmirror = pos.GetSideToMove() == WHITE ? 0 : 8;
		mirror = pos.GetSideToMove() == WHITE ? 0 : 0x38;
		bside = 0;
	}

	// p[i] is to contain the square 0-63 (A1-H8) for a piece of type
	// pc[i] ^ cmirror, where 1 = white pawn, ..., 14 = black king.
	// Pieces of the same type are guaranteed to be consecutive.
	if (!ptr->has_pawns) {
		struct TBEntry_piece *entry = (struct TBEntry_piece *)ptr;
		ubyte *pc = entry->pieces[bside];
		for (i = 0; i < entry->num;) {
			Bitboard bb = pieces((Color)((pc[i] ^ cmirror) >> 3),
				(PieceType)(pc[i] & 0x07), pos);
			do {
				p[i++] = pop_lsb(&bb);
			} while (bb);
		}
		idx = encode_piece(entry, entry->norm[bside], p, entry->factor[bside]);
		res = decompress_pairs(entry->precomp[bside], idx);
	}
	else {
		struct TBEntry_pawn *entry = (struct TBEntry_pawn *)ptr;
		int k = entry->file[0].pieces[0][0] ^ cmirror;
		Bitboard bb = pieces((Color)(k >> 3), (PieceType)(k & 0x07), pos);
		i = 0;
		do {
			p[i++] = pop_lsb(&bb) ^ mirror;
		} while (bb);
		int f = pawn_file(entry, p);
		ubyte *pc = entry->file[f].pieces[bside];
		for (; i < entry->num;) {
			bb = pieces((Color)((pc[i] ^ cmirror) >> 3),
				(PieceType)(pc[i] & 0x07), pos);
			do {
				p[i++] = pop_lsb(&bb) ^ mirror;
			} while (bb);
		}
		idx = encode_pawn(entry, entry->file[f].norm[bside], p, entry->file[f].factor[bside]);
		res = decompress_pairs(entry->file[f].precomp[bside], idx);
	}

	return ((int)res) - 2;
}

static int probe_dtz_table(position& pos, int wdl, int *success)
{
	struct TBEntry *ptr;
	uint64 idx;
	int i, res;
	int p[TBPIECES];

	// Obtain the position's material signature key.
	uint64 key = calc_key(pos, 0);

	if (DTZ_table[0].key1 != key && DTZ_table[0].key2 != key) {
		for (i = 1; i < DTZ_ENTRIES; i++)
			if (DTZ_table[i].key1 == key) break;
		if (i < DTZ_ENTRIES) {
			struct DTZTableEntry table_entry = DTZ_table[i];
			for (; i > 0; i--)
				DTZ_table[i] = DTZ_table[i - 1];
			DTZ_table[0] = table_entry;
		}
		else {
			struct TBHashEntry *ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
			for (i = 0; i < HSHMAX; i++)
				if (ptr2[i].key == key) break;
			if (i == HSHMAX) {
				*success = 0;
				return 0;
			}
			ptr = ptr2[i].ptr;
			char str[16];
			int mirror = (ptr->key != key);
			prt_str(pos, str, mirror);
			if (DTZ_table[DTZ_ENTRIES - 1].entry)
				free_dtz_entry(DTZ_table[DTZ_ENTRIES - 1].entry);
			for (i = DTZ_ENTRIES - 1; i > 0; i--)
				DTZ_table[i] = DTZ_table[i - 1];
			load_dtz_table(str, calc_key(pos, mirror), calc_key(pos, !mirror));
		}
	}

	ptr = DTZ_table[0].entry;
	if (!ptr) {
		*success = 0;
		return 0;
	}

	int bside, mirror, cmirror;
	if (!ptr->symmetric) {
		if (key != ptr->key) {
			cmirror = 8;
			mirror = 0x38;
			bside = (pos.GetSideToMove() == WHITE);
		}
		else {
			cmirror = mirror = 0;
			bside = !(pos.GetSideToMove() == WHITE);
		}
	}
	else {
		cmirror = pos.GetSideToMove() == WHITE ? 0 : 8;
		mirror = pos.GetSideToMove() == WHITE ? 0 : 0x38;
		bside = 0;
	}

	if (!ptr->has_pawns) {
		struct DTZEntry_piece *entry = (struct DTZEntry_piece *)ptr;
		if ((entry->flags & 1) != bside && !entry->symmetric) {
			*success = -1;
			return 0;
		}
		ubyte *pc = entry->pieces;
		for (i = 0; i < entry->num;) {
			Bitboard bb = pieces((Color)((pc[i] ^ cmirror) >> 3),
				(PieceType)(pc[i] & 0x07), pos);
			do {
				p[i++] = pop_lsb(&bb);
			} while (bb);
		}
		idx = encode_piece((struct TBEntry_piece *)entry, entry->norm, p, entry->factor);
		res = decompress_pairs(entry->precomp, idx);

		if (entry->flags & 2)
			res = entry->map[entry->map_idx[wdl_to_map[wdl + 2]] + res];

		if (!(entry->flags & pa_flags[wdl + 2]) || (wdl & 1))
			res *= 2;
	}
	else {
		struct DTZEntry_pawn *entry = (struct DTZEntry_pawn *)ptr;
		int k = entry->file[0].pieces[0] ^ cmirror;
		Bitboard bb = pieces((Color)(k >> 3), (PieceType)(k & 0x07), pos);
		i = 0;
		do {
			p[i++] = pop_lsb(&bb) ^ mirror;
		} while (bb);
		int f = pawn_file((struct TBEntry_pawn *)entry, p);
		if ((entry->flags[f] & 1) != bside) {
			*success = -1;
			return 0;
		}
		ubyte *pc = entry->file[f].pieces;
		for (; i < entry->num;) {
			bb = pieces((Color)((pc[i] ^ cmirror) >> 3),
				(PieceType)(pc[i] & 0x07), pos);
			do {
				p[i++] = pop_lsb(&bb) ^ mirror;
			} while (bb);
		}
		idx = encode_pawn((struct TBEntry_pawn *)entry, entry->file[f].norm, p, entry->file[f].factor);
		res = decompress_pairs(entry->file[f].precomp, idx);

		if (entry->flags[f] & 2)
			res = entry->map[entry->map_idx[f][wdl_to_map[wdl + 2]] + res];

		if (!(entry->flags[f] & pa_flags[wdl + 2]) || (wdl & 1))
			res *= 2;
	}

	return res;
}


static int probe_ab(position& pos, int alpha, int beta, int *success)
{
	int v;
	if (pos.Checked())  pos.GenerateMoves<CHECK_EVASION>(); 
	else {
		pos.ResetMoveGeneration();
		pos.GenerateMoves<TACTICAL>();
	}
	pos.AddUnderPromotions();
	int moveCount;
	ValuatedMove * moves = pos.GetMoves(moveCount);
	for (int i = 0; i < moveCount; ++i) {
		Move capture = moves->move;
		moves++;
		if (pos.GetPieceOnSquare(to(capture)) == BLANK || type(capture) == ENPASSANT || type(capture) == CASTLING) continue;
		position next(pos);
		if (!next.ApplyMove(capture)) continue;
		v = -probe_ab(next, -beta, -alpha, success);
		if (*success == 0) return 0;
		if (v > alpha) {
			if (v >= beta) {
				*success = 2;
				return v;
			}
			alpha = v;
		}
	}

	v = probe_wdl_table(pos, success);
	if (*success == 0) return 0;
	if (alpha >= v) {
		*success = 1 + (alpha > 0);
		return alpha;
	}
	else {
		*success = 1;
		return v;
	}
}

// Probe the WDL table for a particular position.
// If *success != 0, the probe was successful.
// The return value is from the point of view of the side to move:
// -2 : loss
// -1 : loss, but draw under 50-move rule
//  0 : draw
//  1 : win, but draw under 50-move rule
//  2 : win
int Tablebases::probe_wdl(position& pos, int *success)
{
	int v;

	*success = 1;
	v = probe_ab(pos, -2, 2, success);

	// If en passant is not possible, we are done.
	if (pos.GetEPSquare() == OUTSIDE)
		return v;
	if (!(*success)) return 0;

	// Now handle en passant.
	int v1 = -3;
	// Generate (at least) all legal en passant captures.
	if (pos.Checked())  pos.GenerateMoves<CHECK_EVASION>();
	else pos.GenerateMoves<EQUAL_CAPTURES>();

	int moveCount;
	ValuatedMove * moves = pos.GetMoves(moveCount);
	for (int i = 0; i < moveCount; ++i) {
		Move capture = moves->move;
		moves++;
		if (type(capture) != ENPASSANT) continue;
		position next(pos);
		if (!next.ApplyMove(capture)) continue;
		int v0 = -probe_ab(next, -2, 2, success);
		if (*success == 0) return 0;
		if (v0 > v1) v1 = v0;
	}
	if (v1 > -3) {
		if (v1 >= v) v = v1;
		else if (v == 0) {
			pos.GenerateMoves<ALL>();
			moves = pos.GetMoves(moveCount);
			// Check whether there is at least one legal non-ep move.
			for (int i = 0; i < moveCount; ++i) {
				Move capture = moves->move;
				moves++;
				if (type(capture) == ENPASSANT) continue;
				position next(pos);
				if (!next.ApplyMove(capture)) continue;
			}
			// If not, then we are forced to play the losing ep capture.
			if (moves->move == MOVE_NONE)
				v = v1;
		}
	}

	return v;
}

// This routine treats a position with en passant captures as one without.
static int probe_dtz_no_ep(position& pos, int *success)
{
	int wdl, dtz;

	wdl = probe_ab(pos, -2, 2, success);
	if (*success == 0) return 0;

	if (wdl == 0) return 0;

	if (*success == 2)
		return wdl == 2 ? 1 : 101;

	if (wdl > 0) {
		// Generate at least all legal non-capturing pawn moves
		// including non-capturing promotions.
		if (pos.Checked()) {
			pos.GenerateMoves<CHECK_EVASION>();
			pos.AddUnderPromotions();
		}
		else pos.GenerateMoves<ALL>();

		int moveCount;
		ValuatedMove * moves = pos.GetMoves(moveCount);
		for (int i = 0; i < moveCount; ++i) {
			Move move = moves->move;
			moves++;
			if (GetPieceType(pos.GetPieceOnSquare(from(move))) != PAWN || pos.GetPieceOnSquare(to(move)) != BLANK) continue;
			position next(pos);
			if (!next.ApplyMove(move)) continue;
			int v = -probe_ab(next, -2, -wdl + 1, success);
			if (*success == 0) return 0;
			if (v == wdl)
				return v == 2 ? 1 : 101;
		}
	}

	dtz = 1 + probe_dtz_table(pos, wdl, success);
	if (*success >= 0) {
		if (wdl & 1) dtz += 100;
		return wdl >= 0 ? dtz : -dtz;
	}

	if (wdl > 0) {
		int best = 0xffff;
		int moveCount;
		ValuatedMove * moves = pos.GetMoves(moveCount);
		for (int i = 0; i < moveCount; ++i) {
			Move move = moves->move;
			moves++;
			if (GetPieceType(pos.GetPieceOnSquare(from(move))) != PAWN || pos.GetPieceOnSquare(to(move)) != BLANK) continue;
			position next(pos);
			if (!next.ApplyMove(move)) continue;
			int v = -Tablebases::probe_dtz(next, success);
			if (*success == 0) return 0;
			if (v > 0 && v + 1 < best)
				best = v + 1;
		}
		return best;
	}
	else {
		int best = -1;
		if (pos.Checked()) {
			pos.GenerateMoves<CHECK_EVASION>();
			pos.AddUnderPromotions();
		}
		else pos.GenerateMoves<ALL>();

		int moveCount;
		ValuatedMove * moves = pos.GetMoves(moveCount);
		for (int i = 0; i < moveCount; ++i) {
			Move move = moves->move;
			moves++;
			int v;
			position next(pos);
			if (!next.ApplyMove(move)) continue;
			if (next.GetDrawPlyCount() == 0) {
				if (wdl == -2) v = -1;
				else {
					v = probe_ab(next, 1, 2, success);
					v = (v == 2) ? 0 : -101;
				}
			}
			else {
				v = -Tablebases::probe_dtz(next, success) - 1;
			}
			if (*success == 0) return 0;
			if (v < best)
				best = v;
		}
		return best;
	}
}

static int wdl_to_dtz[] = {
  -1, -101, 0, 101, 1
};

// Probe the DTZ table for a particular position.
// If *success != 0, the probe was successful.
// The return value is from the point of view of the side to move:
//         n < -100 : loss, but draw under 50-move rule
// -100 <= n < -1   : loss in n ply (assuming 50-move counter == 0)
//         0        : draw
//     1 < n <= 100 : win in n ply (assuming 50-move counter == 0)
//   100 < n        : win, but draw under 50-move rule
//
// The return value n can be off by 1: a return value -n can mean a loss
// in n+1 ply and a return value +n can mean a win in n+1 ply. This
// cannot happen for tables with positions exactly on the "edge" of
// the 50-move rule.
//
// This implies that if dtz > 0 is returned, the position is certainly
// a win if dtz + 50-move-counter <= 99. Care must be taken that the engine
// picks moves that preserve dtz + 50-move-counter <= 99.
//
// If n = 100 immediately after a capture or pawn move, then the position
// is also certainly a win, and during the whole phase until the next
// capture or pawn move, the inequality to be preserved is
// dtz + 50-movecounter <= 100.
//
// In short, if a move is available resulting in dtz + 50-move-counter <= 99,
// then do not accept moves leading to dtz + 50-move-counter == 100.
//
int Tablebases::probe_dtz(position& pos, int *success)
{
	*success = 1;
	int v = probe_dtz_no_ep(pos, success);

	if (pos.GetEPSquare() == OUTSIDE) return v;
	if (*success == 0) return 0;

	// Now handle en passant.
	int v1 = -3;

	pos.ResetMoveGeneration();
	if (pos.Checked()) pos.GenerateMoves<CHECK_EVASION>();
	else {
		pos.GenerateMoves<TACTICAL>();
	}
	pos.AddUnderPromotions();
	int moveCount;
	ValuatedMove * moves = pos.GetMoves(moveCount);
	for (int i = 0; i < moveCount; ++i) {
		Move capture = moves->move;
		moves++;
		if (type(capture) != ENPASSANT) continue;
		position next(pos);
		if (!next.ApplyMove(capture)) continue;
		int v0 = -probe_ab(next, -2, 2, success);
		if (*success == 0) return 0;
		if (v0 > v1) v1 = v0;
	}
	if (v1 > -3) {
		v1 = wdl_to_dtz[v1 + 2];
		if (v < -100) {
			if (v1 >= 0)
				v = v1;
		}
		else if (v < 0) {
			if (v1 >= 0 || v1 < 100)
				v = v1;
		}
		else if (v > 100) {
			if (v1 > 0)
				v = v1;
		}
		else if (v > 0) {
			if (v1 == 1)
				v = v1;
		}
		else if (v1 >= 0) {
			v = v1;
		}
		else {
			moves = pos.GetMoves(moveCount);
			for (int i = 0; i < moveCount; ++i) {
				Move move = moves->move;
				moves++;
				if (type(move) == ENPASSANT) continue;
				position next(pos);
				if (next.ApplyMove(move)) break;
			}
			if (moves->move == MOVE_NONE && !pos.IsCheck()) {
				pos.ResetMoveGeneration();
				pos.GenerateMoves<QUIETS>();
				moves = pos.GetMoves(moveCount);
				for (int i = 0; i < moveCount; ++i) {
					Move move = moves->move;
					moves++;
					position next(pos);
					if (next.ApplyMove(move)) break;
				}
			}
			if (moves->move == MOVE_NONE)
				v = v1;
		}
	}

	return v;
}

static int has_repeated(position * pos) {
	if (!pos) return false;
	int relevantPlies = std::min(int(pos->GetDrawPlyCount()), pos->GetPliesFromRoot());
	if (relevantPlies < 4) return false;
	return pos->checkRepetition() || has_repeated(pos->Previous());
}

static Value wdl_to_Value[5] = {
  -VALUE_MATE + MAX_DEPTH + 1,
  VALUE_DRAW - 2,
  VALUE_DRAW,
  VALUE_DRAW + 2,
  VALUE_MATE - MAX_DEPTH - 1
};

// Use the DTZ tables to filter out moves that don't preserve the win or draw.
// If the position is lost, but DTZ is fairly high, only keep moves that
// maximise DTZ.
//
// A return value false indicates that not all probes were successful and that
// no moves were filtered out.
bool Tablebases::root_probe(position& pos, std::vector<ValuatedMove>& rootMoves, Value& score)
{
	int success;

	int dtz = probe_dtz(pos, &success);
	if (!success) return false;

	// Probe each move.
	for (size_t i = 0; i < rootMoves.size(); i++) {
		Move move = rootMoves[i].move;
		position next(pos);
		next.ApplyMove(move);
		int v = 0;
		if (next.IsCheck() && dtz > 0) {
			if (next.GetResult() != OPEN) v = 1;
		}
		if (!v) {
			if (next.GetDrawPlyCount() != 0) {
				v = -Tablebases::probe_dtz(next, &success);
				if (v > 0) v++;
				else if (v < 0) v--;
			}
			else {
				v = -Tablebases::probe_wdl(next, &success);
				v = wdl_to_dtz[v + 2];
			}
		}
		if (!success) return false;
		rootMoves[i].score = (Value)v;
	}

	// Obtain 50-move counter for the root position.
	// In Stockfish there seems to be no clean way, so we do it like this:
	int cnt50 = pos.GetDrawPlyCount();

	// Use 50-move counter to determine whether the root position is
	// won, lost or drawn.
	int wdl = 0;
	if (dtz > 0)
		wdl = (dtz + cnt50 <= 100) ? 2 : 1;
	else if (dtz < 0)
		wdl = (-dtz + cnt50 <= 100) ? -2 : -1;

	// Determine the score to report to the user.
	score = wdl_to_Value[wdl + 2];
	// If the position is winning or losing, but too few moves left, adjust the
	// score to show how close it is to winning or losing.
	// NOTE: int(PawnValueEg) is used as scaling factor in score_to_uci().
	if (wdl == 1 && dtz <= 100)
		score = (Value)(((200 - dtz - cnt50) * int(PieceValuesEG[PAWN])) / 200);
	else if (wdl == -1 && dtz >= -100)
		score = -(Value)(((200 + dtz - cnt50) * int(PieceValuesEG[PAWN])) / 200);

	// Now be a bit smart about filtering out moves.
	size_t j = 0;
	if (dtz > 0) { // winning (or 50-move rule draw)
		int best = 0xffff;
		for (size_t i = 0; i < rootMoves.size(); i++) {
			int v = rootMoves[i].score;
			if (v > 0 && v < best)
				best = v;
		}
		int max = best;
		// If the current phase has not seen repetitions, then try all moves
		// that stay safely within the 50-move budget, if there are any.
		if (!has_repeated(pos.Previous()) && best + cnt50 <= 99)
			max = 99 - cnt50;
		for (size_t i = 0; i < rootMoves.size(); i++) {
			int v = rootMoves[i].score;
			if (v > 0 && v <= max)
				rootMoves[j++] = rootMoves[i];
		}
	}
	else if (dtz < 0) { // losing (or 50-move rule draw)
		int best = 0;
		for (size_t i = 0; i < rootMoves.size(); i++) {
			int v = rootMoves[i].score;
			if (v < best)
				best = v;
		}
		// Try all moves, unless we approach or have a 50-move rule draw.
		if (-best * 2 + cnt50 < 100)
			return true;
		for (size_t i = 0; i < rootMoves.size(); i++) {
			if (rootMoves[i].score == best)
				rootMoves[j++] = rootMoves[i];
		}
	}
	else { // drawing
   // Try all moves that preserve the draw.
		for (size_t i = 0; i < rootMoves.size(); i++) {
			if (rootMoves[i].score == 0)
				rootMoves[j++] = rootMoves[i];
		}
	}
	rootMoves.resize(j);

	return true;
}

// Use the WDL tables to filter out moves that don't preserve the win or draw.
// This is a fallback for the case that some or all DTZ tables are missing.
//
// A return value false indicates that not all probes were successful and that
// no moves were filtered out.
bool Tablebases::root_probe_wdl(position& pos, std::vector<ValuatedMove>& rootMoves, Value& score)
{
	int success;

	int wdl = Tablebases::probe_wdl(pos, &success);
	if (!success) return false;
	score = wdl_to_Value[wdl + 2];

	int best = -2;

	// Probe each move.
	for (size_t i = 0; i < rootMoves.size(); i++) {
		Move move = rootMoves[i].move;
		position next(pos);
		next.ApplyMove(move);
		int v = -Tablebases::probe_wdl(next, &success);
		if (!success) return false;
		rootMoves[i].score = (Value)v;
		if (v > best)
			best = v;
	}

	size_t j = 0;
	for (size_t i = 0; i < rootMoves.size(); i++) {
		if (rootMoves[i].score == best)
			rootMoves[j++] = rootMoves[i];
	}
	rootMoves.resize(j);

	return true;
}

