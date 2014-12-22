#pragma once
#include <vector>
#include "types.h"
#include "board.h"
#include "position.h"
#include "pgn.h"

enum PerftType { BASIC,  //All moves are generated together
	             P1, //tactiacl and Quiet Moves are generated seperately
                 P2, //Winning, Equal, Loosing Captures and Quiets are generated separately
                 P3  //Move iterator is used
};

int64_t bench(int depth);

uint64_t perft(position &pos, int depth);

void divide(position &pos, int depth);
bool testPerft(PerftType perftType = BASIC);
void testPolyglotKey();
bool testSEE();
vector<string> readTextFile(string file);
void testCheckQuietCheckMoveGeneration();
void testTacticalMoveGeneration();
void testSearch(position &pos, int depth);
void testFindMate();
void testResult();
void testMateInDos();