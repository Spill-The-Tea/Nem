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
#include "utils.h"
#include "xboard.h"

#ifdef TB
#include "syzygy/tbprobe.h"
#endif


static bool popcountSupport();

int main(int argc, const char* argv[]) {
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
	std::cout << "Nemorino 0.46" << std::endl;
	//std::cout << "Compiled on: " << __DATE__ << " " << __TIME__ << std::endl;
	Initialize();
	//bench2(7);
	//return 0;
	std::string input = "";
	if (argc <= 1) {
		std::getline(std::cin, input);
		if (!input.compare(0, 3, "uci")) {
			protocol = UCI;
			UCIInterface uciInterface;
			uciInterface.loop();
			return 0;
		}
		else if (!input.compare(0, 6, "xboard")) {
			protocol = XBOARD;
			cecp::xboard xb;
			xb.loop();
			return 0;
		}
	}
	else {
		std::string func(argv[1]);
		if (!func.compare("test")) {			
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
					test::divide(pos, depth);
				}
				else if (!input.compare(0, 8, "divide3 ")) {
					int depth = atoi(input.substr(8).c_str());
					test::divide3(pos, depth);
				}
				else if (!input.compare(0, 10, "perftSuite")) {
					test::PerftType pt = test::BASIC;
					if (input.length() > 11) {
						int pti = atoi(input.substr(11).c_str());
						pt = test::PerftType(pti);
					}
					test::testPerft(pt);
				}
				else if (!input.compare(0, 6, "perft ")) {
					int depth = atoi(input.substr(6).c_str());
					std::cout << "Perft:\t" << test::perft(pos, depth) << "\t" << pos.fen() << std::endl;
				}
				else if (!input.compare(0, 7, "perft3 ")) {
					int depth = atoi(input.substr(7).c_str());
					std::cout << "Perft:\t" << test::perft3(pos, depth) << "\t" << pos.fen() << std::endl;
				}
				else if (!input.compare(0, 6, "bench2")) {
					int depth = 11;
					if (input.length() > 7) depth = atoi(input.substr(7).c_str());
					test::bench2(depth);
				}
				else if (!input.compare(0, 5, "bench")) {
					int depth = 11;
					if (input.length() > 6) depth = atoi(input.substr(6).c_str());
					test::bench(depth);
				}
				else if (!input.compare(0, 3, "SEE")) {
					test::testSEE();
				}
				else if (!input.compare(0, 5, "print")) {
					std::cout << pos.print() << std::endl;
				}
				else if (!input.compare(0, 3, "qcg")) {
					test::testCheckQuietCheckMoveGeneration();
				}
				else if (!input.compare(0, 3, "tmg")) {
					test::testTacticalMoveGeneration();
				}
				else if (!input.compare(0, 7, "search ")) {
					int depth = atoi(input.substr(7).c_str());
					test::testSearch(pos, depth);
				}
				else if (!input.compare(0, 6, "result")) {
					test::testResult();
				}
				else if (!input.compare(0, 8, "findmate")) {
					test::testFindMate();
				}
				else if (!input.compare(0, 7, "mattin2")) {
					test::testMateInDos();
				}
				else if (!input.compare(0, 10, "repetition")) {
					test::testRepetition();
				}
				else if (!input.compare(0, 3, "kpk")) {
					test::testKPK();
				}
				else if (!input.compare(0, 7, "threads")) {
					if (input.length() > 8) {
						HelperThreads = atoi(input.substr(8).c_str()) - 1;
					}
				}
				else if (!input.compare("setoption name UCI_Chess960 value true")) {
					Chess960 = true;
				}
#ifdef TB
				else if (!input.compare(0, 6, "tbtest")) {
					SYZYGY_PATH = "C:/TB/syzygy";
					Tablebases::init(SYZYGY_PATH);
					test::testTB();
				}
#endif
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

