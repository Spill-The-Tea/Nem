#include <stdio.h>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <string>
#include <map>
#include "test.h"
#include "search.h"

using namespace std;

uint64_t nodeCount = 0;

uint64_t perft(position &pos, int depth) {
	nodeCount++;
	if (depth == 0) return 1;
	ValuatedMove move;
	uint64_t result = 0;
	ValuatedMove * moves = pos.GenerateMoves<ALL>();
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) result += perft(next, depth - 1);
		++moves;
	}
	return result;
}

uint64_t perft1(position &pos, int depth) {
	nodeCount++;
	if (depth == 0) return 1;
	ValuatedMove move;
	uint64_t result = 0;
	ValuatedMove * moves = pos.GenerateMoves<TACTICAL>();
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) result += perft1(next, depth - 1);
		++moves;
	}
	moves = pos.GenerateMoves<QUIETS>();
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) result += perft1(next, depth - 1);
		++moves;
	}
	return result;
}

uint64_t perft2(position &pos, int depth) {
	nodeCount++;
	if (depth == 0) return 1;
	ValuatedMove move;
	uint64_t result = 0;
	ValuatedMove * moves = pos.GenerateMoves<WINNING_CAPTURES>();
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) result += perft2(next, depth - 1);
		++moves;
	}
	moves = pos.GenerateMoves<EQUAL_CAPTURES>();
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) result += perft2(next, depth - 1);
		++moves;
	}
	moves = pos.GenerateMoves<LOOSING_CAPTURES>();
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) result += perft2(next, depth - 1);
		++moves;
	}
	moves = pos.GenerateMoves<QUIETS>();
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) result += perft2(next, depth - 1);
		++moves;
	}
	return result;
}

uint64_t perft3(position &pos, int depth) {
	nodeCount++;
	if (depth == 0) return 1;
	uint64_t result = 0;
	pos.InitializeMoveIterator<MAIN_SEARCH>();
	Move move;
	while ((move = pos.NextMove())) {
		position next = position(pos);
		if (next.ApplyMove(move)) {
			result += perft3(next, depth - 1);
		}
	}
	return result;
}

void divide(position &pos, int depth) {
	ValuatedMove * moves = pos.GenerateMoves<ALL>();
	ValuatedMove move;
	uint64_t total = 0;
	while ((move = *moves).move) {
		position next = position(pos);
		if (next.ApplyMove(move.move)) {
			uint64_t p = perft(next, depth - 1);
			cout << toString(move.move) << "\t" << p << "\t" << next.fen() << endl;
			total += p;
		}
		++moves;
	}
	cout << "Total: " << total << endl;
}

void testSearch(position &pos, int depth) {
	SearchStopCriteria ssc;
	ssc.MaxDepth = depth;
	search engine;
	ValuatedMove vm = engine.Think(pos, ssc);
	cout << "Best Move: " << toString(vm.move) << " " << vm.score << endl;
}

void testFindMate() {
	std::map<string, Move> puzzles;
	//Mate in 2
	puzzles["2@2bqkbn1/2pppp2/np2N3/r3P1p1/p2N2B1/5Q2/PPPPKPP1/RNB2r2 w KQkq - 0 1"] = createMove(F3, F7);
	puzzles["2@8/6K1/1p1B1RB1/8/2Q5/2n1kP1N/3b4/4n3 w - - 0 1"] = createMove(D6, A3);
	puzzles["2@B7/K1B1p1Q1/5r2/7p/1P1kp1bR/3P3R/1P1NP3/2n5 w - - 0 1"] = createMove(A8, C6);
	puzzles["2@1r6/4b2k/1q1pNrpp/p2Pp3/4P3/1P1R3Q/5PPP/5RK1 w - - 0 1"] = createMove(H3, H6);
	puzzles["2@3r1b1k/5Q1p/p2p1P2/5R2/4q2P/1P2P3/PB5K/8 w - - 0 1"] = createMove(F7, G8);
	puzzles["2@r1bq2r1/b4pk1/p1pp1p2/1p2pP2/1P2P1PB/3P4/1PPQ2P1/R3K2R w - - 0 1"] = createMove(D2, H6);
	puzzles["2@8/8/8/r2p4/kp6/1R1Q4/8/K7 w - - 0 1"] = createMove(B3, B2);
	puzzles["2@7B/2R1KnQ1/1p1PP3/3k4/2N5/r3p1N1/4n3/1q6 w - - 0 1"] = createMove(G7, A1);
	//Mate in 3
	puzzles["3@1r3r1k/5Bpp/8/8/P2qQ3/5R2/1b4PP/5K2 w - - 0 1"] = createMove(E4, H7);
	puzzles["3@r1b1r1k1/1pq1bp1p/p3pBp1/3pR3/7Q/2PB4/PP3PPP/5RK1 w - - 0 1"] = createMove(H4, H7);
	puzzles["3@r5rk/5p1p/5R2/4B3/8/8/7P/7K w - - 0 1"] = createMove(F6, A6);
	puzzles["3@5B2/6P1/1p6/8/1N6/kP6/2K5/8 w - - 0 1"] = createMove<PROMOTION>(G7, G8, KNIGHT);
	puzzles["3@8/R7/4kPP1/3ppp2/3B1P2/1K1P1P2/8/8 w - - 0 1"] = createMove(F6, F7);
	puzzles["3@3r1r1k/1p3p1p/p2p4/4n1NN/6bQ/1BPq4/P3p1PP/1R5K w - - 0 1"] = createMove(G5, F7);
	puzzles["3@rn4k1/3q2r1/p2P3Q/2p1p2P/Pp2P3/1P1B4/2P2P2/2K3R1 w - -"] = createMove(D3, C4);
	puzzles["3@6k1/5pp1/7p/3n4/8/2P5/KQq4P/4R3 b - - 0 1"] = createMove(D5, C3);
	//Mate in 4
	puzzles["4@r5r1/5b1k/2p2p1p/1pP2P2/1P1pRq2/8/1QB3PP/4R1K1 b - -"] = createMove(G8, G2);
	puzzles["4@2q1kr2/6R1/2p1p2Q/ppP2p1P/3P4/8/PP6/1K6 w - - 0 1"] = createMove(H6, G6);
	puzzles["4@r1k4r/4Pp1p/p2Q4/1p2p3/2q1b2P/P7/1PP5/2KR3R w - - 0 1"] = createMove(D6, D8);
	puzzles["4@8/5p1p/4p3/1Q6/P2k4/2p5/7q/2K5 b - - 0 1"] = createMove(H2, D2);
	//Mate in 5
	puzzles["5@6r1/p3p1rk/1p1pPp1p/q3n2R/4P3/3BR2P/PPP2QP1/7K w - -"] = createMove(H5, H6);
	puzzles["5@2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w - - 0 1"] = createMove(E7, E8);
	search engine;
	std::map<string, Move>::iterator iter;
	int count = 0;
	for (iter = puzzles.begin(); iter != puzzles.end(); iter++) {
		engine.Reset();
		string mateIn = iter->first.substr(0, 1);
		string fen = iter->first.substr(2, string::npos);
		SearchStopCriteria ssc;
		ssc.MaxDepth = 2 * atoi(mateIn.c_str()) - 1;
		position pos(fen);
		ValuatedMove vm = engine.Think(pos, ssc);
		cout << ((vm.move == iter->second) && (vm.score == VALUE_MATE - ssc.MaxDepth) ? "OK    " : "ERROR ") << "\t" << toString(vm.move) << "/" << toString(iter->second)
			<< "\t" << vm.score << "/" << VALUE_MATE - ssc.MaxDepth << "\t" << fen << endl;
		count++;
	}

}

void testTacticalMoveGeneration() {
	vector<string> lines;
	ifstream s;
	s.open("C:/Users/chrgu_000/Desktop/Data/cutechess/testpositions/ccrl.epd");
	string l;
	if (s.is_open())
	{
		long lineCount = 0;
		while (s)
		{
			getline(s, l);
			lines.push_back(l);
			lineCount++;
			if ((lineCount & 8191) == 0) cout << ".";
			if (((lineCount & 65535) == 0) || !s) {
				cout << lineCount << " Lines read from File - starting to analyze" << endl;
				long count = 0;
				for (vector<string>::iterator it = lines.begin(); it != lines.end(); ++it) {
					string line = *it;
					if (line.length() < 10) continue;
					size_t indx = line.find(" c0");
					string fen = line.substr(0, indx);
					position p1(fen);
					ValuatedMove * tacticalMoves = p1.GenerateMoves<TACTICAL>();
					position p2(fen);
					ValuatedMove * allMoves = p2.GenerateMoves<ALL>();
					ValuatedMove * tm = tacticalMoves;
					ValuatedMove * am;
					Move move;
					//check move is really tactical
					while ((move = tm->move)) {
						position p3(p1);
						if (p3.ApplyMove(move)) {
							if (p3.GetMaterialKey() == p1.GetMaterialKey()) {
								cout << endl << "Move " << toString(move) << " doesn't change Material key: " << fen << endl;
								__debugbreak();
							}
							bool found = false;
							Move amove;
							am = allMoves;
							while ((amove = am->move)) {
								if (amove == move) {
									found = true;
									break;
								}
								am++;
							}
							if (!found) {
								cout << endl << "Move " << toString(move) << " not part of all moves: " << fen << endl;
								__debugbreak();
							};
						}
						tm++;
					}
					//check for completeness
					am = allMoves;
					while ((move = am->move)) {
						position p3(p1);
						if (p3.GetMaterialKey() != p1.GetMaterialKey()) {
							tm = tacticalMoves;
							Move tmove;
							bool found = false;
							while ((tmove = tm->move)) {
								if (tmove == move) {
									found = true;
									break;
								}
								tm++;
							}
							if (!found) {
								cout << endl << "Move " << toString(move) << " isn't detected as tactical: " << fen << endl;
								__debugbreak();
							};
						}
						am++;
					}
					//if ((count & 1023) == 0) cout << endl << count << "\t";
					++count;
				}
				lines.clear();
			}
		}
		s.close();
		cout << endl;
		cout << lineCount << " lines read!" << endl;
	}
	else
	{
		cout << "Unable to open file" << std::endl;
	}
}

void testResult(string filename, Result result) {
	vector<string> lines;
	ifstream s;
	s.open(filename);
	string l;
	if (s.is_open())
	{
		long lineCount = 0;
		while (s)
		{
			getline(s, l);

			if ((lineCount & 32767) == 0) cout << ".";
			if (l.length() < 10 || !s) {
				long count = 0;
				for (vector<string>::iterator it = lines.begin(); it != lines.end(); ++it) {
					++count;
					string line = *it;
					size_t indx = line.find(" c0");
					string fen = line.substr(0, indx);
					position p1(fen);
					if (count < lines.size()) {
						if (p1.GetResult() != OPEN) {
							cout << "Expected: Open Actual: " << p1.GetResult() << "\t" << p1.fen() << endl;
							__debugbreak();
							position p2(fen);
							p2.GetResult();
						}
					}
					else {
						if (p1.GetResult() != result) {
							cout << "Expected: " << result << " Actual: " << p1.GetResult() << "\t" << p1.fen() << endl;
							__debugbreak();
							position p2(fen);
							p2.GetResult();
						}
					}					
				}
				lines.clear();
			}
			else {
				lines.push_back(l);
				lineCount++;
			}
		}
		s.close();
		cout << endl;
		cout << lineCount << " lines read!" << endl;
	}
	else
	{
		cout << "Unable to open file" << std::endl;
	}
}

void testResult() {
	chrono::system_clock::time_point begin = chrono::high_resolution_clock::now();
	testResult("C:/Users/chrgu_000/Desktop/Data/cutechess/testpositions/stalemate.epd", STALEMATE);
	testResult("C:/Users/chrgu_000/Desktop/Data/cutechess/testpositions/mate.epd", MATE);
	chrono::system_clock::time_point end = chrono::high_resolution_clock::now();
	auto runtime = end - begin;
	chrono::microseconds runtimeMS = chrono::duration_cast<chrono::microseconds>(runtime);
	cout << "Runtime: " << runtimeMS.count() / 1000000.0 << " s" << endl;
}

void testCheckQuietCheckMoveGeneration() {
	vector<string> lines;
	ifstream s;
	s.open("C:/Users/chrgu_000/Desktop/Data/cutechess/testpositions/ccrl.epd");
	string l;
	if (s.is_open())
	{
		long lineCount = 0;
		while (s)
		{
			getline(s, l);
			lines.push_back(l);
			lineCount++;
			if ((lineCount & 8191) == 0) cout << ".";
			if (((lineCount & 65535) == 0) || !s) {
				cout << lineCount << " Lines read from File - starting to analyze" << endl;
				long count = 0;
				for (vector<string>::iterator it = lines.begin(); it != lines.end(); ++it) {
					string line = *it;
					if (line.length() < 10) continue;
					size_t indx = line.find(" c0");
					string fen = line.substr(0, indx);
					position p1(fen);
					ValuatedMove * checkGivingMoves = p1.GenerateMoves<QUIET_CHECKS>();
					position p2(fen);
					ValuatedMove * quietMoves = p2.GenerateMoves<QUIETS>();
					ValuatedMove * cgm = checkGivingMoves;
					ValuatedMove * qm;
					Move move;
					//check quiet check move
					while ((move = cgm->move)) {
						position p3(p1);
						if (p3.ApplyMove(move)) {
							if (!p3.Checked()) {
								cout << endl << "Move " << toString(move) << " doesn't give check: " << fen << endl;
								__debugbreak();
							}
							bool found = false;
							Move qmove;
							qm = quietMoves;
							while ((qmove = qm->move)) {
								if (qmove == move) {
									found = true;
									break;
								}
								qm++;
							}
							if (!found) {
								cout << endl << "Move " << toString(move) << " not part of quiet moves: " << fen << endl;
								__debugbreak();
							};
						}
						cgm++;
					}
					//check for completeness
					qm = quietMoves;
					while ((move = qm->move)) {
						position p3(p1);
						if (p3.ApplyMove(move) && p3.Checked()) {
							cgm = checkGivingMoves;
							Move tmove;
							bool found = false;
							while ((tmove = cgm->move)) {
								if (tmove == move) {
									found = true;
									break;
								}
								cgm++;
							}
							if (!found) {
								cout << endl << "Move " << toString(move) << " not part of check giving moves: " << fen << endl;
								__debugbreak();
							};
						}
						qm++;
					}
					//if ((count & 1023) == 0) cout << endl << count << "\t";
					++count;
				}
				lines.clear();
			}
		}
		s.close();
		cout << endl;
		cout << lineCount << " lines read!" << endl;
	}
	else
	{
		cout << "Unable to open file" << std::endl;
	}
}

void testPolyglotKey() {
	uint64_t key = 0x463b96181691fc9c;
	position pos;
	cout << pos.GetHash() << " - " << key << " " << (pos.GetHash() == key) << " " << pos.fen() << endl;
	Move move = createMove(E2, E4);
	pos.ApplyMove(move);
	key = 0x823c9b50fd114196;
	cout << pos.GetHash() << " - " << key << " " << (pos.GetHash() == key) << " " << pos.fen() << endl;
	move = createMove(D7, D5);
	pos.ApplyMove(move);
	key = 0x0756b94461c50fb0;
	cout << pos.GetHash() << " - " << key << " " << (pos.GetHash() == key) << " " << pos.fen() << endl;
	move = createMove(E4, E5);
	pos.ApplyMove(move);
	key = 0x662fafb965db29d4;
	cout << pos.GetHash() << " - " << key << " " << (pos.GetHash() == key) << " " << pos.fen() << endl;
	move = createMove(F7, F5);
	pos.ApplyMove(move);
	key = 0x22a48b5a8e47ff78;
	cout << pos.GetHash() << " - " << key << " " << (pos.GetHash() == key) << " " << pos.fen() << endl;
	move = createMove(E1, E2);
	pos.ApplyMove(move);
	key = 0x652a607ca3f242c1;
	cout << pos.GetHash() << " - " << key << " " << (pos.GetHash() == key) << " " << pos.fen() << endl;
	move = createMove(E8, F7);
	pos.ApplyMove(move);
	key = 0x00fdd303c946bdd9;
	cout << pos.GetHash() << " - " << key << " " << (pos.GetHash() == key) << " " << pos.fen() << endl;
}

uint64_t perftNodes = 0;
uint64_t testCount = 0;
chrono::microseconds perftRuntime;
bool checkPerft(string fen, int depth, uint64_t expectedResult, PerftType perftType = BASIC) {
	testCount++;
	position pos(fen);
	uint64_t perftResult;
	chrono::system_clock::time_point begin = chrono::high_resolution_clock::now();
	switch (perftType) {
	case BASIC:
		perftResult = perft(pos, depth); break;
	case P1:
		perftResult = perft1(pos, depth); break;
	case P2:
		perftResult = perft2(pos, depth); break;
	case P3:
		perftResult = perft3(pos, depth); break;
	}
	chrono::system_clock::time_point end = chrono::high_resolution_clock::now();
	auto runtime = end - begin;
	chrono::microseconds runtimeMS = chrono::duration_cast<chrono::microseconds>(runtime);
	perftRuntime += runtimeMS;
	perftNodes += expectedResult;
	if (perftResult == expectedResult) {
		if (runtimeMS.count() > 0) {
			cout << testCount << "\t" << "OK\t" << depth << "\t" << perftResult << "\t" << runtimeMS.count() / 1000 << " ms\t" << expectedResult / runtimeMS.count() << " MNodes/s\t" << endl << "\t" << fen << endl;
		}
		else {
			cout << testCount << "\t" << "OK\t" << depth << "\t" << perftResult << "\t" << runtimeMS.count() / 1000 << " ms\t" << endl << "\t" << fen << endl;
		}
		return true;
	}
	else {
		cout << testCount << "\t" << "Error\t" << depth << "\t" << perftResult << "\texpected\t" << expectedResult << "\t" << fen << endl;
		return false;
	}
}

bool testPerft(PerftType perftType) {
	cout << "Starting Perft Suite in Mode " << perftType << endl;
	bool result = true;
	testCount = 0;
	perftNodes = 0;
	perftRuntime = perftRuntime.zero();
	cout << "Initial Position" << endl;
	result = result && checkPerft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20, perftType);
	result = result && checkPerft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400, perftType);
	result = result && checkPerft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902, perftType);
	result = result && checkPerft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281, perftType);
	result = result && checkPerft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609, perftType);
	result = result && checkPerft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324, perftType);
	cout << "Kiwi Pete" << endl;
	result = result && checkPerft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48, perftType);
	result = result && checkPerft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039, perftType);
	result = result && checkPerft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862, perftType);
	result = result && checkPerft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603, perftType);
	result = result && checkPerft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690, perftType);
	cout << "Chess 960 Positions" << endl;
	result = result && checkPerft("nrbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/NRBKQBNR w KQkq - 0 1", 1, 19, perftType);
	result = result && checkPerft("nrbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/NRBKQBNR w KQkq - 0 1", 2, 361, perftType);
	result = result && checkPerft("nrbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/NRBKQBNR w KQkq - 0 1", 3, 7735, perftType);
	result = result && checkPerft("nrbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/NRBKQBNR w KQkq - 0 1", 4, 164966, perftType);
	result = result && checkPerft("nrbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/NRBKQBNR w KQkq - 0 1", 5, 3962549, perftType);
	result = result && checkPerft("nrbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/NRBKQBNR w KQkq - 0 1", 6, 94328606, perftType);
	result = result && checkPerft("rn2k1r1/ppp1pp1p/3p2p1/5bn1/P7/2N2B2/1PPPPP2/2BNK1RR w Gkq - 4 11", 1, 33, perftType);
	result = result && checkPerft("rn2k1r1/ppp1pp1p/3p2p1/5bn1/P7/2N2B2/1PPPPP2/2BNK1RR w Gkq - 4 11", 2, 1072, perftType);
	result = result && checkPerft("rn2k1r1/ppp1pp1p/3p2p1/5bn1/P7/2N2B2/1PPPPP2/2BNK1RR w Gkq - 4 11", 3, 35141, perftType);
	result = result && checkPerft("rn2k1r1/ppp1pp1p/3p2p1/5bn1/P7/2N2B2/1PPPPP2/2BNK1RR w Gkq - 4 11", 4, 1111449, perftType);
	result = result && checkPerft("rn2k1r1/ppp1pp1p/3p2p1/5bn1/P7/2N2B2/1PPPPP2/2BNK1RR w Gkq - 4 11", 5, 37095094, perftType);
	result = result && checkPerft("2rkr3/5PP1/8/5Q2/5q2/8/5pp1/2RKR3 w KQkq - 0 1", 5, 94370149, perftType);
	result = result && checkPerft("rqkrbnnb/pppppppp/8/8/8/8/PPPPPPPP/RQKRBNNB w KQkq - 0 1", 6, 111825069, perftType);
	cout << "Standard Chess positions" << endl;
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 1, 15, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 2, 66, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 3, 1197, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 4, 7059, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 5, 133987, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 6, 764643, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 1, 16, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 2, 71, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 3, 1287, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 4, 7626, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 5, 145232, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 846648, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 1, 5, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 2, 75, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 3, 459, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 4, 8290, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 5, 47635, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 6, 899442, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 1, 5, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 2, 80, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 3, 493, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 4, 8897, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 5, 52710, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 6, 1001523, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 1, 26, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 2, 112, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 3, 3189, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 4, 17945, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 5, 532933, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 6, 2788982, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 1, 5, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 2, 130, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 3, 782, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 4, 22180, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 5, 118882, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 6, 3517770, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 1, 12, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 2, 38, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 3, 564, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 4, 2219, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 5, 37735, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 6, 185867, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 1, 15, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 2, 65, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 3, 1018, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 4, 4573, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 5, 80619, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 6, 413018, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 1, 3, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 2, 32, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 3, 134, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 4, 2073, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 5, 10485, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 6, 179869, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 1, 4, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 2, 49, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 3, 243, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 4, 3991, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 5, 20780, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 6, 367724, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 1, 26, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 2, 568, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 3, 13744, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 4, 314346, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 5, 7594526, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 6, 179862938, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 1, 25, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 2, 567, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 3, 14095, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 4, 328965, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 5, 8153719, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 6, 195629489, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 1, 25, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 2, 548, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 3, 13502, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 4, 312835, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 5, 7736373, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 6, 184411439, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 1, 25, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 2, 547, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 3, 13579, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 4, 316214, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 5, 7878456, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 6, 189224276, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 1, 26, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 2, 583, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 3, 14252, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 4, 334705, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 5, 8198901, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 6, 198328929, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 1, 25, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 2, 560, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 3, 13592, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 4, 317324, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 5, 7710115, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 6, 185959088, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 1, 25, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 2, 560, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 3, 13607, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 4, 320792, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 5, 7848606, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 6, 190755813, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 1, 5, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 2, 75, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 3, 459, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 4, 8290, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 5, 47635, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 6, 899442, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 1, 5, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 2, 80, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 3, 493, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 4, 8897, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 5, 52710, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 6, 1001523, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 1, 15, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 2, 66, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 3, 1197, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 4, 7059, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 5, 133987, perftType);
	result = result && checkPerft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 6, 764643, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 1, 16, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 2, 71, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 3, 1287, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 4, 7626, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 5, 145232, perftType);
	result = result && checkPerft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 6, 846648, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 1, 5, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 2, 130, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 3, 782, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 4, 22180, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 5, 118882, perftType);
	result = result && checkPerft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 6, 3517770, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 1, 26, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 2, 112, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 3, 3189, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 4, 17945, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 5, 532933, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 6, 2788982, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 1, 3, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 2, 32, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 3, 134, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 4, 2073, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 5, 10485, perftType);
	result = result && checkPerft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 6, 179869, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 1, 4, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 2, 49, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 3, 243, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 4, 3991, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 5, 20780, perftType);
	result = result && checkPerft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 6, 367724, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 1, 12, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 2, 38, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 3, 564, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 4, 2219, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 5, 37735, perftType);
	result = result && checkPerft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 6, 185867, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 1, 15, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 2, 65, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 3, 1018, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 4, 4573, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 5, 80619, perftType);
	result = result && checkPerft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 6, 413018, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 1, 26, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 2, 568, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 3, 13744, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 4, 314346, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 5, 7594526, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 6, 179862938, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 1, 26, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 2, 583, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 3, 14252, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 4, 334705, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 5, 8198901, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 6, 198328929, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 1, 25, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 2, 560, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 3, 13592, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 4, 317324, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 5, 7710115, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 6, 185959088, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 1, 25, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 2, 560, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 3, 13607, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 4, 320792, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 5, 7848606, perftType);
	result = result && checkPerft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 6, 190755813, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 1, 25, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 2, 567, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 3, 14095, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 4, 328965, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 5, 8153719, perftType);
	result = result && checkPerft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 6, 195629489, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 1, 25, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 2, 548, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 3, 13502, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 4, 312835, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 5, 7736373, perftType);
	result = result && checkPerft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 6, 184411439, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 1, 25, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 2, 547, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 3, 13579, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 4, 316214, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 5, 7878456, perftType);
	result = result && checkPerft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 6, 189224276, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 1, 14, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 2, 195, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 3, 2760, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 4, 38675, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 5, 570726, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 6, 8107539, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 1, 11, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 2, 156, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 3, 1636, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 4, 20534, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 5, 223507, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 6, 2594412, perftType);
	result = result && checkPerft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 1, 19, perftType);
	result = result && checkPerft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 2, 289, perftType);
	result = result && checkPerft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 3, 4442, perftType);
	result = result && checkPerft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 4, 73584, perftType);
	result = result && checkPerft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 5, 1198299, perftType);
	result = result && checkPerft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 6, 19870403, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 1, 3, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 2, 51, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 3, 345, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 4, 5301, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 5, 38348, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 6, 588695, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 1, 17, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 2, 54, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 3, 835, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 4, 5910, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 5, 92250, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 6, 688780, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 1, 15, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 2, 193, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 3, 2816, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 4, 40039, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 5, 582642, perftType);
	result = result && checkPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 6, 8503277, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 1, 16, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 2, 180, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 3, 2290, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 4, 24640, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 5, 288141, perftType);
	result = result && checkPerft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 6, 3147566, perftType);
	result = result && checkPerft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 2, 68, perftType);
	result = result && checkPerft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 3, 1118, perftType);
	result = result && checkPerft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 4, 16199, perftType);
	result = result && checkPerft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 5, 281190, perftType);
	result = result && checkPerft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 6, 4405103, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 1, 17, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 2, 54, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 3, 835, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 4, 5910, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 5, 92250, perftType);
	result = result && checkPerft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 6, 688780, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 1, 3, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 2, 51, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 3, 345, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 4, 5301, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 5, 38348, perftType);
	result = result && checkPerft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 6, 588695, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 1, 17, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 2, 278, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 3, 4607, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 4, 76778, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 5, 1320507, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 6, 22823890, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 1, 21, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 2, 316, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 3, 5744, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 4, 93338, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 5, 1713368, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 6, 28861171, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 1, 21, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 2, 144, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 3, 3242, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 4, 32955, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 5, 787524, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 6, 7881673, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 1, 7, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 2, 143, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 3, 1416, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 4, 31787, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 5, 310862, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 6, 7382896, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 1, 6, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 2, 106, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 3, 1829, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 4, 31151, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 5, 530585, perftType);
	result = result && checkPerft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 6, 9250746, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 1, 17, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 2, 309, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 3, 5133, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 4, 93603, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 5, 1591064, perftType);
	result = result && checkPerft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 6, 29027891, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 1, 7, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 2, 143, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 3, 1416, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 4, 31787, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 5, 310862, perftType);
	result = result && checkPerft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 6, 7382896, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 1, 21, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 2, 144, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 3, 3242, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 4, 32955, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 5, 787524, perftType);
	result = result && checkPerft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 6, 7881673, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 1, 19, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 2, 275, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 3, 5300, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 4, 104342, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 5, 2161211, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 6, 44956585, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 1, 36, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 2, 1027, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 3, 29215, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 4, 771461, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 5, 20506480, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 6, 525169084, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 1, 19, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 2, 275, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 3, 5300, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 4, 104342, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 5, 2161211, perftType);
	result = result && checkPerft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 6, 44956585, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 1, 36, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 2, 1027, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 3, 29227, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 4, 771368, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 5, 20521342, perftType);
	result = result && checkPerft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 6, 524966748, perftType);
	result = result && checkPerft("6kq/8/8/8/8/8/8/7K w - - 0 1", 1, 2, perftType);
	result = result && checkPerft("6kq/8/8/8/8/8/8/7K w - - 0 1", 2, 36, perftType);
	result = result && checkPerft("6kq/8/8/8/8/8/8/7K w - - 0 1", 3, 143, perftType);
	result = result && checkPerft("6kq/8/8/8/8/8/8/7K w - - 0 1", 4, 3637, perftType);
	result = result && checkPerft("6kq/8/8/8/8/8/8/7K w - - 0 1", 5, 14893, perftType);
	result = result && checkPerft("6kq/8/8/8/8/8/8/7K w - - 0 1", 6, 391507, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 1, 2, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 2, 36, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 3, 143, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 4, 3637, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 5, 14893, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 6, 391507, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 1, 6, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 2, 35, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 3, 495, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 4, 8349, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 5, 166741, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 6, 3370175, perftType);
	result = result && checkPerft("6qk/8/8/8/8/8/8/7K b - - 0 1", 1, 22, perftType);
	result = result && checkPerft("6qk/8/8/8/8/8/8/7K b - - 0 1", 2, 43, perftType);
	result = result && checkPerft("6qk/8/8/8/8/8/8/7K b - - 0 1", 3, 1015, perftType);
	result = result && checkPerft("6qk/8/8/8/8/8/8/7K b - - 0 1", 4, 4167, perftType);
	result = result && checkPerft("6qk/8/8/8/8/8/8/7K b - - 0 1", 5, 105749, perftType);
	result = result && checkPerft("6qk/8/8/8/8/8/8/7K b - - 0 1", 6, 419369, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 1, 2, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 2, 36, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 3, 143, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 4, 3637, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 5, 14893, perftType);
	result = result && checkPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 6, 391507, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 1, 6, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 2, 35, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 3, 495, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 4, 8349, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 5, 166741, perftType);
	result = result && checkPerft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 6, 3370175, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 1, 3, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 2, 7, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 3, 43, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 4, 199, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 5, 1347, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 6, 6249, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k w - - 0 1", 1, 3, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k w - - 0 1", 2, 7, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k w - - 0 1", 3, 43, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k w - - 0 1", 4, 199, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k w - - 0 1", 5, 1347, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k w - - 0 1", 6, 6249, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 1, 1, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 2, 3, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 3, 12, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 4, 80, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 5, 342, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 6, 2343, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 1, 1, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 2, 3, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 3, 12, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 4, 80, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 5, 342, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 6, 2343, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 1, 7, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 2, 35, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 3, 210, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 4, 1091, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 5, 7028, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 6, 34834, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 1, 1, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 2, 3, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 3, 12, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 4, 80, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 5, 342, perftType);
	result = result && checkPerft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 6, 2343, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k b - - 0 1", 1, 1, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k b - - 0 1", 2, 3, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k b - - 0 1", 3, 12, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k b - - 0 1", 4, 80, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k b - - 0 1", 5, 342, perftType);
	result = result && checkPerft("8/8/8/8/8/7K/7P/7k b - - 0 1", 6, 2343, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 1, 3, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 2, 7, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 3, 43, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 4, 199, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 5, 1347, perftType);
	result = result && checkPerft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 6, 6249, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 1, 3, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 2, 7, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 3, 43, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 4, 199, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 5, 1347, perftType);
	result = result && checkPerft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 6, 6249, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 2, 35, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 3, 182, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 4, 1091, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 5, 5408, perftType);
	result = result && checkPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 6, 34822, perftType);
	result = result && checkPerft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 1, 2, perftType);
	result = result && checkPerft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 2, 8, perftType);
	result = result && checkPerft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 3, 44, perftType);
	result = result && checkPerft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 4, 282, perftType);
	result = result && checkPerft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 5, 1814, perftType);
	result = result && checkPerft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 6, 11848, perftType);
	result = result && checkPerft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 1, 2, perftType);
	result = result && checkPerft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 2, 8, perftType);
	result = result && checkPerft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 3, 44, perftType);
	result = result && checkPerft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 4, 282, perftType);
	result = result && checkPerft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 5, 1814, perftType);
	result = result && checkPerft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 6, 11848, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 1, 3, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 2, 9, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 3, 57, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 4, 360, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 5, 1969, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 6, 10724, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 1, 3, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 2, 9, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 3, 57, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 4, 360, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 5, 1969, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 6, 10724, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 2, 25, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 3, 180, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 4, 1294, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 5, 8296, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 6, 53138, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 1, 8, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 2, 61, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 3, 483, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 4, 3213, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 5, 23599, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 6, 157093, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 1, 8, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 2, 61, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 3, 411, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 4, 3213, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 5, 21637, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 6, 158065, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 2, 15, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 3, 90, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 4, 534, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 5, 3450, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 6, 20960, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 1, 3, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 2, 9, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 3, 57, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 4, 360, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 5, 1969, perftType);
	result = result && checkPerft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 6, 10724, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 1, 3, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 2, 9, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 3, 57, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 4, 360, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 5, 1969, perftType);
	result = result && checkPerft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 6, 10724, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 2, 25, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 3, 180, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 4, 1294, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 5, 8296, perftType);
	result = result && checkPerft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 6, 53138, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 1, 8, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 2, 61, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 3, 411, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 4, 3213, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 5, 21637, perftType);
	result = result && checkPerft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 6, 158065, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 1, 8, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 2, 61, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 3, 483, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 4, 3213, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 5, 23599, perftType);
	result = result && checkPerft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 6, 157093, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 2, 15, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 3, 89, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 4, 537, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 5, 3309, perftType);
	result = result && checkPerft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 6, 21104, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 1, 4, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 2, 19, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 3, 117, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 4, 720, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 5, 4661, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 6, 32191, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 2, 19, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 3, 116, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 4, 716, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 5, 4786, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 6, 30980, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 2, 22, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 3, 139, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 4, 877, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 2, 16, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 3, 101, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 4, 637, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 2, 22, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 3, 139, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 4, 877, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 2, 16, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 3, 101, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 4, 637, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 1, 3, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 2, 15, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 3, 84, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 4, 573, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 5, 3013, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 6, 22886, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 2, 16, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 3, 101, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 4, 637, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 5, 4271, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 6, 28662, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 2, 19, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 3, 117, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 4, 720, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 5, 5014, perftType);
	result = result && checkPerft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 6, 32167, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 2, 19, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 3, 117, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 4, 712, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 5, 4658, perftType);
	result = result && checkPerft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 6, 30749, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 2, 22, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 3, 139, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 4, 877, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 2, 16, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 3, 101, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 4, 637, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 2, 22, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 3, 139, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 4, 877, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 2, 16, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 3, 101, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 4, 637, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 2, 15, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 3, 102, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 4, 569, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 5, 4337, perftType);
	result = result && checkPerft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 6, 22579, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 2, 16, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 3, 101, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 4, 637, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 5, 4271, perftType);
	result = result && checkPerft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 6, 28662, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 2, 22, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 3, 139, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 4, 877, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 1, 4, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 2, 16, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 3, 101, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 4, 637, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 2, 22, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 3, 139, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 4, 877, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 1, 4, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 2, 16, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 3, 101, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 4, 637, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 2, 25, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 3, 161, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 4, 1035, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 5, 7574, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 6, 55338, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 2, 25, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 3, 161, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 4, 1035, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 5, 7574, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 6, 55338, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 1, 7, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 2, 49, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 3, 378, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 4, 2902, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 5, 24122, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 6, 199002, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 2, 22, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 3, 139, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 4, 877, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 2, 16, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 3, 101, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 4, 637, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 2, 22, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 3, 139, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 4, 877, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 5, 6112, perftType);
	result = result && checkPerft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 6, 41874, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 1, 4, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 2, 16, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 3, 101, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 4, 637, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 5, 4354, perftType);
	result = result && checkPerft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 6, 29679, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 2, 25, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 3, 161, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 4, 1035, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 5, 7574, perftType);
	result = result && checkPerft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 6, 55338, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 1, 5, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 2, 25, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 3, 161, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 4, 1035, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 5, 7574, perftType);
	result = result && checkPerft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 6, 55338, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 1, 7, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 2, 49, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 3, 378, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 4, 2902, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 5, 24122, perftType);
	result = result && checkPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 6, 199002, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 1, 11, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 2, 97, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 3, 887, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 4, 8048, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 5, 90606, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 6, 1030499, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 1, 24, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 2, 421, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 3, 7421, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 4, 124608, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 5, 2193768, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 6, 37665329, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 1, 18, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 2, 270, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 3, 4699, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 4, 79355, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 5, 1533145, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 6, 28859283, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 1, 24, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 2, 496, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 3, 9483, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 4, 182838, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 5, 3605103, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 6, 71179139, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 1, 11, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 2, 97, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 3, 887, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 4, 8048, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 5, 90606, perftType);
	result = result && checkPerft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 6, 1030499, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 1, 24, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 2, 421, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 3, 7421, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 4, 124608, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 5, 2193768, perftType);
	result = result && checkPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 6, 37665329, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 1, 18, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 2, 270, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 3, 4699, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 4, 79355, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 5, 1533145, perftType);
	result = result && checkPerft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 6, 28859283, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 1, 24, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 2, 496, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 3, 9483, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 4, 182838, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 5, 3605103, perftType);
	result = result && checkPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 6, 71179139, perftType);
	if (result) cout << "Done OK" << endl; else cout << "Error!" << endl;
	cout << "Runtime: " << setprecision(3) << perftRuntime.count() / 1000000.0 << " s" << endl;
	cout << "Count:   " << setprecision(3) << perftNodes / 1000000.0 << " MNodes" << endl;
	cout << "Leafs: " << setprecision(3) << (1.0 * perftNodes) / perftRuntime.count() << " MNodes/s" << endl;
	cout << "Nodes: " << setprecision(3) << (1.0 * nodeCount) / perftRuntime.count() << " MNodes/s" << endl;
	return result;
}

bool testSEE() {
	position pos("1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - ");
	Value see = pos.SEE(E1, E5);
	bool result = true;
	if (see > 0) cout << "OK\t"; else {
		cout << "ERROR\t";
		result = false;
	}
	cout << "SEE: " << see << "\t1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - \t" << toString(createMove(E1, E5)) << endl;
	position pos2("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -");
	see = pos2.SEE(D3, E5);
	if (see < 0) cout << "OK\t"; else {
		cout << "ERROR\t";
		result = false;
	}
	cout << "SEE: " << see << "\t1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - \t" << toString(createMove(D3, E5)) << endl;

	position pos3("2k1r3/1pp4p/p5p1/8/4P3/P7/1PP4P/1K1R4 b - - 0 1");
	see = pos3.SEE(E8, E4);
	if (see > 0) cout << "OK\t"; else {
		cout << "ERROR\t";
		result = false;
	}
	cout << "SEE: " << see << "\t2k1r3/1pp4p/p5p1/8/4P3/P7/1PP4P/1K1R4 b - - 0 1\t" << toString(createMove(E8, E4)) << endl;

	position pos4("2k1q3/1pp1r1bp/p2n2p1/8/4P3/P4B2/1PPN3P/1K1R3Q b - - 0 1");
	see = pos4.SEE(D6, E4);
	if (see < 0) cout << "OK\t"; else {
		cout << "ERROR\t";
		result = false;
	}
	cout << "SEE: " << see << "\t2k1q3/1pp1r1bp/p2n2p1/8/4P3/P4B2/1PPN3P/1K1R3Q b - - 0 1 \t" << toString(createMove(D6, E4)) << endl;

	position pos5("1k6/8/8/3pRrRr/8/8/8/1K6 w - - 0 1");
	see = pos5.SEE(E5, D5);
	if (see < 0) cout << "OK\t"; else {
		cout << "ERROR\t";
		result = false;
	}
	cout << "SEE: " << see << "\t1k6/8/8/3pRrRr/8/8/8/1K6 w - - 0 1  \t" << toString(createMove(E5, D5)) << endl;

	position pos6("1k6/8/8/8/3PrRrR/8/8/1K6 b - - 0 1");
	see = pos6.SEE(E4, D4);
	if (see < 0) cout << "OK\t"; else {
		cout << "ERROR\t";
		result = false;
	}
	cout << "SEE: " << see << "\t1k6/8/8/8/3PrRrR/8/8/1K6 b - - 0 1 \t" << toString(createMove(E4, D4)) << endl;
	return result;
}

vector<string> readTextFile(string file) {
	vector<string> lines;
	ifstream s;
	s.open(file);
	string line;
	if (s.is_open())
	{
		long lineCount = 0;
		while (s)
		{
			getline(s, line);
			lines.push_back(line);
			lineCount++;
			if ((lineCount & 8191) == 0) cout << ".";
		}
		s.close();
		cout << endl;
		cout << lineCount << " lines read!" << endl;
	}
	else
	{
		cout << "Unable to open file" << std::endl;
	}
	return lines;
}