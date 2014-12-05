// Nelson.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <iostream>
#include "types.h"
#include "board.h"
#include "position.h"
#include "test.h"


int main(int argc, const char* argv[]) {
	string input = "";
	//testPolyglotKey();
	//std::getline(cin, input);
	//return 0;
	Initialize();
	testPerft();
	return 0;
	std::cout << sizeof(position) << std::endl;
	position pos;
	while (true) {
		string input = "";
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
			testPerft();
		}
		else if (!input.compare(0, 6, "perft ")) {
			int depth = atoi(input.substr(6).c_str());
			std::cout << "Perft:\t" << perft(pos, depth) << "\t" << pos.fen() << std::endl;
		}
	}
	std::getline(cin, input);
	return 0;
}

