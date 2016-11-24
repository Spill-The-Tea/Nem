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
#include "tablebase.h"
#endif

const int MAJOR_VERSION = 1;
const int MINOR_VERSION = 17;


static bool popcountSupport();

int main(int argc, const char* argv[]) {
#ifndef NO_POPCOUNT
	if (!popcountSupport()) {
		std::cout << "No Popcount support - Engine does't work on this hardware!" << std::endl;
		return 0;
	}
#endif // !1
	if (argc > 1 && argv[1]) {
		std::string arg1(argv[1]);
		if (!arg1.compare("bench")) {
			Initialize();
			int depth = 11;
			test::benchmark(depth);
			return 0;
		}
	}
	std::string input = "";
	position pos;
	while (true) {
		std::getline(std::cin, input);
		if (!input.compare(0, 3, "uci")) {
			Initialize();
			protocol = UCI;
			UCIInterface uciInterface;
			uciInterface.loop();
			return 0;
		}
		else if (!input.compare(0, 6, "xboard")) {
			Initialize();
			protocol = XBOARD;
			cecp::xboard xb;
			xb.loop();
			return 0;
		}
		else if (!input.compare(0, 7, "version")) {
			std::cout << "Nemorino " << MAJOR_VERSION << "." << std::setfill('0') << std::setw(2) << MINOR_VERSION << std::endl;
		}
		else if (!input.compare(0, 8, "position")) {
			Initialize();
			std::string fen;
			if (input.length() < 10) fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
			else fen = input.substr(9);
			pos.setFromFEN(fen);
		}
		else if (!input.compare(0, 6, "perft ")) {
			Initialize();
			int depth = atoi(input.substr(6).c_str());
			std::cout << "Perft:\t" << test::perft(pos, depth) << "\t" << pos.fen() << std::endl;
		}
		else if (!input.compare(0, 7, "divide ")) {
			Initialize();
			int depth = atoi(input.substr(7).c_str());
			test::divide(pos, depth);
		}
		else if (!input.compare(0, 5, "bench")) {
			Initialize();
			int depth = 11;
			if (input.length() > 6) depth = atoi(input.substr(6).c_str());
			test::benchmark(depth);
		}
		else if (!input.compare(0, 5, "print")) {
			Initialize();
			std::cout << pos.print() << std::endl;
		}
		else if (!input.compare(0, 4, "help")) {
			int width = 20;
			std::cout << "Nemorino is a chess engine supporting both UCI and XBoard protocol. To play" << std::endl;
			std::cout << "chess with it you need a GUI (like arena or winboard)" << std::endl;
			std::cout << "License: GNU GENERAL PUBLIC LICENSE v3 (https://www.gnu.org/licenses/gpl.html)" << std::endl << std::endl;
			std::cout << std::left << std::setw(width) << "uci" << "Starts engine in UCI mode" << std::endl;
			std::cout << std::left << std::setw(width) << "xboard" << "Starts engine in XBoard mode" << std::endl;
			std::cout << std::left << std::setw(width) << "version" << "Outputs the current version of the engine" << std::endl;
			std::cout << std::left << std::setw(width) << "position <fen>" << "Sets the engine's position using the FEN-String <fen>" << std::endl;
			std::cout << std::left << std::setw(width) << "perft <depth>" << "Calculates the Perft count of depth <depth> for the current position" << std::endl;
			std::cout << std::left << std::setw(width) << "divide <depth>" << "Calculates the Perft count at depth <depth> - 1 for all legal moves" << std::endl;
			std::cout << std::left << std::setw(width) << "bench [<depth>]" << "Searches a fixed set of position at fixed depth <depth> (default depth is 11)" << std::endl;
			std::cout << std::left << std::setw(width) << "print" << "Outputs the internal board with some information like evaluation and hash keys" << std::endl;
			std::cout << std::left << std::setw(width) << "help" << "Prints out this help lines" << std::endl;
			std::cout << std::left << std::setw(width) << "quit" << "Exits the program" << std::endl;
		}
		else if (!input.compare(0, 4, "quit")) break;
		else if (!input.compare(0, 8, "setvalue")) {
			std::vector<std::string> token = utils::split(input);
			int indx = 1;
			while (indx < (int)token.size() - 1) {
				settings::options.set(token[indx], token[indx + 1]);
				indx += 2;
			}
		}
#ifdef TUNE
		else if (!input.compare(0, 10, "parameters")) {
			std::vector<std::string> token = utils::split(input);
			settings::processParameter(token);
		}
#endif
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

