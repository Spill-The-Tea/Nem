#pragma once
#include <vector>
#include "types.h"
#include "board.h"
#include "position.h"
#include "pgn.h"

enum PerftType { BASIC,  //All moves are generated together
	             P1, //tactical and Quiet Moves are generated seperately
                 P2, //Winning, Equal, Loosing Captures and Quiets are generated separately
                 P3  //Move iterator is used
};

int64_t bench(int depth); //Benchmark positions from SF
int64_t bench2(int depth); //Random positions

uint64_t perft(position &pos, int depth);

void divide(position &pos, int depth);
bool testPerft(PerftType perftType = BASIC);
void testPolyglotKey();
bool testSEE();
std::vector<std::string> readTextFile(std::string file);
void testCheckQuietCheckMoveGeneration();
void testTacticalMoveGeneration();
void testSearch(position &pos, int depth);
void testFindMate();
void testResult();
void testMateInDos();
void testRepetition();
void testKPK();