// Nelson.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <iostream>
#include "types.h"
#include "board.h"
#include "position.h"
#include "test.h"
#include "uci.h"


int main(int argc, const char* argv[]) {
	cout << "Compiled on: " << __DATE__ << " " << __TIME__ << endl;
	Initialize();
	//position p("1B1Nrb2/1N1R1n2/Kp4r1/pp1p1Bp1/R1Pk4/3Pp1Q1/1P1p1P2/6qb w - - 0 1");
	//testMateInDos();
	//return 0;
	//bench(8);
	//return 0;
	if (argc <= 1) loop();
	else {
		string func(argv[1]);
		if (!func.compare("test")) {
			string input = "";
			Initialize();
			//std::getline(cin, input);
			//testPerft();
			//return 0;
			std::cout << sizeof(position) << std::endl;
			position pos;
			std::cout << pos.GetMaterialKey() << std::endl;
			while (true) {
				string input;
				std::getline(cin, input);
				if (!input.compare(0, 4, "quit")) break;
				else if (!input.compare(0, 8, "position")) {
					string fen;
					if (input.length() < 10) fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
					else fen = input.substr(9);
					pos.setFromFEN(fen);
				}
				else if (!input.compare(0, 7, "divide ")) {
					int depth = atoi(input.substr(7).c_str());
					divide(pos, depth);
				}
				else if (!input.compare(0, 10, "perftSuite")) {
					PerftType pt = BASIC;
					if (input.length() > 11) {
						int pti = atoi(input.substr(11).c_str());
						pt = PerftType(pti);
					}
					testPerft(pt);
				}
				else if (!input.compare(0, 6, "perft ")) {
					int depth = atoi(input.substr(6).c_str());
					std::cout << "Perft:\t" << perft(pos, depth) << "\t" << pos.fen() << std::endl;
				}
				else if (!input.compare(0, 5, "bench")) {
					int depth = 7;
					if (input.length() > 6) depth = atoi(input.substr(6).c_str());
					bench(depth);
				}
				else if (!input.compare(0, 3, "SEE")) {
					testSEE();
				}
				else if (!input.compare(0, 5, "print")) {
					cout << pos.print() << endl;
				}
				else if (!input.compare(0, 3, "qcg")) {
					testCheckQuietCheckMoveGeneration();
				}
				else if (!input.compare(0, 3, "tmg")) {
					testTacticalMoveGeneration();
				}
				else if (!input.compare(0, 7, "search ")) {
					int depth = atoi(input.substr(7).c_str());
					testSearch(pos, depth);
				}
				else if (!input.compare(0, 6, "result")) {
					testResult();
				}
				else if (!input.compare(0, 8, "findmate")) {
					testFindMate();
				}
				else if (!input.compare(0, 7, "mattin2")) {
					testMateInDos();
				}
			}
			std::getline(cin, input);
			return 0;
		}
	}
}

