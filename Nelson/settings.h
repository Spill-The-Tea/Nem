#pragma once

#include "types.h"

const int EmergencyTime = 50; //50 ms Emergency time
const int PV_MAX_LENGTH = 32;
const int MASK_TIME_CHECK = (1 << 14) - 1;

const Value PieceValuesMG[] { Value(950), Value(520), Value(325), Value(325), Value(80), VALUE_ZERO, VALUE_ZERO };
const Value PieceValuesEG[] { Value(950), Value(520), Value(325), Value(325), Value(100), VALUE_ZERO, VALUE_ZERO };

const int PAWN_TABLE_SIZE = 1 << 14; //has to be power of 2

const eval MOBILITY_BONUS_KNIGHT[] = { eval(-65, -50), eval(-42, -30), eval(-9, -10), eval(3, 0), eval(15, 10), eval(27, 20), eval(37, 28), eval(42, 31), eval(44, 33) };
const eval MOBILITY_BONUS_BISHOP[] = { eval(-52, -47), eval(-28, -23), eval(6, 1), eval(20, 15), eval(34, 29), eval(48, 43),  eval(60, 55), eval(68, 63), eval(74, 68), 
                                       eval(77, 72), eval(80, 75), eval(82, 77), eval(84, 79), eval(86, 81) };
const eval MOBILITY_BONUS_ROOK[] = { eval(-47, -53), eval(-31, -26), eval(-5, 0), eval(1, 16), eval(7, 32), eval(13, 48), eval(18, 64), eval(22, 80), eval(26, 96),
                                     eval(29, 109), eval(31, 115), eval(33, 119), eval(35, 122), eval(36, 123), eval(37, 124) };
const eval MOBILITY_BONUS_QUEEN[] = { eval(-42, -40), eval(-28, -23), eval(-5, -7), eval(0, 0), eval(6, 10), eval(11, 19), eval(13, 29), eval(18, 38), eval(20, 40), eval(21, 41), 
                                      eval(22, 41), eval(22, 41), eval(22, 41), eval(23, 41), eval(24, 41), eval(25, 41), eval(25, 41), eval(25, 41), eval(25, 41), eval(25, 41), 
									  eval(25, 41), eval(25, 41), eval(25, 41), eval(25, 41), eval(25, 41), eval(25, 41), eval(25, 41), eval(25, 41) };

static int MOBILITY_SCALE = 3;

const Value KING_SAFETY[100] = {
	Value(0), Value(0), Value(1), Value(2), Value(3), Value(5), Value(7), Value(9), Value(12), Value(15),
	Value(18), Value(22), Value(26), Value(30), Value(35), Value(39), Value(44), Value(50), Value(56), Value(62),
	Value(68), Value(75), Value(82), Value(85), Value(89), Value(97), Value(105), Value(113), Value(122), Value(131),
	Value(140), Value(150), Value(169), Value(180), Value(191), Value(202), Value(213), Value(225), Value(237), Value(248),
	Value(260), Value(272), Value(283), Value(295), Value(307), Value(319), Value(330), Value(342), Value(354), Value(366),
	Value(377), Value(389), Value(401), Value(412), Value(424), Value(436), Value(448), Value(459), Value(471), Value(483),
	Value(494), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500),
	Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500),
	Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500),
	Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500)
};