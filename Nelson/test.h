#pragma once

#include "types.h"
#include "board.h"
#include "position.h"

long perft(position &pos, int depth);

void divide(position &pos, int depth);
bool testPerft();