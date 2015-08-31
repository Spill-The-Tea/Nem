#include <thread>
#include <fstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include "uci.h"
#include "position.h"
#include "board.h"
#include "search.h"
#include "test.h"
#include "settings.h"
#include "timemanager.h"


baseSearch * Engine = new search < SINGLE > ;
position * _position = nullptr;
std::thread * Mainthread = nullptr;
int64_t ponderStartTime = 0;
bool ponderActive = false;

int ponderedMoves = 0;
int ponderHits = 0;

void dispatch(std::string line);

// UCI command handlers
void uci();
void isready();
void setoption(std::vector<std::string> &tokens);
void ucinewgame();
void setPosition(std::vector<std::string> &tokens);
void go(std::vector<std::string> &tokens);
void perft(std::vector<std::string> &tokens);
void divide(std::vector<std::string> &tokens);
void setvalue(std::vector<std::string> &tokens);
void quit();
void stop();
void thinkAsync();
void ponderhit();

std::vector<std::string> split(std::string str) {
	std::vector<std::string> tokens;
	std::stringstream ss(str); // Turn the string into a stream.
	std::string tok;

	while (std::getline(ss, tok, ' ')) {
		tokens.push_back(tok);
	}

	return tokens;
}


void loop() {
	//setvbuf(stdout, NULL, _IOLBF, 2048);
	std::string line;
	while (std::getline(std::cin, line)) {
		dispatch(line);
	}
}

void dispatch(std::string line) {
	std::vector<std::string> tokens = split(line);
	if (tokens.size() == 0) return;
	std::string command = tokens[0];
	if (!command.compare("stop"))
		stop();
	else if (!command.compare("go"))
		go(tokens);
	else if (!command.compare("uci"))
		uci();
	else if (!command.compare("isready"))
		isready();
	else if (!command.compare("setoption"))
		setoption(tokens);
	else if (!command.compare("ucinewgame"))
		ucinewgame();
	else if (!command.compare("position"))
		setPosition(tokens);
	else if (!command.compare("ponderhit"))
		ponderhit();
	else if (!command.compare("print"))
		std::cout << _position->print() << std::endl;
	else if (!command.compare("perft"))
		perft(tokens);
	else if (!command.compare("divide"))
		divide(tokens);
	else if (!command.compare("setvalue"))
		setvalue(tokens);
	else if (!command.compare("bench")) {
		if (tokens.size() == 1) {
			bench(11);
			bench2(11);
		}
		else {
			std::string filename = line.substr(6, std::string::npos);
			bench(filename, 11);
		}
	}
	else if (!command.compare("sbench")) {
		int64_t nc = bench(11);
		std::cout << "bench: " << nc << std::endl;
	}
	//else if (!strcmp(token, "eval"))
	//	cout << printEvaluation(pos);
	//else if (!strcmp(token, "qeval"))
	//	cout << "QEval: " << Engine.QEval(pos) << std::endl;
	else if (!command.compare("quit"))
		quit();
}

void uci() {
	Engine->UciOutput = true;
	puts("id name Nemorino");
	puts("id author Christian Günther");
	printf("option name UCI_Chess960 type check default %s\n", Chess960 ? "true" : "false");
	printf("option name Hash type spin default %i min 1 max 16384\n", HashSizeMB);
	printf("option name MultiPV type spin default %i min 1 max 216\n", Engine->MultiPv);
	printf("option name Threads type spin default %i min 1 max 128\n", HelperThreads + 1);
	printf("option name Ponder type check\n");
	//printf("option name Draw Value type spin default %d min -100 max 100\n", DrawValue);
	//printf("option name GaviotaTablebasePaths type string\n");
	//printf("option name GaviotaTablebaseCache type spin default %lu min 1 max 16384\n", GTB_CACHE);
	//printf("option name GaviotaTablebaseCompressionScheme type spin default %lu min 0 max 4\n", GTB_COMPRESSION_SCHEME);
	printf("option name BookFile type string default %s\n", BOOK_FILE.c_str());
	printf("option name OwnBook type check default %s\n", USE_BOOK ? "true" : "false");
	//printf("option name bpf type spin default %lu\n", BETA_PRUNING_FACTOR);
	puts("uciok");
}

void isready() {
	//tt_alloc(HashSizeMB << 20);
	std::cout << "readyok" << std::endl;
}

void setoption(std::vector<std::string> &tokens) {
	if (tokens.size() < 5 || tokens[1].compare("name") || tokens[3].compare("value")) return;
	if (!tokens[2].compare("UCI_Chess960")) Chess960 = !tokens[4].compare("true");
	else if (!tokens[2].compare("Hash")) {
		int hashSize = stoi(tokens[4]);
		if (hashSize != HashSizeMB) {
			HashSizeMB = hashSize;
			tt::InitializeTranspositionTable(HashSizeMB);
		}
	}
	else if (!tokens[2].compare("BookFile")) {
		std::stringstream ssBookfile;
		if (tokens.size() < 5) {
			USE_BOOK = false;
		}
		else {		
			unsigned int idx = 4;
			while (idx < tokens.size()) {
				ssBookfile << ' ' << tokens[idx];
				++idx;

			}
			BOOK_FILE = ssBookfile.str().substr(1);
		}
	}
	else if (!tokens[2].compare("OwnBook")) USE_BOOK = !tokens[4].compare("true");
	else if (!tokens[2].compare("Ponder")) ponderActive = !tokens[4].compare("true");
	else if (!tokens[2].compare("Threads")) {
		HelperThreads = stoi(tokens[4]) - 1;
		if (HelperThreads && Engine->GetType() == SINGLE) {
			delete Engine;
			Engine = new search < MASTER >;
		}
		else if (!HelperThreads && Engine->GetType() == MASTER) {
			delete Engine;
			Engine = new search < SINGLE >;
		}
	}
	else if (!tokens[2].compare("MultiPV")) {
		Engine->MultiPv = stoi(tokens[4]);
	}
}

bool initialized = false;
void ucinewgame(){
	initialized = true;
	Engine->NewGame();
	//tt::InitializeTranspositionTable(HashSizeMB);
	if (USE_BOOK) Engine->BookFile = BOOK_FILE; else Engine->BookFile = "";
	//tablebase::initialize();
	//if (tablebase::AvailableTableBaseLevel > 0)std::cout << "infostd::string Using Tablebases up to " << tablebase::AvailableTableBaseLevel << " pieces" << std::endl;
}

#define MAX_FEN 0x80

void setPosition(std::vector<std::string> &tokens) {
	if (!initialized) ucinewgame();
	if (_position) {
		_position->deleteParents();
		delete(_position);
		_position = nullptr;
	}
	unsigned int idx = 0;
	position * startpos = nullptr;
	if (!tokens[1].compare("startpos")) {
		startpos = new position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		idx = 2;
	}
	else if (!tokens[1].compare("fen")) {
		std::stringstream ssFen;
		idx = 2;
		while (idx < tokens.size() && tokens[idx].compare("moves")) {
			ssFen << ' ' << tokens[idx];
			++idx;
		}
		std::string fen = ssFen.str().substr(1);
		startpos = new position(fen);
	}
	if (startpos) {
		position * pp = startpos;
		if (tokens.size() > idx && !tokens[idx].compare("moves")) {
			++idx;
			while (idx < tokens.size()) {
				position * next = new position(*pp);
				next->ApplyMove(parseMoveInUCINotation(tokens[idx], *next));
				next->AppliedMovesBeforeRoot++;
				pp = next;
				++idx;
			}
		}
		_position = pp;
		_position->SetPrevious(pp->Previous());
	}
}

void deleteThread() {
	Engine->PonderMode = false;
	Engine->StopThinking();
	if (Mainthread != nullptr) {
		if (Mainthread->joinable())  Mainthread->join();
		else std::cout << "info string Can't stop Engine Thread!" << std::endl;
		free(Mainthread);
		Mainthread = nullptr;
	}
	Engine->Reset();
}

void thinkAsync() {
	ValuatedMove BestMove;
	if (Engine == NULL) return;
	if (dynamic_cast<search<MASTER>*>(Engine)) BestMove = (dynamic_cast<search<MASTER>*>(Engine))->Think(*_position);
	else BestMove = (dynamic_cast<search<SINGLE>*>(Engine))->Think(*_position);
	if (!ponderActive) std::cout << "bestmove " << toString(BestMove.move) << std::endl;
	else {
		//First try move from principal variation
		Move ponderMove = Engine->PonderMove();
		if (ponderMove == MOVE_NONE) {
			//Then try to find any move to ponder on (as Arena seems to have problems when no ponder move is provided)
			position next(*_position);
			next.ApplyMove(BestMove.move);
			ValuatedMove * moves = next.GenerateMoves<LEGAL>();
			int movecount = next.GeneratedMoveCount();
			ponderMove = Engine->GetBestBookMove(next, moves, movecount);
			if (ponderMove == MOVE_NONE && movecount > 0) ponderMove = moves->move;
		}
		if (ponderMove == MOVE_NONE || !ponderActive) std::cout << "bestmove " << toString(BestMove.move) << std::endl;
		else std::cout << "bestmove " << toString(BestMove.move) << " ponder " << toString(ponderMove) << std::endl;
	}
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
	Engine->Reset();
}

void go(std::vector<std::string> &tokens) {
	int64_t tnow = now();
	if (!_position) _position = new position();
	int moveTime = 0;
	int pmoveTime = 0;
	int increment = 0;
	int pincrement = 0;
	int movestogo = 30;
	bool ponder = false;
	bool searchmoves = false;
	unsigned int idx = 1;
	bool fixedTime = false;
	int depth = MAX_DEPTH;
	int64_t nodes = INT64_MAX;
	TimeMode mode = UNDEF;
	std::string time = _position->GetSideToMove() == WHITE ? "wtime" : "btime";
	std::string inc = _position->GetSideToMove() == WHITE ? "winc" : "binc";
	std::string ptime = _position->GetSideToMove() == BLACK ? "wtime" : "btime";
	std::string pinc = _position->GetSideToMove() == BLACK ? "winc" : "binc";
	while (idx < tokens.size()) {
		if (!tokens[idx].compare(time)) {
			++idx;
			moveTime = stoi(tokens[idx]);
			searchmoves = false;
		} else if (!tokens[idx].compare(ptime)) {
			++idx;
			pmoveTime = stoi(tokens[idx]);
			searchmoves = false;
		}
		else if (!tokens[idx].compare(inc)) {
			++idx;
			increment = stoi(tokens[idx]);
			searchmoves = false;
		}
		else if (!tokens[idx].compare(pinc)) {
			++idx;
			pincrement = stoi(tokens[idx]);
			searchmoves = false;
		}
		else if (!tokens[idx].compare("depth")) {
			++idx;
			depth = stoi(tokens[idx]);
			searchmoves = false;
		}
		else if (!tokens[idx].compare("nodes")) {
			++idx;
			nodes = stoll(tokens[idx]);
			searchmoves = false;
		}
		else if (!tokens[idx].compare("movestogo")) {
			++idx;
			movestogo = stoi(tokens[idx]);
			searchmoves = false;
		}
		else if (!tokens[idx].compare("movetime")) {
			++idx;
			moveTime = stoi(tokens[idx]);
			movestogo = 1;
			searchmoves = false;
			fixedTime = true;
			mode = FIXED_TIME_PER_MOVE;
		}
		else if (!tokens[idx].compare("ponder")) {
			ponderStartTime = tnow;
			ponder = true;
			ponderedMoves++;
			searchmoves = false;
			mode = INFINIT;
		}
		else if (!tokens[idx].compare("infinite")) {
			moveTime = INT_MAX;
			movestogo = 1;
			searchmoves = false;
			mode = INFINIT;
		}
		else if (!tokens[idx].compare("searchmoves")) {
			searchmoves = true;
		}
		else if (searchmoves) {
			Move m = parseMoveInUCINotation(tokens[idx], *_position);
			Engine->searchMoves.push_back(m);
		}
		++idx;
	}
	if (ponder) {
		moveTime = pmoveTime;
		increment = pincrement;
	}
	if (mode == UNDEF && moveTime == 0 && increment == 0 && nodes < INT64_MAX) mode = NODES;
	Engine->timeManager.initialize(mode, moveTime, depth, nodes, moveTime, increment, movestogo, tnow);

	deleteThread();
	Engine->PonderMode = ponder;
	Mainthread = new std::thread(thinkAsync);
}

void ponderhit() {
	Engine->timeManager.PonderHit();
	Engine->PonderMode = false;
	ponderHits++;
}

void setvalue(std::vector<std::string> &tokens) {
	//Only for CLOP to be implemented per experiment
}

void stop() {
	deleteThread();
}

void perft(std::vector<std::string> &tokens) {
	if (tokens.size() < 2) {
		std::cout << "No depth specified!" << std::endl;
		return;
	}
	int depth = stoi(tokens[1]);
	if (depth == 0) {
		std::cout << tokens[1] << " is no valid depth!" << std::endl;
		return;
	}
	int64_t start = now();
	uint64_t result = perft(*_position, depth);
	int64_t runtime = now() - start;
	std::cout << "Result: " << result << "\t" << runtime << " ms\t" << _position->fen() << std::endl;
}

void divide(std::vector<std::string> &tokens) {
	if (tokens.size() < 2) {
		std::cout << "No depth specified!" << std::endl;
		return;
	}
	int depth = stoi(tokens[1]);
	if (depth == 0) {
		std::cout << tokens[1] << " is no valid depth!" << std::endl;
		return;
	}
	int64_t start = now();
	divide(*_position, depth);
	int64_t runtime = now() - start;
	std::cout << "Runtime: " << runtime << " ms\t" << std::endl;
}

void quit(){
	deleteThread();
	exit(EXIT_SUCCESS);
}
