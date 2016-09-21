#include <thread>
#include <fstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include "uci.h"
#include "utils.h"
#include "position.h"
#include "board.h"
#include "search.h"
#include "test.h"
#include "settings.h"
#include "timemanager.h"

#ifdef TB
#include "tablebase.h"
#endif

void UCIInterface::copySettings(baseSearch * source, baseSearch * destination) {
	destination->UciOutput = source->UciOutput;
	if (source->BookFile) destination->BookFile = new std::string(*source->BookFile);
	destination->PonderMode.store(source->PonderMode.load());
	destination->PrintCurrmove = source->PrintCurrmove;
}

void UCIInterface::loop() {
	uci();
	std::string line;
	while (std::getline(std::cin, line)) {
		dispatch(line);
	}
}

void UCIInterface::dispatch(std::string line) {
	std::vector<std::string> tokens = utils::split(line);
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
			test::benchmark(11);
		}
		else {
			std::string filename = line.substr(6, std::string::npos);
			test::benchmark(filename, 11);
		}
	}
	else if (!command.compare("eval")) {
		std::cout << _position->printEvaluation() << std::endl;
	}
	else if (!command.compare("qscore")) {
		qscore(tokens);
	}
	else if (!command.compare("see")) {
		see(tokens);
	}
	else if (!command.compare("testsee")) {
		test::testSEE();
	}
	//else if (!strcmp(token, "eval"))
	//	cout << printEvaluation(pos);
	//else if (!strcmp(token, "qeval"))
	//	cout << "QEval: " << Engine.QEval(pos) << std::endl;
	else if (!command.compare("quit"))
		quit();
#ifdef TRACE
	else if (!command.compare("dumpTree")) utils::dumpSearchTreeToFile();
#endif
}

void UCIInterface::uci() {
	Engine->UciOutput = true;
	settings::options.printUCI();
	sync_cout << "uciok" << sync_endl;
}

void UCIInterface::isready() {
	sync_cout << "readyok" << sync_endl;
}

void UCIInterface::setoption(std::vector<std::string> &tokens) {
	if (tokens.size() < 4 || tokens[1].compare("name")) return;
	settings::options.read(tokens);
    if (!tokens[2].compare(settings::OPTION_BOOK_FILE)) {
		if (tokens.size() < 5) {
			settings::options.set(settings::OPTION_OWN_BOOK, false);
		}
	}
	else if (!tokens[2].compare(settings::OPTION_PONDER)) 
		ponderActive = settings::options.getBool(settings::OPTION_PONDER);
	else if (!tokens[2].compare(settings::OPTION_THREADS)) {
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
	else if (!tokens[2].compare(settings::OPTION_MULTIPV)) {
		Engine->MultiPv = settings::options.getInt(settings::OPTION_MULTIPV);
	}
	else if (!tokens[2].compare(settings::OPTION_OPPONENT)) {
		try {
			int rating = stoi(tokens[5]);
			int ownRating = 2800;
			settings::options.set(settings::OPTION_CONTEMPT, (ownRating - rating) / 10);
			sync_cout << "info string Contempt set to " << Contempt << sync_endl;
		}
		catch (...) {

		}
	}
	else if (!tokens[2].compare("Clear") && !tokens[3].compare(settings::OPTION_HASH)) {
		tt::clear();
		pawn::clear();
		Engine->Reset();
	}
#ifdef TB
	else if (!tokens[2].compare(settings::OPTION_SYZYGY_PATH)) {
		if (settings::options.getString(settings::OPTION_SYZYGY_PATH).size() > 1) {
			Tablebases::init(settings::options.getString(settings::OPTION_SYZYGY_PATH));
			if (Tablebases::MaxCardinality < 3) {
				sync_cout << "Couldn't find any Tablebase files!!" << sync_endl;
				exit(1);
			}
			InitializeMaterialTable();
		}
	}
#endif
}

void UCIInterface::ucinewgame() {
	initialized = true;
	Engine->NewGame();
	if (settings::options.getBool(settings::OPTION_OWN_BOOK)) Engine->BookFile = new std::string(settings::options.getString(settings::OPTION_BOOK_FILE)); else Engine->BookFile = nullptr;
}

#define MAX_FEN 0x80

void UCIInterface::setPosition(std::vector<std::string> &tokens) {
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

void UCIInterface::deleteThread() {
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

void UCIInterface::qscore(std::vector<std::string>& tokens)
{
	double result = stod(tokens[1]);
	std::vector<position *> positions;
	positions.push_back(_position);
	position * pos = _position;
	while ((pos = pos->Previous()) != nullptr) positions.push_back(pos);
	double totalError = 0;
	int totalPlies = (int)positions.size();
	int plies = totalPlies+1;
	int count = 0;
	for (auto it = positions.rbegin(); it != positions.rend(); ++it)
	{
		--plies;
		position * pos = *it;
#ifdef TUNEPP
		if (pos->GetPawnEntry()->passedPawns == EMPTY) continue;
#endif
		if (pos->Checked()) continue;
		Value score = (dynamic_cast<search<SINGLE>*>(Engine))->qscore(pos);
		if (std::abs(int(score - pos->evaluate())) > 100) continue;
		if (pos->GetSideToMove() == BLACK) score = -score;
		double error = result - utils::sigmoid(score);
		totalError += error * error;
		++count;
	}
	if (count > 0) totalError = totalError / count; else totalError = 0;
	sync_cout << "info string error " << totalError << " count " << count << sync_endl;
}

//bool validatePonderMove(Move bestmove, Move pondermove) {
//	position next(*_position);
//	if (next.ApplyMove(bestmove)) {
//		position next2(next);
//		return next2.validateMove(pondermove);
//	}
//	return false;
//}

void UCIInterface::thinkAsync() {
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

void UCIInterface::go(std::vector<std::string> &tokens) {
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
	Mainthread = new std::thread(&UCIInterface::thinkAsync, this);
}

void UCIInterface::ponderhit() {
	Engine->PonderMode.store(false);
	Engine->timeManager.PonderHit();
}

void UCIInterface::setvalue(std::vector<std::string> &tokens) {
	//Only for CLOP to be implemented per experiment
}

void UCIInterface::stop() {
	utils::debugInfo("Trying to stop...");
	deleteThread();
}

void UCIInterface::perft(std::vector<std::string> &tokens) {
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
	uint64_t result = test::perft(*_position, depth);
	int64_t runtime = now() - start;
	std::cout << "Result: " << result << "\t" << runtime << " ms\t" << _position->fen() << std::endl;
}

void UCIInterface::divide(std::vector<std::string> &tokens) {
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
	test::divide(*_position, depth);
	int64_t runtime = now() - start;
	std::cout << "Runtime: " << runtime << " ms\t" << std::endl;
}

void UCIInterface::see(std::vector<std::string> &tokens) {
	if (_position == nullptr) {
		std::cout << "No position specified!" << std::endl;
		return;
	}
	if (tokens.size() < 2) {
		std::cout << "No move specified!" << std::endl;
		return;
	}
	Move move = parseMoveInUCINotation(tokens[1], *_position);
	std::cout << "SEE: " << _position->SEE(move) << std::endl;
}

void UCIInterface::quit() {
	deleteThread();
	//utils::logger::instance()->flush();
	exit(EXIT_SUCCESS);
}
