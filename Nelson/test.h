#pragma once

#include "types.h"
#include "board.h"
#include "position.h"

uint64_t perft(position &pos, int depth);

void divide(position &pos, int depth);
bool testPerft();
void testPolyglotKey();
bool testSEE();