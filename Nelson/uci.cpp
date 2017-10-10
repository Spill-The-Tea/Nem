#include <thread>
#include <fstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <queue>
#include "uci.h"
#include "utils.h"
#include "position.h"
#include "board.h"
#include "search.h"
#include "test.h"
#include "settings.h"
#include "timemanager.h"
#include "tablebase.h"
#ifdef WPF
#include "evaluation.h"
#endif

void UCIInterface::copySettings(Search * source, Search * destination) {
	destination->UciOutput = source->UciOutput;
	if (source->BookFile) destination->BookFile = new std::string(*source->BookFile);
	destination->PonderMode.store(source->PonderMode.load());
	destination->PrintCurrmove = source->PrintCurrmove;
}

void UCIInterface::loop() {
	uci();
	std::string line;
#ifdef CRASH
	std::queue<std::string> cmds;
#endif
	while (std::getline(std::cin, line)) {
#ifdef CRASH
		if (cmds.size() > 100) cmds.pop();
		cmds.emplace(line);
#endif
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
			test::benchmark(12);
		}
		else {
			std::string filename = line.substr(6, std::string::npos);
			test::benchmark(filename, 12);
		}
	}
	else if (!command.compare("eval")) {
		std::cout << _position->printEvaluation() << std::endl;
	}
	else if (!command.compare("qscore")) {
		qscore(tokens);
	}
#ifdef TUNE
	else if (!command.compare("tt1")) {
		ttlabeled();
	}
	else if (!command.compare("tt2")) {
		ttWDL();
	}
#endif
	else if (!command.compare("see")) {
		see(tokens);
	}
	else if (!command.compare("dumpTT")) {
		dumpTT(tokens);
	}
	else if (!command.compare("tb")) {
		tb();
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
	sync_cout << "id name Nemorino" << sync_endl;
	sync_cout << "id author Christian Guenther" << sync_endl;
	//read ini file (if exists)
	std::ifstream inifile("nemorino.ini");
	if (inifile) {
		for (std::string line; std::getline(inifile, line); )
		{
			if (line.size() == 0) continue;
			if (line[0] == ';') continue;
			std::size_t found = line.find("=");
			if (found != std::string::npos) {
				std::string key = line.substr(0, found);
				std::string value = line.substr(found + 1);
				if (settings::options.find(key) != settings::options.end()) {
					std::vector<std::string> tokens{ "setoption", "name", key, "value", value };
					setoption(tokens);
				}
			}
		}
	}
	settings::options.printUCI();
	//settings::parameter.UCIExpose();
	sync_cout << "uciok" << sync_endl;
}

void UCIInterface::isready() {
	updateFromOptions();
	sync_cout << "readyok" << sync_endl;
}

void UCIInterface::updateFromOptions() {
	ponderActive = settings::options.getBool(settings::OPTION_PONDER);
	settings::parameter.EmergencyTime = settings::options.getInt(settings::OPTION_EMERGENCY_TIME);
#ifdef TUNE
	settings::parameter.PieceValues[QUEEN] = Eval(settings::options.getInt(settings::OPTION_PIECE_VALUES_QUEEN_MG), settings::options.getInt(settings::OPTION_PIECE_VALUES_QUEEN_EG));
	settings::parameter.PieceValues[ROOK] = Eval(settings::options.getInt(settings::OPTION_PIECE_VALUES_ROOK_MG), settings::options.getInt(settings::OPTION_PIECE_VALUES_ROOK_EG));
	settings::parameter.PieceValues[BISHOP] = Eval(settings::options.getInt(settings::OPTION_PIECE_VALUES_BISHOP_MG), settings::options.getInt(settings::OPTION_PIECE_VALUES_BISHOP_EG));
	settings::parameter.PieceValues[KNIGHT] = Eval(settings::options.getInt(settings::OPTION_PIECE_VALUES_KNIGHT_MG), settings::options.getInt(settings::OPTION_PIECE_VALUES_KNIGHT_EG));
	settings::parameter.PieceValues[PAWN] = Eval(settings::options.getInt(settings::OPTION_PIECE_VALUES_PAWN_MG), settings::options.getInt(settings::OPTION_PIECE_VALUES_PAWN_EG));
#endif
	settings::parameter.TBProbeDepth = settings::options.getInt(settings::OPTION_SYZYGY_PROBE_DEPTH);
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
	}
	else if (!tokens[2].compare(settings::OPTION_MULTIPV)) {
		Engine->MultiPv = settings::options.getInt(settings::OPTION_MULTIPV);
	}
	else if (!tokens[2].compare(settings::OPTION_OPPONENT) && tokens.size() >= 6) {
		try {
			int rating = stoi(tokens[5]);
			int ownRating = 2800;
			settings::options.set(settings::OPTION_CONTEMPT, (ownRating - rating) / 10);
			sync_cout << "info string Contempt set to " << (int)settings::parameter.Contempt << sync_endl;
		}
		catch (...) {

		}
	}
	else if (!tokens[2].compare("Clear") && !tokens[3].compare(settings::OPTION_HASH)) {
		tt::clear();
		pawn::clear();
		Engine->Reset();
	}
	else if (!tokens[2].compare("Print") && !tokens[3].compare("Options")) {
		settings::options.printInfo();
	}
	else if (!tokens[2].compare(settings::OPTION_EMERGENCY_TIME)) {
		settings::parameter.EmergencyTime = settings::options.getInt(settings::OPTION_EMERGENCY_TIME);
	}
	else if (!tokens[2].compare(settings::OPTION_SYZYGY_PATH)) {
		if (settings::options.getString(settings::OPTION_SYZYGY_PATH).size() > 1) {
			tablebases::init(settings::options.getString(settings::OPTION_SYZYGY_PATH));
			if (tablebases::MaxCardinality < 3) {
				sync_cout << "Couldn't find any Tablebase files!!" << sync_endl;
				exit(1);
			}
			InitializeMaterialTable();
		}
	}
	else if (!tokens[2].compare(settings::OPTION_SYZYGY_PROBE_DEPTH)) {
		settings::parameter.TBProbeDepth = settings::options.getInt(settings::OPTION_SYZYGY_PROBE_DEPTH);
	}
	else {
		settings::parameter.SetFromUCI(tokens[2], tokens[4]);
	}
}

void UCIInterface::ucinewgame() {
	initialized = true;
	Engine->StopThinking();
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
	Position * startpos = nullptr;
	if (!tokens[1].compare("startpos")) {
		startpos = new Position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
		startpos = new Position(fen);
	}
	if (startpos) {
		Position * pp = startpos;
		if (tokens.size() > idx && !tokens[idx].compare("moves")) {
			++idx;
			while (idx < tokens.size()) {
				Position * next = new Position(*pp);
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
	const double result = stod(tokens[1]);
	std::vector<Position *> positions;
	positions.push_back(_position);
	Position * pos = _position;
	while ((pos = pos->Previous()) != nullptr) positions.push_back(pos);
	double totalError = 0;
	const int totalPlies = (int)positions.size();
	int plies = totalPlies+1;
	int count = 0;
	for (auto it = positions.rbegin(); it != positions.rend(); ++it)
	{
		--plies;
		pos = *it;
		if (pos->Checked()) continue;
		Value score = Engine->qscore(pos);
		if (std::abs(int(score - pos->evaluate())) > 100) continue;
		if (pos->GetSideToMove() == BLACK) score = -score;
		const double error = result - utils::sigmoid(score);
		totalError += error * error;
		++count;
	}
	if (count > 0) totalError = totalError / count; else totalError = 0;
	sync_cout << "info string error " << totalError << " count " << count << sync_endl;
}

#ifdef TUNE
double UCIInterface::ttlabeled()
{
	std::string filename = settings::options.getString(settings::OPTION_TEXEL_TUNING_LABELLED);
	if (filename.length() <= 0) {
		sync_cout << "info string no input data file specified: setoption name TTLabeled value <filename>" << sync_endl;
		return 0;
	}
	tt::clear();
	pawn::clear();
	Engine->Reset();
	double totalError = 0;
	int count = 0;
	sync_cout << "info string starting to analyze positions from " << filename << sync_endl;
	std::cout << "info string analysing ";
	std::ifstream infile(filename);
	for (std::string line; getline(infile, line); )
	{
		std::size_t found = line.find("c9");
		if (found != std::string::npos) {
			Position pos(line.substr(0, found));
			std::string rs = line.substr(found + 3);
			double result = 1;
			if (!rs.compare("\"1/2-1/2\";")) result = 0.5;
			else if (!rs.compare("\"0-1\";")) result = 0;
			Value score = (dynamic_cast<Search<SINGLE>*>(Engine))->qscore(&pos);
			if (pos.GetSideToMove() == BLACK) score = -score;
			double error = result - utils::sigmoid(score);
			totalError += error * error;
			++count;
			if ((count & 0x3FFF) == 0) std::cout << ".";
		}
	}
	std::cout << std::endl;
	if (count > 0) totalError = totalError / count; else totalError = 0;
	sync_cout << "info string error " << totalError << " count " << count << sync_endl;
	return totalError;
}

double UCIInterface::ttWDL() {
	Engine->UciOutput = false;
	const std::string params[3] = { settings::OPTION_TEXEL_TUNING_LOSSES, settings::OPTION_TEXEL_TUNING_DRAWS, settings::OPTION_TEXEL_TUNING_WINS };
	for (int i = 0; i < 3; ++i) {
		std::string filename = settings::options.getString(params[i]);
		if (filename.length() <= 0) {
			sync_cout << "info string no input data file specified: setoption name " << params[i] << " value <filename>" << sync_endl;
			return 0;
		}
	}
	tt::clear();
	pawn::clear();
	Engine->Reset();
	double totalError = 0;
	int count = 0;
	for (int i = 0; i < 3; ++i) {
		double result = 0.5 * i;
		std::string filename = settings::options.getString(params[i]);
		sync_cout << "info string starting to analyze positions from " << filename << sync_endl;
		std::cout << "info string analysing ";
		std::ifstream infile(filename);
		for (std::string line; getline(infile, line); )
		{
			Position pos(line);
			Value score = (dynamic_cast<Search<SINGLE>*>(Engine))->qscore(&pos);
			if (pos.GetSideToMove() == BLACK) score = -score;
			double error = result - utils::sigmoid(score);
			totalError += error * error;
			++count;
			if ((count & 0x3FFF) == 0) std::cout << ".";

		}
		std::cout << std::endl;
	}
	if (count > 0) totalError = totalError / count; else totalError = 0;
	sync_cout << "info string error " << totalError << " count " << count << sync_endl;
	return totalError;
}
#endif
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
	BestMove = Engine->Think(*_position);
	if (!ponderActive) sync_cout << "bestmove " << toString(BestMove.move) << sync_endl;
	else {
		//First try move from principal variation
		Move ponderMove = Engine->PonderMove();
		//Validate ponder move
		Position next(*_position);
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
	if (!_position) _position = new Position();
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
	Engine->timeManager.initialize(mode, moveTime, depth, nodes, moveTime, increment, movestogo, tnow, ponder, settings::options.getInt(settings::OPTION_NODES_TIME));

	deleteThread();
	Engine->PonderMode.store(ponder);
	Mainthread = new std::thread(&UCIInterface::thinkAsync, this);
}

void UCIInterface::ponderhit() {
	Engine->PonderMode.store(false);
	Engine->timeManager.PonderHit();
}

#pragma warning(disable:4100)
void UCIInterface::setvalue(std::vector<std::string> &tokens) {
	//Only for CLOP to be implemented per experiment
}
#pragma warning(default:4100)

void UCIInterface::stop() {
	utils::debugInfo("Trying to stop...");
	deleteThread();
}

void UCIInterface::perft(std::vector<std::string> &tokens) {
	if (tokens.size() < 2) {
		std::cout << "No depth specified!" << std::endl;
		return;
	}
	const int depth = stoi(tokens[1]);
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
	const int depth = stoi(tokens[1]);
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
	std::cout << "SEE: " << (int)_position->SEE(move) << std::endl;
}

void UCIInterface::quit() {
	deleteThread();
#ifdef WPF
	writeWPF();
#endif
	//utils::logger::instance()->flush();
	exit(EXIT_SUCCESS);
}

void UCIInterface::dumpTT(std::vector<std::string>& tokens)
{
	std::string filename;
	if (tokens.size() < 2) {
		filename = "dumpTT.dat";
	}
	else filename = tokens[1];
	std::ofstream of;
	of.open(filename, std::ios::binary | std::ios::out);
	std::string fen = _position->fen();
	size_t len = fen.length();
	char * fena = new char[96];
	for (size_t i = len; i < 96; ++i) fena[i] = 0x00;
	fena[95] = (char)popcount(tt::GetClusterCount() - 1);
	strcpy(fena, fen.c_str());
	of.write(fena, 96);
	tt::dumpTT(of);
	of.close();
}

void UCIInterface::tb() {
	if (_position == nullptr) {
		std::cout << "No position provided!" << std::endl;
		return;
	}
	if (!_position->GetMaterialTableEntry()->IsTablebaseEntry()) {
		std::cout << "Position " << _position->fen() << " not found in tablebase!" << std::endl;
		return;
	}
	tablebases::probe(*_position);
}
