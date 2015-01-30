#include <thread>
#include <fstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include "uci.h"
#include "position.h"
#include "board.h"
#include "search.h"
#include "test.h"
#include "settings.h"


search Engine;
position * pos = nullptr;
std::thread * Mainthread = nullptr;

void dispatch(char *line);

// UCI command handlers
void uci();
void isready();
void setoption(char *line);
void ucinewgame();
void setPosition(char *line);
void go(char *line);
void perft(char *line);
void divide(char *line);
void setvalue(char *line);
void evaluatePosition();
void quit();
void stop();
void thinkAsync(SearchStopCriteria ssc);

bool fexists(const std::string& filename) {
	std::ifstream f(filename.c_str());
	if (f.good()) {
		f.close();
		return true;
	}
	else {
		f.close();
		return false;
	}
}

char *my_strtok(char **str, char *delim)
{
	assert(str && *str && delim);
	char *token = *str, *p = token;

	// special case: if end ofstd::string reached return NULL
	if (!*p)
		return NULL;

	// parse token until first delimiter
	while (*p && !strchr(delim, *p))
		p++;

	// replace subsequent delimiter(s) by 0
	while (*p && strchr(delim, *p))
		*p++ = '\0';

	// set new parser position
	*str = p;

	return token;
}

void loop() {
	setvbuf(stdout, NULL, _IOLBF, 2048);
	char *line = NULL;
	size_t allocated = 0;

	while (!feof(stdin)) {
		std::string line;
		std::getline(std::cin, line);
		dispatch(&line[0]);
	}
}


void dispatch(char *line)
{
	const char *token = my_strtok(&line, " ");

	if (!token)
		return;
	else if (!strcmp(token, "uci"))
		uci();
	else if (!strcmp(token, "isready"))
		isready();
	else if (!strcmp(token, "setoption"))
		setoption(line);
	else if (!strcmp(token, "ucinewgame"))
		ucinewgame();
	else if (!strcmp(token, "position"))
		setPosition(line);
	else if (!strcmp(token, "go"))
		go(line);
	else if (!strcmp(token, "print"))
		std::cout << pos->print() << std::endl;
	else if (!strcmp(token, "perft"))
		perft(line);
	else if (!strcmp(token, "divide"))
		divide(line);
	else if (!strcmp(token, "setvalue"))
		setvalue(line);
	else if (!strcmp(token, "bench"))
		bench2(7);
	//else if (!strcmp(token, "eval"))
	//	cout << printEvaluation(pos);
	//else if (!strcmp(token, "qeval"))
	//	cout << "QEval: " << Engine.QEval(pos) << std::endl;
	else if (!strcmp(token, "quit"))
		quit();
	else if (!strcmp(token, "stop"))
		stop();
}


void uci() {
	Engine.UciOutput = true;
	puts("id name Nelson");
	puts("id author Christian Günther");
	printf("option name UCI_Chess960 type check default %s\n", Chess960 ? "true" : "false");
	printf("option name Hash type spin default %lu min 1 max 16384\n", HashSizeMB);
	//printf("option name Draw Value type spin default %d min -100 max 100\n", DrawValue);
	//printf("option name GaviotaTablebasePaths typestd::string\n");
	//printf("option name GaviotaTablebaseCache type spin default %lu min 1 max 16384\n", GTB_CACHE);
	//printf("option name GaviotaTablebaseCompressionScheme type spin default %lu min 0 max 4\n", GTB_COMPRESSION_SCHEME);
	//printf("option name BookFile typestd::string default %s\n", BOOK_FILE);
	//printf("option name UseBook type check default %s\n", USE_BOOK ? "true" : "false");
	puts("uciok");
}

void isready() {
	//tt_alloc(HashSizeMB << 20);
	puts("readyok");
}
void setoption(char *line){
	const char *token = my_strtok(&line, " ");
	if (strcmp(token, "name")) return;

	char name[64] = "";
	while ((token = my_strtok(&line, " ")) && strcmp(token, "value")) strcat(name, token);

	if (!token)	 return;

	token = my_strtok(&line, " ");
	if (!strcmp(name, "UCI_Chess960")) Chess960 = !strcmp(token, "true");
	if (!strcmp(name, "Hash")) {
		int hashSize = atoi(token);
		if (hashSize != HashSizeMB) {
			HashSizeMB = hashSize;
			tt::InitializeTranspositionTable(HashSizeMB);
		}
	}
	//if (!strcmp(name, "GaviotaTablebasePaths")) {
	//	if (token) GTB_PATH = token; else GTB_PATH = "";
	//}
	//if (!strcmp(name, "GaviotaTablebaseCache")) GTB_CACHE = atoi(token);
	//if (!strcmp(name, "GaviotaTablebaseCache")) GTB_COMPRESSION_SCHEME = TB_compression_scheme(atoi(token));
	//if (!strcmp(name, "BookFile")) {
	//	if (token) BOOK_FILE = token; else USE_BOOK = false;
	//}
	//if (!strcmp(name, "UseBook")) USE_BOOK = !strcmp(token, "true");
	//else if (!strcmp(name, "DrawValue"))
	//	DrawValue = atoi(token);
}
bool initialized = false;
void ucinewgame(){
	initialized = true;	
	Engine.NewGame();
	//tt::InitializeTranspositionTable(HashSizeMB);
	//if (USE_BOOK)  Engine.bookFile = BOOK_FILE; else Engine.bookFile = "";
	//tablebase::initialize();
	//if (tablebase::AvailableTableBaseLevel > 0)std::cout << "infostd::string Using Tablebases up to " << tablebase::AvailableTableBaseLevel << " pieces" << std::endl;
}

#define MAX_FEN 0x80

void setPosition(char *line){
	//Chessbase GUI doesn't send "ucinewgame" command => assure that Hashtable get's initialized in any case
	if (!initialized) ucinewgame();
	if (pos) {
		pos->deleteParents();
		delete(pos);
		pos = nullptr;
	}
	int idx = 0;
	position * startpos;

	char *token = my_strtok(&line, " ");
	if (!token)
		return;
	else if (!strcmp(token, "startpos")) {
		startpos = new position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		token = my_strtok(&line, " ");
	}
	else if (!strcmp(token, "fen")) {
		char fen[MAX_FEN] = "";
		while ((token = my_strtok(&line, " ")) && strcmp(token, "moves"))
			strcat(strcat(fen, token), " ");
		startpos = new position(fen);
	}
	else if (!strcmp(token, "p")) {
		token = my_strtok(&line, " ");
		if (!strcmp(token, "fine70")) startpos = new position("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -");
	}
	bool moves = false;
	position * pp = startpos;
	if (token && !strcmp(token, "moves")) {
		while ((token = my_strtok(&line, " "))) {
			std::string tokenString(token);
			position * next = new position(*pp);
			next->ApplyMove(parseMoveInUCINotation(tokenString, *next));
			next->AppliedMovesBeforeRoot++;
			pp = next;
		}
	}
	pos = pp;
	pos->SetPrevious(pp->Previous());
}

void deleteThread() {
	if (Mainthread != nullptr) {
		if (Mainthread->joinable()) {
			Engine.Stop = true;
			Mainthread->join();
			Engine.Reset();
		}
		free(Mainthread);
		Mainthread = nullptr;
	}
}

void thinkAsync(SearchStopCriteria ssc) {
	ValuatedMove BestMove = Engine.Think(*pos, ssc);
	std::cout << "bestmove " << toString(BestMove.move) << std::endl;
	//if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
	//	cout << "info depth " << Engine.Depth() << " nodes " << Engine.NodeCount << " score cp " << BestMove.score << " nps " << Engine.NodeCount * 1000 / Engine.ThinkTime()
	//	//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
	//	<< " pv " << Engine.PrincipalVariation(Engine.Depth()) << std::endl;
	//else {
	//	int pliesToMate;
	//	if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
	//	cout << "info depth " << Engine.Depth() << " nodes " << Engine.NodeCount << " score mate " << pliesToMate / 2 << " nps " << Engine.NodeCount * 1000 / Engine.ThinkTime()
	//		//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits()
	//		<< " pv " << Engine.PrincipalVariation(Engine.Depth()) << std::endl;
	//}
	Engine.Reset();
}

void go(char *line){
	if (!pos) pos = new position();
	SearchStopCriteria ssc;
	ssc.StartTime = now();
	char *token;
	int moveTime = 0;
	int increment = 0;
	long long nodes = 0;
	int movestogo = 30;
	while ((token = my_strtok(&line, " "))) {
		if (!strcmp(token, pos->GetSideToMove() == WHITE ? "wtime" : "btime"))
			moveTime = atoi(my_strtok(&line, " "));
		else if (!strcmp(token, pos->GetSideToMove() == WHITE ? "winc" : "binc"))
			increment = atoi(my_strtok(&line, " "));
		else if (!strcmp(token, "depth"))
			ssc.MaxDepth = atoi(my_strtok(&line, " "));
		else if (!strcmp(token, "nodes"))
			ssc.MaxNumberOfNodes = atoll(my_strtok(&line, " "));
		else if (!strcmp(token, "movestogo"))
			movestogo = atoi(my_strtok(&line, " "));
	}
	//Simple timemanagement
	if (moveTime > 0) {
		//if (movestogo > 30) movestogo = 30;
		int avalaibleTime = moveTime + movestogo * increment;
		ssc.SoftStopTime = ssc.StartTime + avalaibleTime / movestogo;
		ssc.HardStopTime = ssc.StartTime + moveTime - EmergencyTime;
		if ((ssc.HardStopTime - ssc.StartTime) < 2 * (ssc.SoftStopTime - ssc.StartTime)) {
			ssc.SoftStopTime = ssc.StartTime + (ssc.HardStopTime - ssc.StartTime) / 2;
		}
	}
	deleteThread();
	Mainthread = new std::thread(thinkAsync, ssc);

	//Engine.Think(pos, ssc);
	//cout << "bestmove " << toString(Engine.bestMove) << std::endl;
	//Engine.Reset();

	//const move_t m = id_loop(&board, &lim);

	//char m_str[6];
	//move_to_uci(&board, m, m_str);
	//printf("bestmove %s\n", m_str);
}

void setvalue(char *line) {
	//char * token = my_strtok(&line, " ");
	//if (!strcmp(token, "mf")) {
	//	token = my_strtok(&line, " ");
	//	MOBILITY_FACTOR = atoi(token);
	//	cout << "infostd::string Mobility Factor: " << MOBILITY_FACTOR << std::endl;
	//}
	//else if (!strcmp(token, "ppf")) {
	//	token = my_strtok(&line, " ");
	//	PASSED_PAWN_FACTOR = float(atof(token));
	//	pawn::updateBonusFactors();
	//	cout << "infostd::string Passed Pawn Factor: " << PASSED_PAWN_FACTOR << std::endl;
	//}
}

void stop() {
	deleteThread();
}

void perft(char *line) {
	char * token = my_strtok(&line, " ");
	if (!token) {
		std::cout << "No depth specified!" << std::endl;
		return;
	}
	int depth = atoi(token);
	if (depth == 0) {
		std::cout << token << " is no valid depth!" << std::endl;
		return;
	}
	int64_t start = now();
	uint64_t result = perft(*pos, depth);
	int64_t runtime = now() - start;
	std::cout << "Result: " << result << "\t" << runtime << " ms\t" << pos->fen() << std::endl;
}

void divide(char *line) {
	char * token = my_strtok(&line, " ");
	if (!token) {
		std::cout << "No depth specified!" << std::endl;
		return;
	}
	int depth = atoi(token);
	if (depth == 0) {
		std::cout << token << " is no valid depth!" << std::endl;
		return;
	}
	int64_t start = now();
	divide(*pos, depth);
	int64_t runtime = now() - start;
	std::cout << "Runtime: " << runtime << " ms\t" << std::endl;
}

void quit(){
	deleteThread();
	exit(EXIT_SUCCESS);
}
