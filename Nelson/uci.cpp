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

#ifdef TB
#include "syzygy/tbprobe.h"
#endif

baseSearch * Engine = new search < SINGLE > ;
position * _position = nullptr;
std::thread * Mainthread = nullptr;
int64_t ponderStartTime = 0;
bool ponderActive = false;

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
//bool validatePonderMove(Move bestmove, Move pondermove);

static void copySettings(baseSearch * source, baseSearch * destination) {
	destination->UciOutput = source->UciOutput;
	destination->BookFile = source->BookFile;
	destination->PonderMode.store(source->PonderMode.load());
	destination->PrintCurrmove = source->PrintCurrmove;
}

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
	uci();
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
	else if (!command.compare("eval")) {
		std::cout << _position->printEvaluation() << std::endl;
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
	sync_cout << "id name Nemorino" << sync_endl;
	sync_cout << "id author Christian Günther" << sync_endl;
	sync_cout << "option name UCI_Chess960 type check default " << (Chess960 ? "true" : "false") << sync_endl;
	sync_cout << "option name Hash type spin default " << HashSizeMB << " min 1 max 16384" << sync_endl;
	sync_cout << "option name Clear Hash type button" << sync_endl;
	sync_cout << "option name MultiPV type spin default " << Engine->MultiPv << " min 1 max 216" << sync_endl;
	sync_cout << "option name Threads type spin default " << HelperThreads + 1 << " min 1 max 128" << sync_endl;
	sync_cout << "option name Ponder type check" << sync_endl;
	sync_cout << "option name Contempt type spin default 0 min -1000 max 1000" << sync_endl;
	sync_cout << "option name BookFile type string default " << BOOK_FILE.c_str() << sync_endl;
	sync_cout << "option name OwnBook type check default " << (USE_BOOK ? "true" : "false") << sync_endl;
	sync_cout << "option name UCI_Opponent type string" << sync_endl;
#ifdef TB
	sync_cout << "option name SyzygyPath type string default " << SYZYGY_PATH.c_str() << sync_endl;
	sync_cout << "option name SyzygyProbeDepth type spin default " << SYZYGY_PROBE_DEPTH << " min 0 max 10" << sync_endl;
#endif
	sync_cout << "uciok" << sync_endl;
}

void isready() {
	sync_cout << "readyok" << sync_endl;
}

void setoption(std::vector<std::string> &tokens) {
	if (tokens.size() < 4 || tokens[1].compare("name")) return;
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
			baseSearch * newEngine = new search < MASTER >;
			copySettings(Engine, newEngine);
			delete Engine;
			Engine = newEngine;
		}
		else if (!HelperThreads && Engine->GetType() == MASTER) {
			baseSearch * newEngine = new search < SINGLE >;
			copySettings(Engine, newEngine);
			delete Engine;
			Engine = newEngine;
		}
	}
	else if (!tokens[2].compare("MultiPV")) {
		Engine->MultiPv = stoi(tokens[4]);
	}
	else if (!tokens[2].compare("Contempt")) {
		Contempt = Value(stoi(tokens[4]));
	}
	else if (!tokens[2].compare("UCI_Opponent")) {
		try {
			int rating = stoi(tokens[5]);
			int ownRating = 2700;
			Contempt = Value((ownRating - rating) / 10);
			sync_cout << "info string Contempt set to " << Contempt << sync_endl;
		}
		catch (...){

		}
	}
	else if (!tokens[2].compare("Clear") && !tokens[3].compare("Hash")) {
		tt::clear();
		pawn::clear();
		Engine->Reset();
	}
#ifdef TB
	else if (!tokens[2].compare("SyzygyPath")) {
		std::stringstream ssTBPath;
		if (tokens.size() < 5) {
			SYZYGY_PATH = "";
		}
		else {
			unsigned int idx = 4;
			while (idx < tokens.size()) {
				ssTBPath << ' ' << tokens[idx];
				++idx;

			}
			SYZYGY_PATH = ssTBPath.str().substr(1);
			Tablebases::init(SYZYGY_PATH);
			if (Tablebases::MaxCardinality < 3) {
				sync_cout << "Couldn't find any Tablebase files!!" << sync_endl;
				exit(1);
			}
			InitializeMaterialTable();
		}
	}
	else if (!tokens[2].compare("SyzygyProbeDepth")) {
		SYZYGY_PROBE_DEPTH = Value(stoi(tokens[4]));
	}
#endif
}

bool initialized = false;
void ucinewgame(){
	initialized = true;
	Engine->NewGame();
	if (USE_BOOK) Engine->BookFile = BOOK_FILE; else Engine->BookFile = "";
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
	Engine->PonderMode.store(false);
	Engine->StopThinking();
	if (Mainthread != nullptr) {
		if (Mainthread->joinable()) Mainthread->join();
		else utils::debugInfo("Can't stop Engine Thread!");
		free(Mainthread);
		Mainthread = nullptr;
	}
	Engine->Reset();
}

//bool validatePonderMove(Move bestmove, Move pondermove) {
//	position next(*_position);
//	if (next.ApplyMove(bestmove)) {
//		position next2(next);
//		return next2.validateMove(pondermove);
//	}
//	return false;
//}

void thinkAsync() {
	ValuatedMove BestMove;
	if (Engine == NULL) return;
	if (dynamic_cast<search<MASTER>*>(Engine)) BestMove = (dynamic_cast<search<MASTER>*>(Engine))->Think(*_position);
	else BestMove = (dynamic_cast<search<SINGLE>*>(Engine))->Think(*_position);
	if (!ponderActive) sync_cout << "bestmove " << toString(BestMove.move) << sync_endl;
	else {
		//First try move from principal variation
		Move ponderMove = Engine->PonderMove();
		//Validate ponder move
		position next(*_position);
		next.ApplyMove(BestMove.move);
		ponderMove = next.validMove(ponderMove);
		if (ponderMove == MOVE_NONE) sync_cout << "bestmove " << toString(BestMove.move) << sync_endl;
		else {
			sync_cout << "bestmove " << toString(BestMove.move) << " ponder " << toString(ponderMove) << sync_endl;
		}
	}
	Engine->Reset();
}

void go(std::vector<std::string> &tokens) {
	int64_t tnow = now();
	if (!_position) _position = new position();
	int moveTime = 0;
	int increment = 0;
	int movestogo = 30;
	bool ponder = false;
	bool searchmoves = false;
	unsigned int idx = 1;
	int depth = MAX_DEPTH;
	int64_t nodes = INT64_MAX;
	TimeMode mode = UNDEF;
	std::string time = _position->GetSideToMove() == WHITE ? "wtime" : "btime";
	std::string inc = _position->GetSideToMove() == WHITE ? "winc" : "binc";
	while (idx < tokens.size()) {
		if (!tokens[idx].compare(time)) {
			++idx;
			moveTime = stoi(tokens[idx]);
			searchmoves = false;
		} 
		else if (!tokens[idx].compare(inc)) {
			++idx;
			increment = stoi(tokens[idx]);
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
			mode = FIXED_TIME_PER_MOVE;
		}
		else if (!tokens[idx].compare("ponder")) {
			ponderStartTime = tnow;
			ponder = true;
			searchmoves = false;
			//mode = INFINIT;
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
	//if (ponder) {
	//	moveTime = pmoveTime;
	//	increment = pincrement;
	//}
	if (mode == UNDEF && moveTime == 0 && increment == 0 && nodes < INT64_MAX) mode = NODES;
	Engine->timeManager.initialize(mode, moveTime, depth, nodes, moveTime, increment, movestogo, tnow, ponder);

	deleteThread();
	Engine->PonderMode.store(ponder);
	Mainthread = new std::thread(thinkAsync);
}

void ponderhit() {
	Engine->PonderMode.store(false);
	Engine->timeManager.PonderHit();
}

void setvalue(std::vector<std::string> &tokens) {
	//Only for CLOP to be implemented per experiment
}

void stop() {
	utils::debugInfo("Trying to stop...");
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
	//utils::logger::instance()->flush();
	exit(EXIT_SUCCESS);
}
