#pragma once

#include "types.h"

const int EmergencyTime = 50; //50 ms Emergency time
const int PV_MAX_LENGTH = 32;
const int MASK_TIME_CHECK = (1 << 14) - 1;

const Value PieceValuesMG[] { Value(950), Value(520), Value(325), Value(325), Value(80), VALUE_ZERO, VALUE_ZERO };
const Value PieceValuesEG[] { Value(950), Value(520), Value(325), Value(325), Value(100), VALUE_ZERO, VALUE_ZERO };