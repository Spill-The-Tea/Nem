#include <iostream>
#include "xboard.h"
#include "utils.h"

namespace cecp {

	xboard::xboard()
	{
		ponder.store(false);
		drawOffered.store(false);
		engineState.store(Waiting);
		Mainthread = new std::thread(&xboard::run, this);
	}


	xboard::~xboard()
	{
		deleteThread();
		delete Engine;
		position * prev;
		do {
			prev = pos->Previous();
			delete pos;
			pos = prev;
		} while (prev != nullptr);
	}

	void xboard::run() {
		while (engineState.load() != Exiting) {
			std::unique_lock<std::mutex> lckStart(mtxStart);
			while (engineState.load() == Waiting) cvStart.wait(lckStart);
			if (engineState.load() == Thinking) {
				//sync_cout << "Start Thinking" << sync_endl;
				//EngineSide = pos->GetSideToMove();
				Engine->PonderMode.store(false);
				prepareTimeManager(false);
				//sync_cout << Engine->timeManager.print() << sync_endl;
				Engine->Stop.store(false);
				if (dynamic_cast<search<MASTER>*>(Engine)) BestMove = (dynamic_cast<search<MASTER>*>(Engine))->Think(*pos);
				else BestMove = (dynamic_cast<search<SINGLE>*>(Engine))->Think(*pos);
				if (drawOffered.load() && BestMove.score < -Contempt) {
					sync_cout << "offer draw" << sync_endl;
					drawOffered.store(false);
				}
				sync_cout << "move " << toXboardString(BestMove.move) << sync_endl;
				position * next = new position(*pos);
				next->ApplyMove(BestMove.move);
				moves.push_back(BestMove.move);
				next->AppliedMovesBeforeRoot++;
				pos = next;
				if (processResult(BestMove.score) && ponder.load() && engineState.load() == Thinking && (ponderMove = Engine->PonderMove())) {
					engineState.store(Pondering);
				} else engineState.store(Waiting);
			} else if (engineState.load() == Pondering) {
				//sync_cout << "Start Pondering" << sync_endl;
				if (ponderpos) {
					delete ponderpos;
					ponderpos = nullptr;
				}
				ponderpos = new position(*pos);
				ponderpos->ApplyMove(ponderMove);
				ponderpos->AppliedMovesBeforeRoot++;
				Engine->PonderMode.store(true);
				prepareTimeManager(true);
				//sync_cout << Engine->timeManager.print() << sync_endl;
				Engine->Stop.store(false);
				//sync_cout << "Starting to ponder on " << toXboardString(ponderMove) << sync_endl;
				if (dynamic_cast<search<MASTER>*>(Engine)) BestMove = (dynamic_cast<search<MASTER>*>(Engine))->Think(*ponderpos);
				else BestMove = (dynamic_cast<search<SINGLE>*>(Engine))->Think(*ponderpos);
				//On ponder hit engine state is switched to Thinking
				if (engineState.load() == Thinking) {
					//We had a ponder hit => the search result is our next move
					//sync_cout << "Ponderhit detected " << toXboardString(BestMove.move) << sync_endl;
					sync_cout << "move " << toXboardString(BestMove.move) << sync_endl;
					position * next = new position(*pos);
					next->ApplyMove(BestMove.move);
					moves.push_back(BestMove.move);
					next->AppliedMovesBeforeRoot++;
					pos = next;
					//Check if game is over
					if (!processResult(BestMove.score)) engineState.store(Waiting);
					else if (ponder.load() && (ponderMove = Engine->PonderMove())) {
						engineState.store(Pondering);
					}
					else {
						//ponder is switched off, or we have no ponder move => let the engine sleep
						engineState.store(Waiting);
					}
				}
				else {
					//we had a ponder miss => we have to start thinking again
					//sync_cout << "Pondermiss " << sync_endl;
					engineState.store(Thinking);
				}
			}
			Engine->Reset();
		}
	}

	bool xboard::processResult(Value score) {
		DetailedResult dr = pos->GetDetailedResult();
		switch (dr)
		{
		case NO_RESULT:
			return true;
		case WHITE_MATES:
			sync_cout << "1-0 {White mates}" << sync_endl;
		case BLACK_MATES:
			sync_cout << "0-1 {Black mates}" << sync_endl;
		case DRAW_50_MOVES:
			if (score < -Contempt) sync_cout << "1/2-1/2 {50-move rule}" << sync_endl; else return true;
		case DRAW_STALEMATE:
			sync_cout << "1/2-1/2 {Stalemate}" << sync_endl;
		case DRAW_REPETITION:
			if (score < -Contempt) sync_cout << "1/2-1/2 {3-fold Repetition}" << sync_endl; else return true;
		case DRAW_MATERIAL:
			sync_cout << "1/2-1/2 {No mating material}" << sync_endl;
		default:
			return true;
		}
		return false;
	}

	//Only used when engine has received quit command
	void xboard::deleteThread() {
		engineState.store(Exiting);
		cvStart.notify_one();
		Engine->PonderMode.store(false);
		Engine->StopThinking();
		if (Mainthread != nullptr) {
			if (Mainthread->joinable())  Mainthread->join();
			else sync_cout << "info string Can't stop Engine Thread!" << sync_endl;
			free(Mainthread);
			Mainthread = nullptr;
		}
		Engine->Reset();
	}

	void printResult(Color stm, Value score)
	{
		if (score == 0) sync_cout << "1/2-1/2" << sync_endl;
		if ((score > VALUE_DRAW && stm == WHITE) || (score < VALUE_DRAW && stm == BLACK)) sync_cout << "1-0" << sync_endl;
		else sync_cout << "0-1" << sync_endl;
	}

	void xboard::loop() {
		std::string line;
		Engine->XBoardOutput = true;
		while (std::getline(std::cin, line)) {
			if (!dispatch(line)) break;
		}
	}

	bool xboard::dispatch(std::string line) {
		std::vector<std::string> tokens = split(line);
		if (tokens.size() == 0) return true;
		std::string command = tokens[0];
		if (!command.compare("quit")) {
			deleteThread();
			//utils::logger::instance()->flush();
			exit(EXIT_SUCCESS);
		}
		else if (!command.compare("usermove")) {
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
		else if (!command.compare("ping")) {
			sync_cout << "pong " << tokens[1] << sync_endl;
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
			EngineSide = Color(EngineSide^1);
		}
		else if (!command.compare("analyze")) {
			analyze();
		}
		else if (line[0] == '.') {
			sync_cout << Engine->XAnalysisOutput << sync_endl;
		}
#ifdef TB
		else if (!command.compare("egtpath")) {
			egtpath(tokens);
		}
#endif
		return true;
	}

	void xboard::go() {
		if (xstate == FORCE) xstate = STANDARD;
		EngineSide = pos->GetSideToMove();
		engineState.store(Thinking);
		cvStart.notify_one();
	}

	void xboard::analyze() {
		xstate = ANALYZE;
		if (engineState.load() == Thinking) {
			Engine->timeManager.switchToInfinite();
			return;
		}
		else if (engineState.load() == Pondering) Engine->StopThinking();
		go();
	}

	void xboard::variant(std::vector<std::string> tokens) {
		Chess960 = !tokens[1].compare("fischerandom");
	}

	void xboard::memory(std::vector<std::string> tokens) {
		int hashSize = stoi(tokens[1]);
		if (hashSize != HashSizeMB) {
			HashSizeMB = hashSize;
			tt::InitializeTranspositionTable(HashSizeMB);
		}
	}

	void xboard::cores(std::vector<std::string> tokens) {
		HelperThreads = stoi(tokens[1]) - 1;
		if (HelperThreads && Engine->GetType() == SINGLE) {
			delete Engine;
			Engine = new search < MASTER >;
			//Todo timemanagement settings are lost when engine is reinitialized
		}
		else if (!HelperThreads && Engine->GetType() == MASTER) {
			delete Engine;
			Engine = new search < SINGLE >;
			//Todo timemanagement settings are lost when engine is reinitialized
		}
	}

	void xboard::protover(std::vector<std::string> tokens) {
		protocolVersion = stoi(tokens[1]);
		sync_cout << "feature ping=1 setboard=1 usermove=1 time=1 draw=1 sigint=0 sigterm=0 reuse=0 myname=\"Nemorino\""
		          << " variants=\"normal,fischerrandom\" colors=0 ics=1 name=1 include=1"
		          << " memory=1 smp=1 ics=1 name=1"
#ifdef TB
		          << " egt = \"syzygy\""
#endif
		          << sync_endl;
				  sync_cout << "feature option=\"Clear Hash -button\"" << sync_endl;
				  sync_cout << "feature option=\"MultiPV -spin " << Engine->MultiPv << " 1 216\"" << sync_endl;
				  sync_cout << "feature option=\"Contempt -spin 0 -1000 1000\"" << sync_endl;
				  sync_cout << "feature option=\"BookFile -file " << BOOK_FILE.c_str() << "\"" << sync_endl;
				  sync_cout << "feature option=\"OwnBook -check " << (USE_BOOK ? "1\"" : "0\"") << sync_endl;
				  sync_cout << "feature done=1" << sync_endl;
	}

	void xboard::newGame() {
		Engine->StopThinking();
		engineState.store(Waiting);
		clearPos();
		pos = new position();
		EngineSide = BLACK;
	}

	void xboard::moveNow() {
		Engine->StopThinking();
	}

	void xboard::sd(std::vector<std::string> tokens) {
		tc_maxDepth = stoi(tokens[1]);
	}

	void xboard::st(std::vector<std::string> tokens) {
		tc_movetime = 1000 * stoi(tokens[1]);
		tc_time = 0;
		tc_moves = 0;
		tc_increment = 0;
	}

	void xboard::level(std::vector<std::string> tokens) {
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

	void xboard::setboard(std::string line) {
		delete pos;
		pos = new position(line.substr(9));
		if (xstate == ANALYZE) {
			Engine->StopThinking();
			go();
		}
	}


	void xboard::usermove(std::vector<std::string> tokens) {
		//sync_cout << "Usermove received " << tokens[1] << sync_endl;
		Move move = parseMoveInXBoardNotation(tokens[1], *pos);
		position * next = new position(*pos);
		if (next->ApplyMove(move)) {
			moves.push_back(move);
			next->AppliedMovesBeforeRoot++;
			pos = next;
		}
		else sync_cout << "Illegal move: " << tokens[1] << sync_endl;
		if (xstate != FORCE) {
			if (engineState.load() == Pondering) {
				if (move == ponderMove) {
					//Ponderhit
					Engine->PonderMode.store(false);
					engineState.store(Thinking);
					Engine->timeManager.PonderHit();
				}
				else {
					//Ponder miss
					Engine->StopThinking();
				}
			}
			else if (engineState.load() == Waiting) {
				go();
			}
		}
		Engine->searchMoves.clear();
	}

	void xboard::includeMoves(std::vector<std::string> tokens) {
		if (!tokens[1].compare("all")) Engine->searchMoves.clear();
		else {
			for (int i = 1; i < int(tokens.size()); ++i) {
				Engine->searchMoves.push_back(parseMoveInXBoardNotation(tokens[1], *pos));
			}
		}
	}

#ifdef TB
	void xboard::egtpath(std::vector<std::string> tokens) {
		SYZYGY_PATH = tokens[1];
		Tablebases::init(SYZYGY_PATH);
		if (Tablebases::MaxCardinality < 3) {
			sync_cout << "Couldn't find any Tablebase files!!" << sync_endl;
			exit(1);
		}
		InitializeMaterialTable();
	}
#endif

	void xboard::option(std::vector<std::string> tokens) {
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
		if (!name.compare("Clear Hash")) {
			tt::clear();
			pawn::clear();
			Engine->Reset();
		}
		else if (!name.compare("MultiPV")) {
			Engine->MultiPv = stoi(tokens[1]);
		}
		else if (!name.compare("Contempt")) {
			Contempt = Value(stoi(tokens[1]));
		}
		else if (!name.compare("BookFile")) {
			BOOK_FILE = value;
			if (BOOK_FILE.empty()) USE_BOOK = false;
		}
		else if (!name.compare("OwnBook")) {
			USE_BOOK = !value.compare("1");
		}
	}

	void xboard::clearPos() {
		if (pos == nullptr) return;
		pos->deleteParents();
		delete pos;
		pos = new position();
	}

	void xboard::prepareTimeManager(bool startPonder) {
		int64_t tnow = now();
		TimeMode mode = xstate == ANALYZE ? INFINIT :
			                                tc_movetime > 0 ? FIXED_TIME_PER_MOVE : UNDEF;
		int movestogo = 30;
		if (tc_moves > 0) movestogo = tc_moves - (moves.size() % tc_moves);

		Engine->timeManager.initialize(mode, tc_movetime, tc_maxDepth, INT64_MAX, int(time[EngineSide]), tc_increment, movestogo, tnow, startPonder);
	}

	std::vector<std::string> xboard::split(std::string str) {
		std::vector<std::string> tokens;
		std::stringstream ss(str); // Turn the string into a stream.
		std::string tok;

		while (std::getline(ss, tok, ' ')) {
			tokens.push_back(tok);
		}

		return tokens;
	}

	std::string xboard::toXboardString(Move move) {
		if (!Chess960 || type(move) != CASTLING) return toString(move);
		if (to(move) < from(move)) return "O-O-O"; else return "O-O";
	}

	Move xboard::parseMoveInXBoardNotation(const std::string& xboardMove, const position& pos) {
		if (!Chess960 || xboardMove[0] != 'O') return parseMoveInUCINotation(xboardMove, pos);
		Square kingSquare = lsb(pos.PieceBB(KING, pos.GetSideToMove()));
		if (!xboardMove.compare("O-O")) {
			return createMove<CASTLING>(kingSquare, InitialRookSquare[2 * pos.GetSideToMove()]);
		}
		else if (!xboardMove.compare("O-O-O")) {
			return createMove<CASTLING>(kingSquare, InitialRookSquare[2 * pos.GetSideToMove() + 1]);
		}
		return MOVE_NONE;
	}

}