#pragma once

#include <map>
#include <memory>
#include "types.h"
#include "board.h"
#include "utils.h"

const int PV_MAX_LENGTH = 32; //Maximum Length of displayed Principal Variation
const int MASK_TIME_CHECK = (1 << 14) - 1; //Time is only checked each MASK_TIME_CHECK nodes

const int PAWN_TABLE_SIZE = 1 << 14; //has to be power of 2
const int KILLER_TABLE_SIZE = 1 << 11; //has to be power of 2

////Bonus for Passed Pawns (dynamic evaluations)
//const eval BONUS_PASSED_PAWN_BACKED = eval(0, 15); //Passed pawn with own rook behind
//const eval MALUS_PASSED_PAWN_BLOCKED[6] = { eval(0, 3), eval(0,10), eval(0,10), eval(0,30), EVAL_ZERO, eval(0,30) }; //Passed pawn with own rook behind

namespace settings {

	enum CAPTURES {
		QxP, BxP, NxP, RxP, QxN,
		QxR, RxN, QxB, BxN, RxB,
		KxP, EP, PxP, KxN, PxB,
		BxR, PxN, NxR, NxN, NxB,
		RxR, BxB, PxR, KxB, KxR,
		QxQ,
		RxQ, BxQ, NxQ, PxQ, KxQ //Winning Captures of Queen
	};

	struct Parameters {
	public:
		void Initialize();
		int LMRReduction(int depth, int moveNumber);

		int HelperThreads = 0;
		Value Contempt = VALUE_ZERO;
		Color EngineSide = WHITE;
		Protocol protocol = NO_PROTOCOL;
		int EmergencyTime = 100;
		//King safety parameters
		int KING_SAFETY_MAXVAL = 500;
		int KING_SAFETY_MAXINDEX = 61;
		double KING_SAFETY_LINEAR = 0.5;
		int ATTACK_UNITS_SAFE_CONTACT_CHECK = 5;
		Value KING_SAFETY[100];
		eval CENTER_CONTROL = eval(3, 0);
		eval EXTENDED_CENTER_CONTROL = eval(3, 0);
		eval SCALE_BISHOP_PAIR_WITH_PAWNS = EVAL_ZERO; //Reduce Bonus Bishop Pair by this value for each pawn on the board
		eval BONUS_BISHOP_PAIR_NO_OPP_MINOR = EVAL_ZERO; //Bonus for Bishop pair, if opponent has no minor piece for exchange
		eval SCALE_EXCHANGE_WITH_PAWNS = EVAL_ZERO; //Decrease Value of Exchange with number of pawns
		eval SCALE_EXCHANGE_WITH_MAJORS = EVAL_ZERO; //Decrease Value of Exchange with number of majors
		eval PAWN_STORM[4] = { eval(10, 0), eval(25, 0), eval(15, 0), eval(5, 0) };
		Value BETA_PRUNING_FACTOR = Value(95);
		inline Value BetaPruningMargin(int depth) { return Value(depth * BETA_PRUNING_FACTOR); }
		Value RAZORING_FACTOR = Value(50);
		Value RAZORING_OFFSET = Value(50);
		inline Value RazoringMargin(int depth) { return Value(depth * RAZORING_FACTOR + RAZORING_OFFSET); }
		int LIMIT_QSEARCH = -3;
		int LIMIT_QSEARCH_TT = LIMIT_QSEARCH + 1;
		eval HANGING = eval(16, 13);
		eval KING_ON_ONE = eval(1, 29);
		eval KING_ON_MANY = eval(3, 63);
		eval ROOK_ON_OPENFILE = eval(10, 0);
		eval ROOK_ON_SEMIOPENFILE = eval(10, 0);
		eval ROOK_ON_SEMIOPENFILE_WITH_KQ = eval(5, 0);
		eval ROOK_ON_7TH = eval(20, 0);

		eval BONUS_BISHOP_PAIR = eval(50);
		Value BONUS_CASTLING = Value(0);
        Value BONUS_TEMPO = Value(5);
        Value DELTA_PRUNING_SAFETY_MARGIN = Value(VALUE_100CP);
        eval PAWN_SHELTER_2ND_RANK = eval(30, -10);
		eval PAWN_SHELTER_3RD_RANK = eval(15, -8);
		eval PAWN_SHELTER_4TH_RANK = eval(8, -4);
        Value PROBCUT_MARGIN = Value(90);
		Value BONUS_KNIGHT_OUTPOST = Value(5);
		Value BONUS_BISHOP_OUTPOST = Value(0);
		eval PieceValues[7]{ eval(1025), eval(490, 550), eval(325), eval(325), eval(80, 100), eval(VALUE_KNOWN_WIN), eval(0) };
		int FULTILITY_PRUNING_DEPTH = 3;
		Value FUTILITY_PRUNING_LIMIT[4] = { VALUE_ZERO, PieceValues[BISHOP].mgScore, PieceValues[ROOK].mgScore, PieceValues[QUEEN].mgScore };
		eval PSQT[12][64]{
			{  // White Queens
				eval(-10,-8), eval(-7,-5), eval(-4,-2), eval(-2,0), eval(-2,0), eval(-4,-2), eval(-7,-5), eval(-10,-8),  // Rank 1
				eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 2
				eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 3
				eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 4
				eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 5
				eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 6
				eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 7
				eval(-7,-8), eval(-4,-5), eval(-2,-2), eval(0,0), eval(0,0), eval(-2,-2), eval(-4,-5), eval(-7,-8)  // Rank 8
			},
			{  // Black Queen
				eval(7,8), eval(4,5), eval(2,2), eval(0,0), eval(0,0), eval(2,2), eval(4,5), eval(7,8),  // Rank 1
				eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 2
				eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 3
				eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 4
				eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 5
				eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 6
				eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 7
				eval(10,8), eval(7,5), eval(4,2), eval(2,0), eval(2,0), eval(4,2), eval(7,5), eval(10,8)  // Rank 8
			},
			{  // White Rook
				eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 1
				eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 2
				eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 3
				eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 4
				eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 5
				eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 6
				eval(3,0), eval(3,0), eval(4,0), eval(6,0), eval(6,0), eval(4,0), eval(3,0), eval(3,0),  // Rank 7
				eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0)  // Rank 8
			},
			{  // Black Rook
				eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 1
				eval(-3,0), eval(-3,0), eval(-4,0), eval(-6,0), eval(-6,0), eval(-4,0), eval(-3,0), eval(-3,0),  // Rank 2
				eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 3
				eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 4
				eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 5
				eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 6
				eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 7
				eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0)  // Rank 8
			},
			{  // White Bishop
				eval(-12,-8), eval(-10,-5), eval(-7,-2), eval(-4,0), eval(-4,0), eval(-7,-2), eval(-10,-5), eval(-12,-8),  // Rank 1
				eval(-4,-5), eval(3,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(3,-2), eval(-4,-5),  // Rank 2
				eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 3
				eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 4
				eval(0,0), eval(5,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(5,2), eval(0,0),  // Rank 5
				eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 6
				eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 7
				eval(-7,-8), eval(-4,-5), eval(-2,-2), eval(0,0), eval(0,0), eval(-2,-2), eval(-4,-5), eval(-7,-8)  // Rank 8
			},
			{  // Black Bishop
				eval(7,8), eval(4,5), eval(2,2), eval(0,0), eval(0,0), eval(2,2), eval(4,5), eval(7,8),  // Rank 1
				eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 2
				eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 3
				eval(0,0), eval(-5,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-5,-2), eval(0,0),  // Rank 4
				eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 5
				eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 6
				eval(4,5), eval(-3,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(-3,2), eval(4,5),  // Rank 7
				eval(12,8), eval(10,5), eval(7,2), eval(4,0), eval(4,0), eval(7,2), eval(10,5), eval(12,8)  // Rank 8
			},
			{  // White Knight
				eval(-19,-18), eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(-3,-2), eval(-8,-7), eval(-14,-12), eval(-19,-18),  // Rank 1
				eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(4,5), eval(4,5), eval(-3,-2), eval(-8,-7), eval(-14,-12),  // Rank 2
				eval(-8,-7), eval(-3,-2), eval(4,5), eval(15,16), eval(15,16), eval(4,5), eval(-3,-2), eval(-8,-7),  // Rank 3
				eval(-3,-2), eval(4,5), eval(15,16), eval(21,22), eval(21,22), eval(15,16), eval(4,5), eval(-3,-2),  // Rank 4
				eval(-3,-2), eval(4,5), eval(21,16), eval(26,22), eval(26,22), eval(21,16), eval(4,5), eval(-3,-2),  // Rank 5
				eval(-8,-7), eval(-3,-2), eval(10,5), eval(31,16), eval(31,16), eval(10,5), eval(-3,-2), eval(-8,-7),  // Rank 6
				eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(4,5), eval(4,5), eval(-3,-2), eval(-8,-7), eval(-14,-12),  // Rank 7
				eval(-19,-18), eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(-3,-2), eval(-8,-7), eval(-14,-12), eval(-19,-18)  // Rank 8
			},
			{  // Black Knight
				eval(19,18), eval(14,12), eval(8,7), eval(3,2), eval(3,2), eval(8,7), eval(14,12), eval(19,18),  // Rank 1
				eval(14,12), eval(8,7), eval(3,2), eval(-4,-5), eval(-4,-5), eval(3,2), eval(8,7), eval(14,12),  // Rank 2
				eval(8,7), eval(3,2), eval(-10,-5), eval(-31,-16), eval(-31,-16), eval(-10,-5), eval(3,2), eval(8,7),  // Rank 3
				eval(3,2), eval(-4,-5), eval(-21,-16), eval(-26,-22), eval(-26,-22), eval(-21,-16), eval(-4,-5), eval(3,2),  // Rank 4
				eval(3,2), eval(-4,-5), eval(-15,-16), eval(-21,-22), eval(-21,-22), eval(-15,-16), eval(-4,-5), eval(3,2),  // Rank 5
				eval(8,7), eval(3,2), eval(-4,-5), eval(-15,-16), eval(-15,-16), eval(-4,-5), eval(3,2), eval(8,7),  // Rank 6
				eval(14,12), eval(8,7), eval(3,2), eval(-4,-5), eval(-4,-5), eval(3,2), eval(8,7), eval(14,12),  // Rank 7
				eval(19,18), eval(14,12), eval(8,7), eval(3,2), eval(3,2), eval(8,7), eval(14,12), eval(19,18)  // Rank 8
			},
			{  // White Pawn
				eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0),  // Rank 1
				eval(-17,-11), eval(-9,-11), eval(-7,-11), eval(-9,-11), eval(-9,-11), eval(-7,-11), eval(-9,-11), eval(-17,-11),  // Rank 2
				eval(-15,-9), eval(-12,-9), eval(-7,-9), eval(-1,-9), eval(-1,-9), eval(-7,-9), eval(-12,-9), eval(-15,-9),  // Rank 3
				eval(-17,-6), eval(-9,-6), eval(-7,-6), eval(6,-6), eval(6,-6), eval(-7,-6), eval(-9,-6), eval(-17,-6),  // Rank 4
				eval(-9,-3), eval(-7,-3), eval(-4,-3), eval(9,-3), eval(9,-3), eval(-4,-3), eval(-7,-3), eval(-9,-3),  // Rank 5
				eval(3,9), eval(6,9), eval(9,9), eval(11,9), eval(11,9), eval(9,9), eval(6,9), eval(3,9),  // Rank 6
				eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20),  // Rank 7
				eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0)  // Rank 8
			},
			{  //  Black Pawn
				eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0),  // Rank 1
				eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20),  // Rank 2
				eval(-3,-9), eval(-6,-9), eval(-9,-9), eval(-11,-9), eval(-11,-9), eval(-9,-9), eval(-6,-9), eval(-3,-9),  // Rank 3
				eval(9,3), eval(7,3), eval(4,3), eval(-9,3), eval(-9,3), eval(4,3), eval(7,3), eval(9,3),  // Rank 4
				eval(17,6), eval(9,6), eval(7,6), eval(-6,6), eval(-6,6), eval(7,6), eval(9,6), eval(17,6),  // Rank 5
				eval(15,9), eval(12,9), eval(7,9), eval(1,9), eval(1,9), eval(7,9), eval(12,9), eval(15,9),  // Rank 6
				eval(17,11), eval(9,11), eval(7,11), eval(9,11), eval(9,11), eval(7,11), eval(9,11), eval(17,11),  // Rank 7
				eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0)  // Rank 8
			},
			{  // White King
				eval(16,-20), eval(17,-15), eval(16,-9), eval(14,-4), eval(14,-4), eval(16,-9), eval(17,-15), eval(16,-20),  // Rank 1
				eval(16,-15), eval(16,-9), eval(14,-4), eval(11,7), eval(11,7), eval(14,-4), eval(16,-9), eval(16,-15),  // Rank 2
				eval(11,-9), eval(11,-4), eval(11,7), eval(8,23), eval(8,23), eval(11,7), eval(11,-4), eval(11,-9),  // Rank 3
				eval(8,-4), eval(8,7), eval(3,23), eval(-2,28), eval(-2,28), eval(3,23), eval(8,7), eval(8,-4),  // Rank 4
				eval(3,-4), eval(0,7), eval(-2,23), eval(-7,28), eval(-7,28), eval(-2,23), eval(0,7), eval(3,-4),  // Rank 5
				eval(-7,-9), eval(-7,-4), eval(-12,7), eval(-18,23), eval(-18,23), eval(-12,7), eval(-7,-4), eval(-7,-9),  // Rank 6
				eval(-12,-15), eval(-12,-9), eval(-18,-4), eval(-18,7), eval(-18,7), eval(-18,-4), eval(-12,-9), eval(-12,-15),  // Rank 7
				eval(-18,-20), eval(-18,-15), eval(-18,-9), eval(-18,-4), eval(-18,-4), eval(-18,-9), eval(-18,-15), eval(-18,-20)  // Rank 8
			},
			{  // Black King
				eval(18,20), eval(18,15), eval(18,9), eval(18,4), eval(18,4), eval(18,9), eval(18,15), eval(18,20),  // Rank 1
				eval(12,15), eval(12,9), eval(18,4), eval(18,-7), eval(18,-7), eval(18,4), eval(12,9), eval(12,15),  // Rank 2
				eval(7,9), eval(7,4), eval(12,-7), eval(18,-23), eval(18,-23), eval(12,-7), eval(7,4), eval(7,9),  // Rank 3
				eval(-3,4), eval(0,-7), eval(2,-23), eval(7,-28), eval(7,-28), eval(2,-23), eval(0,-7), eval(-3,4),  // Rank 4
				eval(-8,4), eval(-8,-7), eval(-3,-23), eval(2,-28), eval(2,-28), eval(-3,-23), eval(-8,-7), eval(-8,4),  // Rank 5
				eval(-11,9), eval(-11,4), eval(-11,-7), eval(-8,-23), eval(-8,-23), eval(-11,-7), eval(-11,4), eval(-11,9),  // Rank 6
				eval(-16,15), eval(-16,9), eval(-14,4), eval(-11,-7), eval(-11,-7), eval(-14,4), eval(-16,9), eval(-16,15),  // Rank 7
				eval(-16,20), eval(-17,15), eval(-16,9), eval(-14,4), eval(-14,4), eval(-16,9), eval(-17,15), eval(-16,20)  // Rank 8
			}
		};
		Value CAPTURE_SCORES[6][7] = {
			// Captured:  QUEEN, ROOK,   BISHOP,   KNIGHT,    PAWN,   King,   EP-Capture/Promotion
			{ Value(QxQ), Value(QxR),  Value(QxB),  Value(QxN),  Value(QxP),  Value(0),  Value(0) },   // QUEEN
			{ Value(RxQ), Value(RxR), Value(RxB), Value(RxN),  Value(RxP),  Value(0),  Value(0) },   // ROOK
			{ Value(BxQ), Value(BxR), Value(BxB), Value(BxN),  Value(BxP),  Value(0),  Value(0) },   // BISHOP
			{ Value(NxQ), Value(NxR), Value(NxB), Value(NxN), Value(NxP),  Value(0),  Value(0) },   // KNIGHT
			{ Value(PxQ), Value(PxR), Value(PxB), Value(PxN), Value(PxP),  Value(0), Value(EP) },   // PAWN
			{ Value(KxQ), Value(KxR), Value(KxB), Value(KxN), Value(KxP), Value(0),  Value(0) }    // KING
		};
		eval MOBILITY_BONUS_KNIGHT[9] = { eval(-29, -23), eval(-19, -14), eval(-4, -4), eval(1, 0), eval(7, 4), eval(12, 9), eval(16, 12), eval(19, 14), eval(21, 15) };
		eval MOBILITY_BONUS_BISHOP[14] = { eval(-23, -22), eval(-12, -11), eval(2, 0), eval(9, 7), eval(15, 14), eval(22, 19), eval(28, 25), eval(32, 29), eval(33, 32),
			eval(36, 33), eval(37, 35), eval(37, 36), eval(39, 36), eval(40, 37) };
		eval MOBILITY_BONUS_ROOK[15] = { eval(-22, -25), eval(-14, -12), eval(-2, 0), eval(0, 7), eval(2, 15), eval(5, 22), eval(8, 29), eval(9, 37), eval(12, 44),
			eval(14, 50), eval(14, 53), eval(15, 56), eval(16, 57), eval(16, 57), eval(16, 58) };
		eval MOBILITY_BONUS_QUEEN[28] = { eval(-19, -18), eval(-12, -11), eval(-2, -2), eval(0, 0), eval(2, 4), eval(5, 8), eval(5, 14), eval(8, 18), eval(9, 18), eval(9, 19),
			eval(9, 19), eval(9, 19), eval(9, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19),
			eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19), eval(11, 19) };
		// Threat[defended/weak][minor/major attacking][attacked PieceType] contains
		// bonuses according to which piece type attacks which one.
		// "inspired" by SF
		eval Threat[2][2][6] = {
			{ { eval(0, 0), eval(0, 0), eval(10, 18), eval(12, 19), eval(22, 49), eval(18, 53) }, // Defended Minor
			{ eval(0, 0), eval(0, 0), eval(5, 7), eval(5, 7), eval(4, 7), eval(12, 24) } }, // Defended Major
			{ { eval(0, 0), eval(0, 16), eval(17, 21), eval(16, 25), eval(21, 50), eval(18, 52) }, // Weak Minor
			{ eval(0, 0), eval(0, 14), eval(13, 29), eval(13, 29), eval(0, 22), eval(12, 26) } } // Weak Major
		};
		eval PASSED_PAWN_BONUS[6] = { eval(0), eval(0), eval(23), eval(39), eval(81), eval(101) };
		eval MALUS_BLOCKED[6] = { eval(30), eval(20), eval(10), eval(5), eval(0), eval(0) }; //indexed by distance to conversion
		eval BONUS_PROTECTED_PASSED_PAWN[6] = { eval(0), eval(0), eval(0), eval(30), eval(30), eval(30) };
        eval MALUS_ISOLATED_PAWN = eval(5);
		eval MALUS_BACKWARD_PAWN = eval(20);
		eval MALUS_ISLAND_COUNT = eval(5);
		eval BONUS_CANDIDATE = eval(5);
		eval BONUS_LEVER = eval(5);
		eval MALUS_DOUBLED_PAWN = eval(0, 20);
#ifdef TB
		int TBProbeDepth = 8;
#endif
	private:
		int LMR_REDUCTION[64][64];
	};

	extern Parameters parameter;

	void Initialize();
#ifdef TUNE
	void processParameter(std::vector<std::string> parameters);
#endif

	enum OptionType { SPIN, CHECK, BUTTON, STRING };

	const std::string OPTION_HASH = "Hash";
	const std::string OPTION_CHESS960 = "UCI_Chess960";
	const std::string OPTION_CLEAR_HASH = "Clear Hash";
	const std::string OPTION_PRINT_OPTIONS = "Print Options";
	const std::string OPTION_CONTEMPT = "Contempt";
	const std::string OPTION_MULTIPV = "MultiPV";
	const std::string OPTION_THREADS = "Threads";
	const std::string OPTION_PONDER = "Ponder";
	const std::string OPTION_BOOK_FILE = "BookFile";
	const std::string OPTION_OWN_BOOK = "OwnBook";
	const std::string OPTION_OPPONENT = "UCI_Opponent";
	const std::string OPTION_EMERGENCY_TIME = "MoveOverhead";
	const std::string OPTION_NODES_TIME = "Nodestime"; //Nodes per millisecond
	const std::string OPTION_TEXEL_TUNING_WINS = "TTWin"; //Path to file containing EPD records of won positions
	const std::string OPTION_TEXEL_TUNING_DRAWS = "TTDraw"; //Path to file containing EPD records of drawn positions
	const std::string OPTION_TEXEL_TUNING_LOSSES = "TTLoss"; //Path to file containing EPD records of lost positions
	const std::string OPTION_TEXEL_TUNING_LABELLED = "TTLabeled"; //Path to file containing EPD records with WDL label
	const std::string OPTION_PIECE_VALUES_QUEEN_MG = "PVQM";
	const std::string OPTION_PIECE_VALUES_QUEEN_EG = "PVQE";
	const std::string OPTION_PIECE_VALUES_ROOK_MG = "PVRM";
	const std::string OPTION_PIECE_VALUES_ROOK_EG = "PVRE";
	const std::string OPTION_PIECE_VALUES_BISHOP_MG = "PVBM";
	const std::string OPTION_PIECE_VALUES_BISHOP_EG = "PVBE";
	const std::string OPTION_PIECE_VALUES_KNIGHT_MG = "PVNM";
	const std::string OPTION_PIECE_VALUES_KNIGHT_EG = "PVNE";
	const std::string OPTION_PIECE_VALUES_PAWN_MG = "PVPM";
	const std::string OPTION_PIECE_VALUES_PAWN_EG = "PVPE";
#ifdef TB
	const std::string OPTION_SYZYGY_PATH = "SyzygyPath";
	const std::string OPTION_SYZYGY_PROBE_DEPTH = "SyzygyProbeDepth";
#endif

	class Option {
	public:
		Option() { otype = OptionType::STRING; };
		Option(std::string Name, OptionType Type = OptionType::BUTTON, std::string DefaultValue = "", std::string MinValue = "", std::string MaxValue = "", bool Technical = false);
		virtual ~Option() { };
		void virtual set(std::string value) = 0;
		void virtual read(std::vector<std::string> &tokens) = 0;
		std::string printUCI();
		std::string printInfo() const;
		inline std::string getName() { return name; }
		inline bool isTechnical() { return technical; }
		inline OptionType getType() { return otype; }
	protected:
		std::string name;
		OptionType otype;
		std::string defaultValue;
		std::string minValue;
		std::string maxValue;
		bool technical;
	};



	class OptionSpin: public Option {
	public:
		OptionSpin(std::string Name, int Value, int Min, int Max, bool Technical = false) : Option(Name, OptionType::SPIN, std::to_string(Value), std::to_string(Min), std::to_string(Max), Technical) { };
		virtual ~OptionSpin() { };
		inline void set(std::string value) { _value = stoi(value); }
		inline void set(int value) { _value = value; }
		inline int getValue() { 
			if (_value == INT_MIN) set(defaultValue); 
			return _value; 
		}
		inline void read(std::vector<std::string> &tokens) { set(tokens[4]); }
	protected:
		int _value = INT_MIN;
	};

	class OptionCheck : public Option {
	public:
		OptionCheck(std::string Name, bool value, bool Technical = false);
	    virtual ~OptionCheck() { };
		void set(std::string value);
		inline bool getValue() { return _value; }
		inline void read(std::vector<std::string> &tokens) { set(tokens[4]); }
		inline void set(bool value) { _value = value; };
	protected:
		bool _value = true;
	};

	class OptionString : public Option {
	public:
		OptionString(std::string Name, std::string defaultValue = "", bool Technical = false) : Option(Name, OptionType::STRING, defaultValue, "", "", Technical) { _value = defaultValue; };
		virtual ~OptionString() { };
		void set(std::string value) { _value = value; }
		inline std::string getValue() { return _value; }
		void read(std::vector<std::string> &tokens);
	protected:
		std::string _value;
	};

	class OptionButton : public Option {
	public:
		OptionButton(std::string Name, bool Technical = false) : Option(Name, OptionType::BUTTON, "", "", "", Technical) { };
		virtual ~OptionButton() { };
		void set(std::string value) { }
		inline void read(std::vector<std::string> &tokens) { }
	};

	class OptionThread : public OptionSpin {
	public:
		OptionThread() : OptionSpin(OPTION_THREADS, parameter.HelperThreads + 1, 1, 128) { };
		virtual ~OptionThread() { };
		void set(std::string value);
	};
	
	class Option960 : public OptionCheck {
	public:
		Option960() : OptionCheck(OPTION_CHESS960, Chess960) { };
		virtual ~Option960() { };
		void set(std::string value);
		inline void set(bool value) { Chess960 = value; };
	};

	class OptionContempt : public OptionSpin {
	public:
		OptionContempt() : OptionSpin(OPTION_CONTEMPT, parameter.Contempt, -1000, 1000) { };
		virtual ~OptionContempt() { };
		void set(std::string value);
		void set(int value);
	};

	class OptionHash : public OptionSpin {
	public:
		OptionHash() : OptionSpin(OPTION_HASH, 32, 1, 16384) { };
		virtual ~OptionHash() { };
		void set(std::string value);
		void set(int value);
	};

	class Options : public std::map<std::string, Option *> {
	public:
		Options();
		void printUCI();
		void printInfo();
		void read(std::vector<std::string> &tokens);
		int getInt(std::string key);
		bool getBool(std::string key);
		std::string getString(std::string key);
		void set(std::string key, std::string value);
		void set(std::string key, int value);
		void set(std::string key, bool value);
	private:
		void initialize();
	};

	extern Options options;

}