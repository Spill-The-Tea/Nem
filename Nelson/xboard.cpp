#include <iostream>
#include "xboard.h"
#include "utils.h"

namespace cecp {

	XBoard::XBoard()
	{
		BestMove.move = MOVE_NONE;
		BestMove.score = VALUE_ZERO;
		time[0] = 0;
		time[1] = 0;
		ponder.store(false);
		drawOffered.store(false);
		engineState.store(Waiting);
		sync_cout << "# Waiting (constructor)" << sync_endl;
		Enginethread = new std::thread(&XBoard::run, this);
	}


	XBoard::~XBoard()
	{
		deleteThread();
		delete Engine;
		Position * prev;
		do {
			prev = pos->Previous();
			delete pos;
			pos = prev;
		} while (prev != nullptr);
	}
	/* This is the engine thread and the crucial part of the xboard protocol driver
		Basically the engine can have 3 states: Waiting, Thinking and Pondering.
		When pondering is off the situation is easy, there is no pondering state and we will have only transitions between Waiting and Thinking
		Waiting -> Thinking: When the game starts, or when the opponent moved
		Thinking -> Waiting: When the search is done
		In ponder mode the situation is more complicated. Mainly we have transisitions between pondering and thinking:
		In the beginning of the game the engine starts thinking.
		Then after search is finished it starts to ponder.
		Some time later the opponent will move, we will either have a ponderhit, then we will switch the engine to thinking without stopping the search
		If we have a pondermiss the search is stopped and the engine will set to thinking on the new position
	*/
	void XBoard::run() {
		while (engineState.load() != Exiting) {
			//Check if engine shall sleep
			std::unique_lock<std::mutex> lckStart(mtxStart);
			while (engineState.load() == Waiting) cvStart.wait(lckStart);
			if (engineState.load() == Thinking) {
				//Start Thinking
				Engine->PonderMode.store(false);
				prepareTimeManager(false);
				Engine->Stop.store(false);
				BestMove = Engine->Think(*pos);
				//Search is done => check for draw offer
				if (drawOffered.load() && BestMove.score < -settings::parameter.Contempt) {
					//Accept it if we are worse than Contempt
					sync_cout << "offer draw" << sync_endl;
					drawOffered.store(false);
				}
				//Now send move to GUI...
				sync_cout << "move " << toXboardString(BestMove.move) << sync_endl;
				//...and apply it tour internal board
				Position * next = new Position(*pos);
				next->ApplyMove(BestMove.move);
				moves.push_back(BestMove.move);
				next->AppliedMovesBeforeRoot++;
				pos = next;
				//Process result and check if engine should ponder
				if (processResult(BestMove.score) && ponder.load() && engineState.load() == Thinking && (ponderMove = Engine->PonderMove())) {
					//Start pondering
					engineState.store(Pondering);
					sync_cout << "# Pondering (run1)" << sync_endl;
				}
				else {
					//No pondering => send engine to sleep
					engineState.store(Waiting);
					sync_cout << "# Waiting (run2)" << sync_endl;
				}
			}
			else if (engineState.load() == Pondering) {
				//start pondering: prepare ponder position
				if (ponderpos) {
					delete ponderpos;
					ponderpos = nullptr;
				}
				ponderpos = new Position(*pos);
				ponderMove = ponderpos->validMove(ponderMove);
				if (ponderMove == MOVE_NONE) {
					//No pondering => send engine to sleep
					engineState.store(Waiting);
					sync_cout << "# Waiting (run3)" << sync_endl;
					continue;
				}
				ponderpos->ApplyMove(ponderMove);
				ponderpos->AppliedMovesBeforeRoot++;
				Engine->PonderMode.store(true);
				prepareTimeManager(true);
				Engine->Stop.store(false);
				//start engine to search ponder position
				BestMove = Engine->Think(*ponderpos);
				//Search has finished. There are 2 possible states:
				//Engine is Thinking, this means we had a ponder hit and the engine state was switched to thinking
				if (engineState.load() == Thinking) {
					//We had a ponder hit => the search result is our next move
					//check for draw offer
					if (drawOffered.load() && BestMove.score < -settings::parameter.Contempt) {
						//Accept it if we are worse than Contempt
						sync_cout << "offer draw" << sync_endl;
						drawOffered.store(false);
					}
					//Now send move to GUI...
					sync_cout << "move " << toXboardString(BestMove.move) << sync_endl;
					//...and apply it tour internal board
					Position * next = new Position(*pos);
					next->ApplyMove(BestMove.move);
					moves.push_back(BestMove.move);
					next->AppliedMovesBeforeRoot++;
					pos = next;
					//Check if game is over
					if (!processResult(BestMove.score)) {
						//Game is over => send engine to sleep
						engineState.store(Waiting);
						sync_cout << "# Waiting (run3)" << sync_endl;
					}
					else if (ponder.load() && (ponderMove = Engine->PonderMove())) {
						//Send engine to ponder mode
						engineState.store(Pondering);
						sync_cout << "# Pondering (run4)" << sync_endl;
					}
					else {
						//ponder is switched off, or we have no ponder move => let the engine sleep
						engineState.store(Waiting);
						sync_cout << "# Waiting (run5)" << sync_endl;
					}
				}
				else {
					//we had a ponder miss => we have to start thinking again
					engineState.store(Thinking);
					sync_cout << "# Thinking (run6)" << sync_endl;
				}
			}
			Engine->Reset();
		}
	}

	bool XBoard::processResult(Value score) {
		DetailedResult dr = pos->GetDetailedResult();
		switch (dr)
		{
		case NO_RESULT:
			return true;
		case WHITE_MATES:
			sync_cout << "1-0 {White mates}" << sync_endl;
			return false;
		case BLACK_MATES:
			sync_cout << "0-1 {Black mates}" << sync_endl;
			return false;
		case DRAW_50_MOVES:
			if (score < -settings::parameter.Contempt) {
				sync_cout << "1/2-1/2 {50-move rule}" << sync_endl;
				return false;
			}
			else return true;
		case DRAW_STALEMATE:
			sync_cout << "1/2-1/2 {Stalemate}" << sync_endl;
			return false;
		case DRAW_REPETITION:
			if (score < -settings::parameter.Contempt) {
				sync_cout << "1/2-1/2 {3-fold Repetition}" << sync_endl;
				return false;
			}
			else return true;
		case DRAW_MATERIAL:
			sync_cout << "1/2-1/2 {No mating material}" << sync_endl;
			return false;
		default:
			return true;
		}
	}

	//Only used when engine has received quit command
	void XBoard::deleteThread() {
		engineState.store(Exiting);
		cvStart.notify_one();
		Engine->PonderMode.store(false);
		Engine->StopThinking();
		if (Enginethread != nullptr) {
			if (Enginethread->joinable())  Enginethread->join();
			else sync_cout << "info string Can't stop Engine Thread!" << sync_endl;
			free(Enginethread);
			Enginethread = nullptr;
		}
		Engine->Reset();
	}

	void printResult(Color stm, Value score)
	{
		if (score == 0) sync_cout << "1/2-1/2" << sync_endl;
		if ((score > VALUE_DRAW && stm == WHITE) || (score < VALUE_DRAW && stm == BLACK)) sync_cout << "1-0" << sync_endl;
		else sync_cout << "0-1" << sync_endl;
	}

	void XBoard::loop() {
		std::string line;
		Engine->XBoardOutput = true;
		while (std::getline(std::cin, line)) {
			if (!line.compare(0, 4, "ping")) {
				line[1] = 'o';
				sync_cout << line << sync_endl;
			}
			else if (!dispatch(line)) break;
		}
	}

	bool XBoard::dispatch(std::string line) {
		std::vector<std::string> tokens = utils::split(line);
		if (tokens.size() == 0) return true;
		std::string command = tokens[0];
		if (!command.compare("usermove")) {
			usermove(tokens);
		}
		else if (!command.compare("draw")) {
			drawOffered.store(true);
		}
		else if (!command.compare("time")) {
			time[EngineSide] = 10 * stoi(tokens[1]);
			if (engineState.load() == Pondering) Engine->timeManager.updateTime(time[EngineSide]);
		}
		else if (!command.compare("otim")) {
			time[EngineSide ^ 1] = 10 * stoi(tokens[1]);
		}
		else if (!command.compare("level")) {
			level(tokens);
		}
		else if (!command.compare("st")) {
			st(tokens);
		}
		else if (!command.compare("sd")) {
			sd(tokens);
		}
		else if (!command.compare("new")) {
			newGame();
		}
		else if (!command.compare("protover")) {
			protover(tokens);
		}
		else if (!command.compare("variant")) {
			variant(tokens);
		}
		else if (!command.compare("go")) {
			go();
		}
		else if (!command.compare("option")) {
			option(tokens);
		}
		else if (!command.compare("?")) {
			moveNow();
		}
		else if (!command.compare("setboard")) {
			setboard(line);
		}
		else if (!command.compare("undo")) {
			pos = pos->Previous();
			moves.pop_back();
			if (xstate == ANALYZE) {
				Engine->StopThinking();
				go();
			}
		}
		else if (!command.compare("remove")) {
			pos = pos->Previous();
			moves.pop_back();
			if (moves.size() > 0) {
				pos = pos->Previous();
				moves.pop_back();
			}
			if (xstate == ANALYZE) {
				Engine->StopThinking();
				go();
			}
		}
		else if (!command.compare("memory")) {
			memory(tokens);
		}
		else if (!command.compare("cores")) {
			cores(tokens);
		}
		else if (!command.compare("force") || !command.compare("exit")) {
			Engine->StopThinking();
			xstate = FORCE;
		}
		else if (!command.compare("hard")) {
			ponder = true;
		}
		else if (!command.compare("easy")) {
			ponder = false;
		}
		else if (!command.compare("post")) {
			Engine->XBoardOutput = true;
		}
		else if (!command.compare("nopost")) {
			Engine->XBoardOutput = false;
		}
		else if (!command.compare("include")) {
			includeMoves(tokens);
		}
		else if (!command.compare("playother")) {
			EngineSide = Color(EngineSide ^ 1);
		}
		else if (!command.compare("analyze")) {
			analyze();
		}
		else if (!command.compare("rating")) {
			rating(tokens);
		}
		else if (line[0] == '.') {
			sync_cout << Engine->GetXAnalysisOutput() << sync_endl;
		}
		else if (!command.compare("egtpath")) {
			egtpath(tokens);
		}
		else if (!command.compare("quit")) {
			deleteThread();
			//utils::logger::instance()->flush();
			exit(EXIT_SUCCESS);
		}
		return true;
	}

	void XBoard::go() {
		if (xstate == FORCE) xstate = STANDARD;
		EngineSide = pos->GetSideToMove();
		engineState.store(Thinking);
		sync_cout << "# Thinking (go)" << sync_endl;
		cvStart.notify_one();
	}

	void XBoard::analyze() {
		xstate = ANALYZE;
		if (engineState.load() == Thinking) {
			Engine->timeManager.switchToInfinite();
			return;
		}
		else if (engineState.load() == Pondering) Engine->StopThinking();
		go();
	}

	void XBoard::variant(std::vector<std::string> tokens) {
		Chess960 = !tokens[1].compare("fischerandom");
	}

	void XBoard::memory(std::vector<std::string> tokens) {
		settings::options.set(settings::OPTION_HASH, tokens[1]);
		tt::InitializeTranspositionTable();
	}

	void XBoard::cores(std::vector<std::string> tokens) {
		settings::parameter.HelperThreads = stoi(tokens[1]) - 1;
	}

	void XBoard::protover(std::vector<std::string> tokens) {
		protocolVersion = stoi(tokens[1]);
		sync_cout << "feature ping=1 setboard=1 usermove=1 time=1 draw=1 sigint=0 sigterm=0 reuse=0 myname=\"" << VERSION_INFO << "\""
			<< " variants=\"normal,fischerrandom\" colors=0 ics=1 name=1 include=1"
			<< " memory=1 smp=1 name=1"
			<< " egt = \"syzygy\""
			<< sync_endl;
		sync_cout << "feature option=\"Clear Hash -button\"" << sync_endl;
		sync_cout << "feature option=\"MultiPV -spin " << Engine->MultiPv << " 1 216\"" << sync_endl;
		sync_cout << "feature option=\"Contempt -spin 0 -1000 1000\"" << sync_endl;
		sync_cout << "feature option=\"BookFile -file " << settings::options.getString(settings::OPTION_BOOK_FILE).c_str() << "\"" << sync_endl;
		sync_cout << "feature option=\"OwnBook -check " << (settings::options.getBool(settings::OPTION_OWN_BOOK) ? "1\"" : "0\"") << sync_endl;
		sync_cout << "feature done=1" << sync_endl;
	}

	void XBoard::newGame() {
		Engine->StopThinking();
		engineState.store(Waiting);
		sync_cout << "# Waiting (newGame)" << sync_endl;
		clearPos();
		pos = new Position();
		EngineSide = BLACK;
	}

	void XBoard::moveNow() {
		Engine->StopThinking();
	}

	void XBoard::sd(std::vector<std::string> tokens) {
		tc_maxDepth = stoi(tokens[1]);
	}

	void XBoard::st(std::vector<std::string> tokens) {
		tc_movetime = 1000 * stoi(tokens[1]);
		tc_time = 0;
		tc_moves = 0;
		tc_increment = 0;
	}

	void XBoard::level(std::vector<std::string> tokens) {
		tc_movetime = 0;
		tc_moves = stoi(tokens[1]);
		try {
			tc_time = 60000 * stoi(tokens[2]);
		}
		catch (const std::invalid_argument&) {
			size_t indx = tokens[2].find(":");
			if (indx == std::string::npos) {
				size_t indx2 = tokens[2].find_first_not_of("0123456789");
				tc_time = 60000 * stoi(tokens[2].substr(0, indx2));
			}
			else {
				size_t indx2 = tokens[2].find_first_not_of("0123456789", indx + 1);
				tc_time = 60000 * stoi(tokens[2].substr(0, indx));
				if (indx2 == std::string::npos) {
					tc_time += 1000 * stoi(tokens[2].substr(indx + 1));
				}
				else {
					tc_time += stoi(tokens[2].substr(indx + 1, indx2 - indx - 1));
				}
			}
		}
		tc_increment = int(stod(tokens[3]) * 1000);
	}

	void XBoard::setboard(std::string line) {
		delete pos;
		pos = new Position(line.substr(9));
		if (xstate == ANALYZE) {
			Engine->StopThinking();
			go();
		}
	}

	//Opponent has played a move
	void XBoard::usermove(std::vector<std::string> tokens) {
		//Apply move to internal board
		Move move = parseMoveInXBoardNotation(tokens[1], *pos);
		Position * next = new Position(*pos);
		if (next->ApplyMove(move)) {
			moves.push_back(move);
			next->AppliedMovesBeforeRoot++;
			pos = next;
		}
		else sync_cout << "Illegal move: " << tokens[1] << sync_endl;
		if (xstate != FORCE) {
			//If engine is pondering the move is either a ponderhit or miss
			if (engineState.load() == Pondering) {
				if (move == ponderMove) {
					//Ponderhit: We switch the engine state to thinking..
					Engine->PonderMode.store(false);
					engineState.store(Thinking);
					sync_cout << "# Thinking (usermove)" << sync_endl;
					//..and inform the timemanager
					Engine->timeManager.PonderHit();
				}
				else {
					//Ponder miss: stop the engine, anything else will be handled by the engine thread
					Engine->StopThinking();
				}
			}
			else if (engineState.load() == Waiting) {
				//Engine is in waiting state => wake it up and let it think
				go();
			}
		}
		//After a move is played clean the move filter
		Engine->searchMoves.clear();
	}

	void XBoard::includeMoves(std::vector<std::string> tokens) {
		if (!tokens[1].compare("all")) Engine->searchMoves.clear();
		else {
			for (int i = 1; i < int(tokens.size()); ++i) {
				Engine->searchMoves.push_back(parseMoveInXBoardNotation(tokens[1], *pos));
			}
		}
	}

	//When playing on FICS, the server informs about opponent's rating via the rating command
	//Contempt is adjusted based on rating difference
	void XBoard::rating(std::vector<std::string> tokens) {
		int ratingOpponent = stoi(tokens[2]);
		int ownRating = stoi(tokens[1]);
		if (ownRating == 0) ownRating = 2700;
		if (ratingOpponent == 0) ratingOpponent = 2000;
		settings::parameter.Contempt = Value((ownRating - ratingOpponent) / 10);
	}

	void XBoard::egtpath(std::vector<std::string> tokens) {
		tablebases::init(settings::options.getString(settings::OPTION_SYZYGY_PATH));
		if (tablebases::MaxCardinality < 3) {
			sync_cout << "Couldn't find any Tablebase files!!" << sync_endl;
			exit(1);
		}
		InitializeMaterialTable();
	}

	void XBoard::option(std::vector<std::string> tokens) {
		size_t indx = tokens[1].find('=');
		std::string name;
		std::string value;
		if (indx == std::string::npos) {
			name = tokens[1];
			value.clear();
		}
		else {
			name = tokens[1].substr(0, indx);
			value = tokens[1].substr(indx + 1);
		}
		if (!name.compare(settings::OPTION_CLEAR_HASH)) {
			tt::clear();
			pawn::clear();
			Engine->Reset();
		}
		else if (!name.compare(settings::OPTION_MULTIPV)) {
			Engine->MultiPv = stoi(value);
		}
		else if (!name.compare(settings::OPTION_CONTEMPT)) {
			settings::parameter.Contempt = Value(stoi(value));
		}
		else if (!name.compare(settings::OPTION_BOOK_FILE)) {
			((settings::OptionString *)settings::options[settings::OPTION_BOOK_FILE])->set(value);
			if (((settings::OptionString *)settings::options[settings::OPTION_BOOK_FILE])->getValue().empty()) ((settings::OptionCheck *)settings::options[settings::OPTION_OWN_BOOK])->set(false);
		}
		else if (!name.compare(settings::OPTION_OWN_BOOK)) {
			((settings::OptionCheck *)settings::options[settings::OPTION_OWN_BOOK])->set(!value.compare("1"));
		}
		else if (!name.compare(settings::OPTION_EMERGENCY_TIME)) {
			settings::parameter.EmergencyTime = stoi(value);
		}
		else if (!name.compare(settings::OPTION_SYZYGY_PROBE_DEPTH)) {
			settings::parameter.TBProbeDepth = stoi(value);
		}
	}

	void XBoard::clearPos() {
		if (pos == nullptr) return;
		pos->deleteParents();
		delete pos;
		pos = new Position();
	}

	void XBoard::prepareTimeManager(bool startPonder) {
		int64_t tnow = now();
		TimeMode mode = xstate == ANALYZE ? INFINIT :
			tc_movetime > 0 ? FIXED_TIME_PER_MOVE : UNDEF;
		int movestogo = 30;
		if (tc_moves > 0) movestogo = tc_moves - (moves.size() % tc_moves);

		Engine->timeManager.initialize(mode, tc_movetime, tc_maxDepth, INT64_MAX, int(time[EngineSide]), tc_increment, movestogo, tnow, startPonder);
	}

	std::string XBoard::toXboardString(Move move) {
		if (!Chess960 || type(move) != CASTLING) return toString(move);
		if (to(move) < from(move)) return "O-O-O"; else return "O-O";
	}

	Move XBoard::parseMoveInXBoardNotation(const std::string& xboardMove, const Position& pos) {
		if (!Chess960 || xboardMove[0] != 'O') return parseMoveInUCINotation(xboardMove, pos);
		Square kingSquare = pos.KingSquare(pos.GetSideToMove());
		if (!xboardMove.compare("O-O")) {
			return createMove<CASTLING>(kingSquare, InitialRookSquare[2 * pos.GetSideToMove()]);
		}
		else if (!xboardMove.compare("O-O-O")) {
			return createMove<CASTLING>(kingSquare, InitialRookSquare[2 * pos.GetSideToMove() + 1]);
		}
		return MOVE_NONE;
	}

}