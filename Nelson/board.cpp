#include "board.h"
#include <vector>

Bitboard InBetweenFields[64][64];

void InitializeInBetweenFields() {
	for (int from = 0; from < 64; from++) {
		InBetweenFields[from][from] = 0ull;
		for (int to = from + 1; to < 64; to++) {
			InBetweenFields[from][to] = 0;
			int colFrom = from & 7;
			int colTo = to & 7;
			int rowFrom = from >> 3;
			int rowTo = to >> 3;
			if (colFrom == colTo) {
				for (int row = rowFrom + 1; row < rowTo; row++)
					InBetweenFields[from][to] = InBetweenFields[from][to]
					| (1ull << (8 * row + colTo));
			}
			else if (rowFrom == rowTo) {
				for (int col = colFrom + 1; col < colTo; col++)
					InBetweenFields[from][to] = InBetweenFields[from][to]
					| (1ull << (8 * rowTo + col));
			}
			else if (((to - from) % 7) == 0 && rowTo > rowFrom
				&& colTo < colFrom) //long diagonal A1-H8 has difference 63 which results in %7 = 0
			{
				for (int i = from + 7; i < to; i = i + 7)
					InBetweenFields[from][to] = InBetweenFields[from][to]
					| (1ull << i);
			}
			else if (((to - from) % 9) == 0 && rowTo > rowFrom) {
				for (int i = from + 9; i < to; i = i + 9)
					InBetweenFields[from][to] = InBetweenFields[from][to]
					| (1ull << i);
			}
		}
	}
	for (int from = 0; from < 64; from++) {
		for (int to = from + 1; to < 64; to++) {
			InBetweenFields[to][from] = InBetweenFields[from][to];
		}
	}
}

Bitboard KnightAttacks[64];
Bitboard KingAttacks[64];

void InitializeKnightAttacks() {
	int knightMoves[] = { -17, -15, -10, -6, 6, 10, 15, 17 };
	for (int square = A1; square <= H8; square++) {
		KnightAttacks[square] = 0;
		int col = square & 7;
		for (int move = 0; move < 8; move++)
		{
			int to = square + knightMoves[move];
			if (to < 0 || to > 63) continue;
			int toCol = to & 7;
			int colldiff = col - toCol;
			if (colldiff < -2 || colldiff > 2) continue;
			KnightAttacks[square] |= ToBitboard(to);
		}
	}
}

void InitializeKingAttacks() {
	int kingMoves[] = { -9, -8, -7, -1, 1, 7, 8, 9 };
	for (int square = 0; square < 64; square++)
	{
		KingAttacks[square] = 0;
		int col = square & 7;
		for (int move = 0; move < 8; move++)
		{
			int to = square + kingMoves[move];
			if (to < 0 || to > 63) continue;
			int toCol = to & 7;
			int colldiff = col - toCol;
			if (colldiff < -1 || colldiff > 1) continue;
			KingAttacks[square] |= ToBitboard(to);
		}
	}
}

Bitboard PawnAttacks[2][64];
void InitializePawnAttacks() {
	for (Square sq = A2; sq <= H7; ++sq) {
		Bitboard sqBB = ToBitboard(sq);
		PawnAttacks[WHITE][sq] = (sqBB << 7) & NOT_H_FILE;
		PawnAttacks[WHITE][sq] |= (sqBB << 9) & NOT_A_FILE;
		PawnAttacks[BLACK][sq] = (sqBB >> 9) & NOT_H_FILE;
		PawnAttacks[BLACK][sq] |= (sqBB >> 7) & NOT_A_FILE;
	}
}

int BishopShift[] = { 59, 60, 59, 59, 59, 59, 60, 59, 60, 60, 59, 59, 59, //a1 - e2
59, 60, 60, 60, 60, 57, 57, 57, 57, 60, 60, 59, 59, 57, 55, 55, 57, //f2 - f4
59, 59, 59, 59, 57, 55, 55, 57, 59, 59, 60, 60, 57, 57, 57, 57, 60, //g4 - g6
60, 60, 60, 59, 59, 59, 59, 60, 60, 59, 60, 59, 59, 59, 59, 60, 59 }; //h6 - h8

int RookShift[] = { 52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, //a1 - f2
54, 53, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, //g2 - g4
53, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53, //h4 - h6
54, 55, 55, 55, 55, 55, 54, 54, 53, 54, 54, 54, 54, 53, 54, 53 }; //a7 - h8

uint64_t RookMagics[] =
{ 0x8880004002208a10ull, 0x0840100040042004ull, //a1
0x0180281000802000ull, 0x0180048010000800ull,
0x060020a200100408ull, 0x210002880c000100ull,
0x0100420024008900ull, 0x808001288014c100ull,
0x0004800040042480ull, 0x0801002100804000ull, //a2
0x0003004500200010ull, 0x0008800800809000ull,
0x0944801400080080ull, 0x0960800600040081ull,
0x201a00112c0a0008ull, 0x0006000041811204ull,
0x0012208000804000ull, 0x0001404008a01000ull, //a3
0x0000820040220150ull, 0x00419b0010002100ull,
0x0184008080040800ull, 0x3481010004000208ull,
0x440844001850014aull, 0x0005c200004a8401ull,
0x0080010100204084ull, 0x0011610900400481ull, //a4
0x0001023100200040ull, 0x10021001000b0020ull,
0x0000240080180082ull, 0x8132400801100420ull,
0x1051000300044600ull, 0x2040008a00204904ull,
0x2080002000404000ull, 0x0004806000804002ull, //a5
0xc009002005001040ull, 0x2010020800801080ull,
0x2080880080800400ull, 0x8080020080800400ull,
0x0014020804000110ull, 0x4c30004106001084ull,
0x0080014420004000ull, 0x0403201003404001ull, //a6
0x1080408200720020ull, 0xc210010148110020ull,
0x00c0080100910004ull, 0x4001000400030008ull,
0x0006880201940010ull, 0x00190004b0430002ull,
0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, //a7
0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull,
0xffffffe9ffe7ce00ull, 0xfffffff5fff3e600ull,
0x01000a1008018400ull, 0x510FFFF5F63C96A0ull,
0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, //a8
0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull,
0x411FFFDDFFDBF4D6ull, 0x0042007008010402ull,
0x0003ffef27eebe74ull, 0x7645FFFECBFEA79Eull };
uint64_t BishopMagics[] =
{ 0xffedf9fd7cfcffffull, 0xfc0962854a77f576ull, //a1
0x0004080200501010ull, 0x0084140181222400ull,
0x0214104404000006ull, 0x0108822021880802ull,
0xfc0a66c64a7ef576ull, 0x7ffdfdfcbd79ffffull,
0xfc0846a64a34fff6ull, 0xfc087a874a3cf7f6ull, //a2
0x0208424a00450000ull, 0x00022404018c0440ull,
0x0100520210c00014ull, 0x0000019010280501ull,
0xfc0864ae59b4ff76ull, 0x3c0860af4b35ff76ull,
0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, //a3
0x2002200108010100ull, 0x4c88000401202000ull,
0x0001800400a00820ull, 0x0a60805840504024ull,
0x7c0c028f5b34ff76ull, 0xfc0a028e5ab4df76ull,
0x4150041830200a40ull, 0x0041100020620a42ull, //a4
0x9004208424010400ull, 0x0000808008020006ull,
0x00018400a0812001ull, 0x0108044102044200ull,
0x4116020501b80100ull, 0x082a620401028200ull,
0x0001201041200444ull, 0x00011410100a1006ull, //a5
0x2444020800430045ull, 0x00000a0080080180ull,
0x0802008024020200ull, 0x0004d00100408080ull,
0x0a0c24044c240110ull, 0x020609a064410400ull,
0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, //a6
0x8041002090008848ull, 0x404004a018003300ull,
0x0000080504020a42ull, 0x8010a01800408020ull,
0x43ff9a5cf4ca0c01ull, 0x4BFFCD8E7C587601ull,
0xfc0ff2865334f576ull, 0xfc0bf6ce5924f576ull, //a7
0x00080854040400c0ull, 0x0020204142020045ull,
0x10c800300a088008ull, 0x1322082108008000ull,
0xc3ffb7dc36ca8c89ull, 0xc3ff8a54f4ca2c89ull,
0xfffffcfcfd79edffull, 0xfc0863fccb147576ull, //a8
0x100d203e83480802ull, 0x0108142048420200ull,
0x4040000110420200ull, 0x1480001020010104ull,
0xfc087e8e4bb2f736ull, 0x43ff9e4ef4ca2c89ull };


Bitboard OccupancyMaskRook[64];
Bitboard OccupancyMaskBishop[64];

void InitializeOccupancyMasks() {
	int i, bitRef;
	Bitboard mask;
	for (bitRef = 0; bitRef <= 63; bitRef++) {
		mask = 0;
		for (i = bitRef + 8; i <= 55; i += 8)
			mask |= (1ull << i);
		for (i = bitRef - 8; i >= 8; i -= 8)
			mask |= (1ull << i);
		for (i = bitRef + 1; (i & 7) != 7 && (i & 7) != 0; i++)
			mask |= (1ull << i);
		for (i = bitRef - 1; (i & 7) != 7 && (i & 7) != 0 && i >= 0; i--)
			mask |= (1ull << i);
		OccupancyMaskRook[bitRef] = mask;
	}
	for (bitRef = 0; bitRef <= 63; bitRef++) {
		mask = 0;
		for (i = bitRef + 9; (i & 7) != 7 && (i & 7) != 0 && i <= 55; i += 9)
			mask |= (1ull << i);
		for (i = bitRef - 9; (i & 7) != 7 && (i & 7) != 0 && i >= 8; i -= 9)
			mask |= (1ull << i);
		for (i = bitRef + 7; (i & 7) != 7 && (i & 7) != 0 && i <= 55; i += 7)
			mask |= (1ull << i);
		for (i = bitRef - 7; (i & 7) != 7 && (i & 7) != 0 && i >= 8; i -= 7)
			mask |= (1ull << i);
		OccupancyMaskBishop[bitRef] = mask;
	}
}

std::vector<Bitboard> occupancyVariation[64];
std::vector<Bitboard> occupancyAttackSet[64];

void generateOccupancyVariations(bool isRook)
{
	for (int i = 0; i < 64; i++) {
		occupancyAttackSet[i].clear();
		occupancyVariation[i].clear();
	}
	Bitboard* occupancies;
	if (isRook) occupancies = OccupancyMaskRook; else occupancies = OccupancyMaskBishop;
	for (int square = A1; square <= H8; square++) {
		Bitboard occupancy = occupancies[square];
		Bitboard subset = 0;
		do {
			occupancyVariation[square].push_back(subset);
			//Now calculate attack set: subset without shadowed fields
			Bitboard attackset = 0;
			Bitboard temp = subset;
			do {
				Square to = lsb(temp);
				if ((InBetweenFields[square][to] & subset) == 0) attackset |= ToBitboard(to);
				temp &= temp - 1;
			} while (temp);
			occupancyAttackSet[square].push_back(attackset);
			subset = (subset - occupancy) & occupancy;
		} while (subset);
	}
}

int MaxIndexRook[64];
int MaxIndexBishop[64];
int IndexOffsetRook[64];
int IndexOffsetBishop[64];

Bitboard MagicMovesRook[88576];
Bitboard MagicMovesBishop[4800];

void InitializeMaxIndices() {
	for (int i = 0; i < 64; i++)
	{
		MaxIndexRook[i] = 1 << (64 - RookShift[i]);
		MaxIndexBishop[i] = 1 << (64 - BishopShift[i]);
		if (i == 0) {
			IndexOffsetRook[0] = 0;
			IndexOffsetBishop[0] = 0;
		}
		else {
			IndexOffsetRook[i] = IndexOffsetRook[i - 1] + MaxIndexRook[i - 1];
			IndexOffsetBishop[i] = IndexOffsetBishop[i - 1] + MaxIndexBishop[i - 1];
		}
	}
	int totalRook = IndexOffsetRook[63] + MaxIndexRook[63];
	int totalBishop = IndexOffsetBishop[63] + MaxIndexBishop[63];
}

void InitializeMoveDB(bool isRook) {
	for (int square = A1; square <= H8; square++) {
		for (int i = 0; i < occupancyVariation[square].size(); i++) {
			if (isRook) {
				int magicIndex = (int)((occupancyVariation[square][i] * RookMagics[square]) >> RookShift[square]);
				Bitboard attacks = 0;
				for (int to = square + 8; to <= H8; to += 8) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				for (int to = square - 8; to >= A1; to -= 8) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				for (int to = square + 1; (to & 7) != 0; to++) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				for (int to = square - 1; (to & 7) != 7; to--) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				MagicMovesRook[IndexOffsetRook[square] + magicIndex] = attacks;
			}
			else {
				int magicIndex = (int)((occupancyVariation[square][i] * BishopMagics[square]) >> BishopShift[square]);
				Bitboard attacks = 0;
				for (int to = square + 9; to <= H8 && (to & 7) != 0; to += 9) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				for (int to = square + 7; to <= H8 && (to & 7) != 7; to += 7) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				for (int to = square - 7; to >= A1 && (to & 7) != 0; to -= 7) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				for (int to = square - 9; to >= A1 && (to & 7) != 7; to -= 9) {
					attacks |= ToBitboard(to);
					if ((occupancyVariation[square][i] & ToBitboard(to)) != 0) break;
				}
				MagicMovesBishop[IndexOffsetBishop[square] + magicIndex] = attacks;
			}
		}
	}
}

void InitializeMagic() {
	InitializeMaxIndices();
	InitializeOccupancyMasks();
	generateOccupancyVariations(false);
	InitializeMoveDB(false);
	generateOccupancyVariations(true);
	InitializeMoveDB(true);
}

void Initialize() {
	InitializeInBetweenFields();
	InitializeKingAttacks();
	InitializeKnightAttacks();
	InitializePawnAttacks();
	InitializeMagic();
}