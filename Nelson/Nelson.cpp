// Nelson.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <iostream>
#include "stdlib.h"
#include "types.h"
#include "board.h"
#include "position.h"
#include "test.h"
#include "uci.h"


static bool popcountSupport();
static bool is64Bit();

int main(int argc, const char* argv[]) {
#ifdef _MSC_VER
#ifndef _WIN64
	cout << "Only 64 bit operating systems are supported!" << std::endl;
	return 0;
#endif
#endif // _MSC_VER
#ifdef __GNUC__
//#ifndef __x86_64__
//	cout << "Only 64 bit operating systems are supported!" << std::endl;
//	return 0;
//#endif
#endif // __GNUC__
	if (!popcountSupport()) {
		std::cout << "No Popcount support - Engine does't work on this hardware!" << std::endl;
		return 0;
	}
	std::cout << "Compiled on: " << __DATE__ << " " << __TIME__ << std::endl;
	Initialize();
	//bench2(7);
	//return 0;
	if (argc <= 1) loop();
	else {
		std::string func(argv[1]);
		if (!func.compare("test")) {
			std::string input = "";
			//std::getline(cin, input);
			//testPerft();
			//return 0;
			std::cout << sizeof(position) << std::endl;
			position pos;
			std::cout << pos.GetMaterialKey() << std::endl;
			while (true) {
				std::getline(std::cin, input);
				if (!input.compare(0, 4, "quit")) break;
				else if (!input.compare(0, 8, "position")) {
					std::string fen;
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
				else if (!input.compare(0, 6, "bench2")) {
					int depth = 10;
					if (input.length() > 7) depth = atoi(input.substr(7).c_str());
					bench2(depth);
				}
				else if (!input.compare(0, 5, "bench")) {
					int depth = 10;
					if (input.length() > 6) depth = atoi(input.substr(6).c_str());
					bench(depth);
				}
				else if (!input.compare(0, 3, "SEE")) {
					testSEE();
				}
				else if (!input.compare(0, 5, "print")) {
					std::cout << pos.print() << std::endl;
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
				else if (!input.compare(0, 10, "repetition")) {
					testRepetition();
				}
				else if (!input.compare(0, 3, "kpk")) {
					testKPK();
				}
				else if (!input.compare(0, 7, "threads")) {
					if (input.length() > 8) HelperThreads = atoi(input.substr(8).c_str()) - 1;
				}
			}
			std::getline(std::cin, input);
			return 0;
		}
	}
}

#ifdef _MSC_VER
static bool popcountSupport() {
	int cpuInfo[4];
	int functionId = 0x00000001;
	__cpuid(cpuInfo, functionId);
	return (cpuInfo[2] & (1 << 23)) != 0;
}
#endif // _MSC_VER
#ifdef __GNUC__
#define cpuid(func,ax,bx,cx,dx)\
	__asm__ __volatile__ ("cpuid":\
	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
static bool popcountSupport() {
    int cpuInfo[4];
    cpuid(0x00000001, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
 	return (cpuInfo[2] & (1 << 23)) != 0;
}
#endif // __GNUC__

