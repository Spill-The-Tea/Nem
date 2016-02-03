#pragma once

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "types.h"
#include "book.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include "timemanager.h"
#include "utils.h"

#ifdef TB
#include "syzygy/tbprobe.h"
#endif

#ifdef STAT
#include "stat.h"
#endif


enum ThreadType { SINGLE, MASTER, SLAVE };
enum SearchResultType { EXACT_RESULT, FAIL_LOW, FAIL_HIGH };

/* The baseSearch and search structs represent basically the engine. The "design" shows what happens when and experienced developer coming from "real" OO languages
   (like Java or C# - with interfaces, ...) meets C++. I don't like it, but it works and there is no need to refactor it!
   We have a baseSearch, which acts as the engine interface to the outside and a templated derived class, which represents the engine threads. The engine is using Lazy SMP,
   currently all inter-thread communication is done via the transposition table.
*/
struct baseSearch {
public:
	//the search itself issues the info output while thinking. To do is it needs to know whether it's running in xboard, or uci mode. 
	//This is of course from OO perspective bad, and the output should be done in the resp. protocol drivers, but as long as the number
	//of protocols will stay at 2, it's easier, than creating an generic info class/struct, which then can be handled by the different protocol drivers
	bool UciOutput = false;
	bool XBoardOutput = false;
	bool PrintCurrmove = true;
	//Flag indicating the engine is currently pondering
	std::atomic<bool> PonderMode;
	//The search's result (will be updated while searching)
	ValuatedMove BestMove;
	//Total Node Count (including nodes from Quiescence Search)
	int64_t NodeCount = 0;
	//Quiescence Search Node Count
	int64_t QNodeCount = 0;
	//Maximum Depth reached during Search
	int MaxDepth = 0;
	//Flag, indicating that search shall stop as soon as possible 
	std::atomic<bool> Stop;
	//Flag indicating that search shall be exited (will be called from destructor to stop engine threads)
	std::atomic<bool> Exit;
	//Polyglot bookfile if provided
	std::string BookFile = "";
	int rootMoveCount = 0;
	ValuatedMove * rootMoves = nullptr;
	position rootPosition;
	//For analysis purposes, list of root moves which shall be considered
	std::vector<Move> searchMoves;
	//Number of root moves for which full analysis shall be done
	int MultiPv = 1;
	//The timemanager, which handles the time assignment and checks if a search shall be continued
	timemanager timeManager;

	baseSearch();
	virtual ~baseSearch();
	//Re-initialize BestMove, History, CounterMoveHistory, Killer, Node counters, ....
	void Reset();
	//Determines the Principal Variation (PV is stored during search in "triangular" array and is completed from hash table, if
	//it's incomplete (due to prunings)
	std::string PrincipalVariation(position & pos, int depth = PV_MAX_LENGTH);
	//Reset engine to start a new game
	void NewGame();
	//Returns the current nominal search depth
	inline int Depth() const { return _depth; }
	//Returns the thinkTime needed so far (is updated after every iteration)
    inline Time_t ThinkTime() const { return _thinkTime; }
	//Statistic data no needed for playing
	inline double cutoffAt1stMoveRate() const { return 100.0 * cutoffAt1stMove / cutoffCount; }
	inline double cutoffAverageMove() const { return 1 + 1.0 * cutoffMoveIndexSum / cutoffCount; }
	//Determines the move the engine assumes that the opponent is playing
	inline Move PonderMove() const { return ponderMove; }
	//Stops the current search immediatialy
	inline void StopThinking() {
		PonderMode.store(false);
		Stop.store(true);
	}
	//Get's the "best" move from the polyglot opening book
	Move GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount);
	//There are 3 types of engine threads: SINGLE (in single-thread mode) and MASTER and SLAVE (in SMP mode)
	virtual ThreadType GetType() = 0;
	//returns the analysis output for xboard in thread-safe mode 
	std::string GetXAnalysisOutput();
	//handling of the thinking output for uci and xboard
	void info(position &pos, int pvIndx, SearchResultType srt = EXACT_RESULT);

	void debugInfo(std::string info);

protected:
	//Mutex to synchronize access to analysis output
	std::mutex mtxXAnalysisOutput;
	//Polyglot book object to read book moves
	polyglot::book * book = nullptr;
	//History tables used in move ordering during search
	CounterMoveHistoryManager cmHistory;
	HistoryManager History;
	//Killer moves: 2 slots per ply from Root (so at ply N the indices 2*N and 2*N+1 contain the relevant killer moves)
	ExtendedMove killer[2 * MAX_DEPTH];
	//Internal statistics (to get a measure of move ordering quality)
	uint64_t cutoffAt1stMove = 0;
	uint64_t cutoffCount = 0;
	uint64_t cutoffMoveIndexSum = 0;
	//in xboard protocol in analysis mode, the GUI determines the point in time when information shall be provided (in UCI the engine determines these 
	//point in times. As this information is only available while the search's recursion level is root the information is prepared, whenever feasible and
	//stored in member XAnylysisOutput, where the protocol driver can retrieve it at any point in time
	std::string XAnalysisOutput;

	//Array where moves from Principal Variation are stored
	Move PVMoves[PV_MAX_LENGTH];
	//Move which engine expects opponent will reply to it's next move
	Move ponderMove = MOVE_NONE;
	int _depth = 0;
	Time_t _thinkTime;
	Move counterMove[12 * 64];
#ifdef TB
	uint64_t tbHits = 0;
	//Flag indicating whether TB probes shall be made during search
	bool probeTB = true;
#endif

	inline bool Stopped() { return Stop; }

};

template<ThreadType T> struct search : public baseSearch {
public:
	//Reference to the Master Thread (only set when T == SLAVE
	baseSearch * master = nullptr;

	search();
	~search();
	//Main entry point 
	inline ValuatedMove Think(position &pos);
	//In case of SMP, start Slave threads
	void startHelper();
	//Initialize slave threads (once per move)
	void initializeSlave(baseSearch * masterSearch, int id);
	ThreadType GetType() { return T; }
	std::condition_variable cvStart;
	std::mutex mtxStart;
	//reference to slave objects (only set when T == MASTER)
	search<SLAVE> * slaves = nullptr;
	//slave threads
	std::vector<std::thread> subThreads;
	//Utility method (not used when thinking). Checks if a position is quiet (that's static evalution is about the same as QSearch result). Might be
	//useful for tuning
	bool isQuiet(position &pos);

private:
	//Main recursive search method
	template<bool PVNode> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv, bool prune = true, bool skipNullMove = false);
	//At root level there is a different search method (as there is some special logic requested)
	Value SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv, int startWithMove = 0);
	//Quiescence Search (different implementations for positions in check and not in check)
	template<bool WithChecks> Value QSearch(Value alpha, Value beta, position &pos, int depth);
	//Updates killer, history and counter move history tables, whenever a cutoff has happened
	void updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex);
	//Thread id (0: MASTER or SINGLE, SLAVES are sequentially numbered starting with 1
	int id = 0;

};

//This method is a in theory useless indirection to start the slave threads as it only calls slave.startHelper(), which is the worker method of the slave thread.
//Unfortunately when trying to launch start.helper directly on thread creation there is an inexplicable compiler error C1001 on MSVC. As this indirection works
//and doesn't harm I'll leave it
void startThread(search<SLAVE> & slave);

template<ThreadType T> inline ValuatedMove search<T>::Think(position &pos) {
	//Initialize Engine before starting the new search
	_thinkTime = 1; //to avoid divide by 0 errors
	ponderMove = MOVE_NONE;
	Value score = VALUE_ZERO;
	rootPosition = pos;
	pos.ResetPliesFromRoot();
	EngineSide = pos.GetSideToMove();
	tt::newSearch();
	//Get all root moves
	ValuatedMove * generatedMoves = pos.GenerateMoves<LEGAL>();
	rootMoveCount = pos.GeneratedMoveCount();
	if (rootMoveCount == 0) {
		BestMove.move = MOVE_NONE;
		BestMove.score = VALUE_ZERO;
		utils::debugInfo("No valid move!");
		return BestMove;
	}
	if (rootMoveCount == 1) {
		BestMove = *generatedMoves; //if there is only one legal move save time and return move immediately (although there is no score assigned)
		utils::debugInfo("Only one valid move!");
		goto END;
	}
	//check if book is available
	if (USE_BOOK && BookFile.size() > 0) {
		if (book == nullptr) book = new polyglot::book(BookFile);
		//Currently engine isn't looking for the best book move, but selects on of the available bookmoves by random, with entries weight used to define the
		//probability 
		Move bookMove = book->probe(pos, false, generatedMoves, rootMoveCount);
		if (bookMove != MOVE_NONE) {
			BestMove.move = bookMove;
			BestMove.score = VALUE_ZERO;
			utils::debugInfo("Book move");
			//Try to find a suitable move for pondering
			position next(pos);
			if (next.ApplyMove(bookMove)) {
				ValuatedMove* replies = next.GenerateMoves<LEGAL>();
				ponderMove = book->probe(next, true, replies, next.GeneratedMoveCount());
				if (ponderMove == MOVE_NONE && next.GeneratedMoveCount() > 0) ponderMove = replies->move;
			}
			goto END;
		}
	}
	//If a search move list is provided replace root moves by srach moves
	if (searchMoves.size()) {
		rootMoveCount = int(searchMoves.size());
		rootMoves = new ValuatedMove[rootMoveCount];
		for (int i = 0; i < rootMoveCount; ++i) rootMoves[i].move = searchMoves[i];
		searchMoves.clear();
	}
	else {
		rootMoves = new ValuatedMove[rootMoveCount];
		memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	}
#ifdef TB
	//Root probing of tablebases. This is done as suggested by SF: Keep only the winning, resp. drawing, moves in the move list
	//and then search normally. This way the engine will play "better" than by simply choosing the "best" tablebase move (which is
	//the move which minimizes the number until drawPlayCount is reset without changing the result
	tbHits = 0;
	probeTB = true;
	if (pos.GetMaterialTableEntry()->IsTablebaseEntry()) {
		std::vector<ValuatedMove> tbMoves(rootMoves, rootMoves + rootMoveCount);
		bool tbHit = Tablebases::root_probe(pos, tbMoves, score);
		if (!tbHit) tbHit = Tablebases::root_probe_wdl(pos, tbMoves, score);
		if (tbHit) {
			tbHits++;
			delete[] rootMoves;
			rootMoveCount = (int)tbMoves.size();
			rootMoves = new ValuatedMove[rootMoveCount];
			std::copy(tbMoves.begin(), tbMoves.end(), rootMoves);
			if (rootMoveCount == 1) {
				BestMove = rootMoves[0]; //if tablebase probe only returns one move play => play it and done!
				utils::debugInfo("Only one tablebase move preserving the result!");
				goto END;
			}
			probeTB = false;
		}
	}
#endif
	//Initialize PV-Array
	std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	if (T == MASTER) {
		//SMP active => start helper threads (if not yet done)
		if (slaves == nullptr) {
			slaves = new search<SLAVE>[HelperThreads];
			for (int i = 0; i < HelperThreads; ++i) subThreads.push_back(std::thread(&startThread, std::ref(slaves[i])));
		}
		//Initialize slaves and wake them up
		for (int i = 0; i < HelperThreads; ++i) {
			//slaves[i].initializeSlave(this);			
			slaves[i].mtxStart.lock();
			slaves[i].initializeSlave(this, i + 1);
			slaves[i].mtxStart.unlock();
			slaves[i].cvStart.notify_one();
		}
	}
	//Special logic to get static evaluation via UCI: if go depth 0 is requested simply return static evaluation
	if (timeManager.GetMaxDepth() == 0) {
		BestMove.move = MOVE_NONE;
		BestMove.score = pos.evaluate();
		if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD)) sync_cout << "info score cp " << BestMove.score << sync_endl;
		else {
			int pliesToMate;
			if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
			sync_cout << "info score mate " << pliesToMate / 2 << sync_endl;
		}
		return BestMove;
	}
	//Iterativ Deepening Loop
	for (_depth = 1; _depth <= timeManager.GetMaxDepth(); ++_depth) {
		Value alpha, beta, delta = Value(20);
		for (int pvIndx = 0; pvIndx < MultiPv; ++pvIndx) {
			if (_depth >= 5 && MultiPv == 1 && std::abs(score) < VALUE_KNOWN_WIN) {
				//set aspiration window
				alpha = std::max(score - delta, -VALUE_INFINITE);
				beta = std::min(score + delta, VALUE_INFINITE); 
			}
			else {
				alpha = -VALUE_MATE;
				beta = VALUE_MATE;
			}
			while (true) {
				score = SearchRoot(alpha, beta, pos, _depth, PVMoves, pvIndx);
				//Best move is already in first place, this is assured by SearchRoot
				//therefore we sort only the other moves
				std::stable_sort(rootMoves + pvIndx + 1, &rootMoves[rootMoveCount], sortByScore);
				if (Stopped()) {
					break;
				}
			    if (score <= alpha) {
					//fail-low
					beta = (alpha + beta) / 2;
					alpha = std::max(score - delta, -VALUE_INFINITE);
					debugInfo("Fail-Low");
					//inform timemanager to assigne more time
					if (!PonderMode.load()) timeManager.reportFailLow();
				}
				else if (score >= beta) {
					//fail-high
					alpha = (alpha + beta) / 2;
					beta = std::min(score + delta, VALUE_INFINITE);
					debugInfo("Fail-High");
				}
				else {
					//Iteration completed
					BestMove = rootMoves[0];
					score = BestMove.score;
					break;
				}
				if (std::abs(score) >= VALUE_KNOWN_WIN) {
					alpha = -VALUE_MATE;
					beta = VALUE_MATE;
				}
				else delta += delta / 4 + 5; //widening formula is from SF - very small widening of the aspiration window size, more might be better => let's tune it some day
			}
			Time_t tNow = now();
			_thinkTime = std::max(tNow - timeManager.GetStartTime(), int64_t(1));
			Stop.load();
			if (!Stopped()) {
				//check if new deeper iteration shall be started
				if (!timeManager.ContinueSearch(_depth, BestMove, NodeCount, tNow, PonderMode)) {
					Stop.store(true);
				}
			} else debugInfo("Iteration cancelled!");
			//send information to GUI
			info(pos, pvIndx);
			if (Stopped()) break;
		}
		if (Stopped()) break;
	}

	if (T == MASTER) {
		//search is done, send helper threads to sleep
		for (int i = 0; i < HelperThreads; ++i) slaves[i].StopThinking();
	}

END://when pondering engine must not return a best move before opponent moved => therefore let main thread wait	
	bool infoSent = false;
	while (PonderMode.load()) {
		if (!infoSent) utils::debugInfo("Waiting for opponent..");
		infoSent = true;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	if (PVMoves[0] != MOVE_NONE && PVMoves[0] != BestMove.move) {
		std::stringstream ss;
		ss << "PV Move: " << toString(PVMoves[0]) << " Bestmove: " << toString(BestMove.move);
		utils::debugInfo(ss.str());
		BestMove.move = PVMoves[0];
	}
	//If for some reason search did not find a best move return the  first one (to avoid loss and it's anyway the best guess then)
	if (BestMove.move == MOVE_NONE) BestMove = rootMoves[0];
	if (rootMoves) delete[] rootMoves;
	rootMoves = nullptr;
	return BestMove;
}

//prepare slave thread for next search (set root position, moves and material table entry)
template<ThreadType T> void search<T>::initializeSlave(baseSearch * masterSearch, int Id) {
	if (T == SLAVE) {
		master = masterSearch;
		id = Id;
		rootMoveCount = masterSearch->rootMoveCount;
		rootMoves = new ValuatedMove[rootMoveCount];
		memcpy(rootMoves, masterSearch->rootMoves, rootMoveCount * sizeof(ValuatedMove));
		rootPosition = position(masterSearch->rootPosition);
		rootPosition.copy(masterSearch->rootPosition);
		rootPosition.InitMaterialPointer();
		Stop.store(false);
	}
}

//slave thread: slave threads are searching until stopped and then send to sleep. When a new position is searched, then they are
//woke up again. As guard against spurious wake-ups the pointer to the master search is used (this is of course strange and should be replaced by some atomic flag)
template<ThreadType T> void search<T>::startHelper() {
	while (!Exit.load()) {
		std::unique_lock<std::mutex> lckStart(mtxStart);
		while (master == nullptr) cvStart.wait(lckStart); //Sleep
		int depth = 1;
		std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
		//Iterative Deepening Loop
		while (!Stop && depth <= MAX_DEPTH) {
			Value alpha = -VALUE_MATE;
			Value beta = VALUE_MATE;
			SearchRoot(alpha, beta, rootPosition, depth, &PVMoves[0]);
			std::stable_sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
			++depth;
		}
		master = nullptr;
	}
	delete[] rootMoves;
}

template<ThreadType T> search<T>::search() {  }

template<ThreadType T> search<T>::~search() {
	//stop and delete Slave Threads
	if (T == MASTER && slaves != nullptr) {
		for (int i = 0; i < HelperThreads; ++i) {
			slaves[i].Exit.store(true);
			slaves[i].master = this;
			slaves[i].cvStart.notify_one();
			slaves[i].StopThinking();
			subThreads[i].join();
		}
		delete[] slaves;
	}
}

template<ThreadType T> Value search<T>::SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv, int startWithMove) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Value bonus;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	pv[1] = MOVE_NONE; //pv[1] will be the ponder move, so we should make sure, that it is initialized as well
	bool ZWS = false;  //ZWS: Zero Window Search
	bool lmr = !pos.Checked() && depth >= 3;
	//move loop
	for (int i = startWithMove; i < rootMoveCount; ++i) {
		if (T != SLAVE) {
			//Send information about the current move to GUI
			if (UciOutput && PrintCurrmove && depth > 5 && (now() - timeManager.GetStartTime()) > 3000) {
				sync_cout << "info depth " << depth << " currmove " << toString(rootMoves[i].move) << " currmovenumber " << i + 1 << sync_endl;
			}
			else if (XBoardOutput && timeManager.Mode() == INFINIT) {
				std::lock_guard<std::mutex> lck(mtxXAnalysisOutput);
				std::stringstream ss;
				ss << "stat01: " << (now() - timeManager.GetStartTime()) / 10 << " " << NodeCount << " "
					<< depth << " " << rootMoveCount - i << " " << rootMoveCount + 1 << " " << toString(rootMoves[i].move);
				XAnalysisOutput = ss.str();
			}
		}
		//apply move
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		//determine reduction (Late Move Reduction)
		int reduction = 0;
		if (lmr && i >= startWithMove + 5 && pos.IsQuietAndNoCastles(rootMoves[i].move) && !next.Checked()) {
			reduction = 1;
		}
		//To make engine prefer castling add bonus for castling moves (see https://chessprogramming.wikispaces.com/Ronald+de+Man#Scoring%20Root%20Moves)
		if (type(rootMoves[i].move) == CASTLING) bonus = BONUS_CASTLING; else bonus = VALUE_ZERO;
		if (ZWS) {
			score = bonus - Search<false>(Value(bonus - alpha - 1), bonus - alpha, next, depth - 1 - reduction, subpv, !next.GetMaterialTableEntry()->SkipPruning());
			if (score > alpha && score < beta)
				//Research without reduction and with full alpha-beta window
				score = bonus - Search<true>(bonus - beta, bonus - alpha, next, depth - 1, subpv, !next.GetMaterialTableEntry()->SkipPruning());
		}
		else {
			score = bonus - Search<true>(bonus - beta, bonus - alpha, next, depth - 1 - reduction, subpv, !next.GetMaterialTableEntry()->SkipPruning());
			if (reduction > 0 && score > alpha && score < beta)
				//Research without reduction
				score = bonus - Search<true>(bonus - beta, bonus - alpha, next, depth - 1, subpv, !next.GetMaterialTableEntry()->SkipPruning());
		}
		if (Stopped()) break;
		rootMoves[i].score = score;
		if (score > bestScore) {
			bestScore = score;
			pv[0] = rootMoves[i].move;
			memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
			if (i > 0) {
				//make sure that best move is always in first place 
				ValuatedMove bm = rootMoves[i];
				for (int j = i; j > 0; --j) rootMoves[j] = rootMoves[j - 1];
				rootMoves[0] = bm;
			}
			if (score >= beta) return score;
			else if (score > alpha)
			{
				ZWS = true;
				alpha = score;
			}
		}
	}
	//	}
	return bestScore;
}

//This is the main alpha-beta search routine
template<ThreadType T> template<bool PVNode> Value search<T>::Search(Value alpha, Value beta, position &pos, int depth, Move * pv, bool prune, bool skipNullMove) {
	++NodeCount;
	if (T != SLAVE) {
		if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
	}
	if (Stopped()) return VALUE_ZERO;
	if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	//Mate distance pruning
	alpha = Value(std::max(int(-VALUE_MATE) + pos.GetPliesFromRoot(), int(alpha)));
	beta = Value(std::min(int(VALUE_MATE) - pos.GetPliesFromRoot() - 1, int(beta)));
	if (alpha >= beta) return alpha;
	//If depth = 0 is reached go to Quiescence Search
	if (depth <= 0) {
		return QSearch<true>(alpha, beta, pos, depth);
	}
	//TT lookup
	bool ttFound;
	tt::Entry ttEntry;
	tt::Entry* ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	Move ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		if (ttMove && pos.IsQuiet(ttMove)) updateCutoffStats(ttMove, depth, pos, -1);
		return ttValue;
	}
#ifdef TB
	// Tablebase probe
	// Probing is only done if root position was no tablebase position (probeTB is true) and if drawPlayCount = 0 (in that case we get the necessary WDL information) to return
	// immediately
	if (probeTB && depth >= SYZYGY_PROBE_DEPTH && pos.GetDrawPlyCount() == 0 && pos.GetMaterialTableEntry()->IsTablebaseEntry())
	{
		int found, v = Tablebases::probe_wdl(pos, &found);
		if (found)
		{
			tbHits++;
			Value value = v < -1 ? -VALUE_MATE + MAX_DEPTH + pos.GetPliesFromRoot()
				: v >  1 ? VALUE_MATE - MAX_DEPTH - pos.GetPliesFromRoot()
				: VALUE_DRAW + 2 * v;
			if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			return value;
		}
	}
#endif
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool checked = pos.Checked();
	Value staticEvaluation = checked ? VALUE_NOTYETDETERMINED :
		ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate() + BONUS_TEMPO;
	if (!skipNullMove && prune && !checked) {
		//Check if Value from TT is better
		Value effectiveEvaluation = staticEvaluation;
		if (ttFound &&
			((ttValue > staticEvaluation && ttEntry.type() == tt::LOWER_BOUND)
				|| (ttValue < staticEvaluation && ttEntry.type() == tt::UPPER_BOUND))) effectiveEvaluation = ttValue;

		//Razoring a la SF 
		if (!checked && depth < 4
			&& (effectiveEvaluation + Value(256 + 16 * depth)) <= alpha
			&& ttMove == MOVE_NONE
			&& !pos.PawnOn7thRank())
		{
			if (depth <= 1 && (effectiveEvaluation + 302) <= alpha) return QSearch<true>(alpha, beta, pos, 0);
			Value razorAlpha = alpha - Value(256 + 16 * depth);
			Value razorScore = QSearch<true>(razorAlpha, Value(razorAlpha + 1), pos, 0);
			if (razorScore <= razorAlpha) return razorScore;
		}
		// Beta Pruning
		if (depth < 7
			&& effectiveEvaluation < VALUE_KNOWN_WIN
			&& (effectiveEvaluation - BETA_PRUNING_FACTOR * depth) >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove()))
			return effectiveEvaluation - BETA_PRUNING_FACTOR * depth;
		//Null Move Pruning
		if (depth > 1 //only if there is available depth to reduce
			&& effectiveEvaluation >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove())
			) {
			int reduction = (depth + 14) / 5;
			Square epsquare = pos.GetEPSquare();
			pos.NullMove();
			Value nullscore = -Search<false>(-beta, -alpha, pos, depth - reduction, subpv, false, true);
			pos.NullMove(epsquare);
			if (nullscore >= beta) {
				if (nullscore >= VALUE_MATE_THRESHOLD) nullscore = beta;
				if (depth < 9 && beta < VALUE_KNOWN_WIN) return nullscore;
				// Do verification search at high depths
				Value verificationScore = Search<false>(beta - 1, beta, pos, depth - reduction, subpv, false);
				if (verificationScore >= beta) return nullscore;
			}
		}

		// ProbCut (copied from SF)
		if (!PVNode
			&&  depth >= 5
			&& std::abs(int(beta)) < VALUE_MATE_THRESHOLD)
		{
			Value rbeta = std::min(Value(beta + 90), VALUE_INFINITE);
			int rdepth = depth - 4;

			position cpos(pos);
			cpos.copy(pos);
			Value limit = PieceValuesMG[GetPieceType(pos.getCapturedInLastMove())];
			Move ttm = ttMove;
			if (ttm != MOVE_NONE && cpos.SEE(ttMove) < limit) ttm = MOVE_NONE;
			cpos.InitializeMoveIterator<QSEARCH>(&History, &cmHistory, nullptr, MOVE_NONE, ttm, limit);
			Move move;
			while ((move = cpos.NextMove())) {
				position next(cpos);
				if (next.ApplyMove(move)) {
					Value score = -Search<false>(-rbeta, Value(-rbeta + 1), next, rdepth, subpv, !next.GetMaterialTableEntry()->SkipPruning());
					if (score >= rbeta)
						return score;
				}
			}
		}
	}
	//IID: No Hash move available => Try to find one with a reduced search
	if (!ttMove && PVNode ? depth >= 4 : depth > 6) {
		position next(pos);
		next.copy(pos);
		//As there is no Hash move available, let's do a reduced search to get one
		//Logically pruning and null move should be skipped here, as we are looking for a move and not for a cutoff,
		//but at short time control there was no improvement and at longer time controls the engine played worse (Don't know why) without pruning and null-move
		Search<PVNode>(alpha, beta, next, PVNode ? depth - 2 : depth / 2, subpv, true);
		if (Stopped()) return VALUE_ZERO;
		ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
		ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	}
	Move counter = pos.GetCounterMove(counterMove);
	//Futility Pruning I: If quiet moves can't raise alpha, only generate tactical moves and moves which give check
	bool futilityPruning = !skipNullMove && !checked && depth <= FULTILITY_PRUNING_DEPTH && beta < VALUE_MATE_THRESHOLD && pos.NonPawnMaterial(pos.GetSideToMove());
	if (futilityPruning && alpha > (staticEvaluation + FUTILITY_PRUNING_LIMIT[depth]))
		pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, &cmHistory, nullptr, counter, ttMove);
	else
		pos.InitializeMoveIterator<MAIN_SEARCH>(&History, &cmHistory, &killer[2 * pos.GetPliesFromRoot()], counter, ttMove);
	Value score;
	Value bestScore = -VALUE_MATE;
	tt::NodeType nodeType = tt::UPPER_BOUND;
	bool lmr = !checked && depth >= 3;
	Move move;
	int moveIndex = 0;
	bool ZWS = false;
	Square recaptureSquare = pos.GetLastAppliedMove() && pos.Previous()->GetPieceOnSquare(to(pos.GetLastAppliedMove())) != BLANK ? to(pos.GetLastAppliedMove()) : OUTSIDE;
	while ((move = pos.NextMove())) {
		//Late move Pruning I
		//if (!PVNode && !checked && !critical && depth <= 3 && moveIndex >= LMPMoveCount[depth]) {
		//	moveIndex++;
		//	continue;
		//}
		bool prunable = !PVNode && !checked && move != ttMove && move != counter && std::abs(int(bestScore)) <= VALUE_MATE_THRESHOLD
			&& move != killer[2 * pos.GetPliesFromRoot()].move && move != killer[2 * pos.GetPliesFromRoot() + 1].move && pos.IsQuietAndNoCastles(move);
		if (prunable) {
			// late-move pruning II
			if (depth <= 3 && moveIndex >= depth * 4) {
				moveIndex++;
				continue;
			}
			//SEE pruning
			if (depth <= 3 && pos.SEE_Sign(move) < 0) {
				moveIndex++;
				continue;
			}
		}
		position next(pos);
		if (next.ApplyMove(move)) {
			bool critical = move == counter || !pos.QuietMoveGenerationPhaseStarted() || moveIndex == 0;
			//Check extension
			int extension = (next.Checked() && pos.SEE_Sign(move) >= VALUE_ZERO) ? 1 : 0;
			if (!extension && to(move) == recaptureSquare) {
				++extension;
			}
			int reduction = 0;
			//LMR: Late move reduction
			if (lmr && !critical && moveIndex >= 2 && !extension && !next.Checked()) {
				if (PVNode) {
					if (moveIndex >= 5) reduction = 1;
				}
				else {
					if (moveIndex >= 5) reduction = depth / 3; else reduction = 1;
				}
			}
			if (ZWS) {
				score = -Search<false>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction + extension, subpv, !next.GetMaterialTableEntry()->SkipPruning());
				if (score > alpha && score < beta) score = -Search<PVNode>(-beta, -alpha, next, depth - 1 + extension, subpv, !next.GetMaterialTableEntry()->SkipPruning());
			}
			else {
				score = -Search<PVNode>(-beta, -alpha, next, depth - 1 - reduction + extension, subpv, !next.GetMaterialTableEntry()->SkipPruning());
				if (score > alpha && reduction > 0) score = -Search<PVNode>(-beta, -alpha, next, depth - 1 + extension, subpv, !next.GetMaterialTableEntry()->SkipPruning());
			}

			if (score >= beta) {
				updateCutoffStats(move, depth, pos, moveIndex);
				//Update transposition table
				if (T != SINGLE)  ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				return score;
			}
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha)
				{
					ZWS = true;
					nodeType = tt::EXACT;
					alpha = score;
					pv[0] = move;
					memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
				}
			}
		}
		moveIndex++;
	}
	//Update transposition table
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	return bestScore;
}

template<ThreadType T> template<bool WithChecks> Value search<T>::QSearch(Value alpha, Value beta, position &pos, int depth) {
	++QNodeCount;
	if (!WithChecks) {
		MaxDepth = std::max(MaxDepth, pos.GetPliesFromRoot());
		//Ugly!! to avoid counting nodes twice the total node count is only incremented when WithChecks is false. This is based on the fact, that only at depth = 0
		//QSearch<true> is called. When this changes this will break!
		++NodeCount;
		if (T == MASTER) {
			if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
		}
		if (Stopped()) return VALUE_ZERO;
		if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	}
	//TT lookup
	tt::Entry ttEntry;
	bool ttFound;
	tt::Entry* ttPointer;
	ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		return ttValue;
	}
	bool checked = pos.Checked();
	int ttDepth = WithChecks || checked ? 0 : -1;
	Value standPat;
	if (checked) {
		standPat = VALUE_NOTYETDETERMINED;
	}
	else {
		standPat = ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate() + BONUS_TEMPO;
		//check if ttValue is better
		if (ttFound && ttValue != VALUE_NOTYETDETERMINED && ((ttValue > standPat && ttEntry.type() == tt::LOWER_BOUND) || (ttValue < standPat && ttEntry.type() == tt::UPPER_BOUND))) standPat = ttValue;
		if (standPat >= beta) {
			if (T == SINGLE) ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, MOVE_NONE, standPat);
			else ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, MOVE_NONE, standPat);
			return beta;
		}
		//Delta Pruning
		if (!pos.GetMaterialTableEntry()->IsLateEndgame()) {
			Value delta = PieceValuesEG[pos.GetMostValuableAttackedPieceType()] + int(pos.PawnOn7thRank()) * (PieceValuesEG[QUEEN] - PieceValuesEG[PAWN]) + DELTA_PRUNING_SAFETY_MARGIN;
			if (standPat + delta < alpha) return alpha;
		}
		if (alpha < standPat) alpha = standPat;
	}
	if (WithChecks) pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, &cmHistory, nullptr, MOVE_NONE);
	else pos.InitializeMoveIterator<QSEARCH>(&History, &cmHistory, nullptr, MOVE_NONE);
	Move move;
	Value score;
	tt::NodeType nt = tt::UPPER_BOUND;
	while ((move = pos.NextMove())) {
		if (pos.SEE_Sign(move) < VALUE_ZERO) continue;
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<false>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, move, standPat);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, move, standPat);
				return beta;
			}
			if (score > alpha) {
				nt = tt::EXACT;
				alpha = score;
			}
		}
	}
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), alpha, nt, ttDepth, MOVE_NONE, standPat);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), alpha, nt, ttDepth, MOVE_NONE, standPat);
	return alpha;
}

template<ThreadType T> void search<T>::updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex) {
	if (moveIndex >= 0) {
		cutoffMoveIndexSum += moveIndex;
		cutoffCount++;
		cutoffAt1stMove += moveIndex == 0;
	}
	if (moveIndex < 0 || pos.QuietMoveGenerationPhaseStarted()) {
		Square toSquare = to(cutoffMove);
		Piece movingPiece = pos.GetPieceOnSquare(from(cutoffMove));
		if (moveIndex >= 0) {
			ExtendedMove killerMove(movingPiece, cutoffMove);
			int pfr = pos.GetPliesFromRoot();
			killer[2 * pfr + 1] = killer[2 * pfr];
			killer[2 * pfr] = killerMove;
		}
		Value v = Value(depth * depth);
		History.update(v, movingPiece, toSquare);
		Piece prevPiece = BLANK;
		Square prevTo = OUTSIDE;
		Move lastApplied;
		if ((lastApplied = FixCastlingMove(pos.GetLastAppliedMove()))) {
			prevTo = to(lastApplied);
			prevPiece = pos.GetPieceOnSquare(prevTo);
			counterMove[(pos.GetPieceOnSquare(prevTo) << 6) + prevTo] = cutoffMove;
			cmHistory.update(v, prevPiece, prevTo, movingPiece, toSquare);
		}
		if (moveIndex > 0) {
			ValuatedMove * alreadyProcessedQuiets = pos.GetMovesOfCurrentPhase();
			for (int i = 0; i < pos.GetMoveNumberInPhase() - 1; ++i) {
				Move alreadyProcessedMove = FixCastlingMove(alreadyProcessedQuiets->move);
				movingPiece = pos.GetPieceOnSquare(from(alreadyProcessedMove));
				toSquare = to(alreadyProcessedMove);
				History.update(-v, movingPiece, toSquare);
				if (pos.GetLastAppliedMove())
					cmHistory.update(-v, prevPiece, prevTo, movingPiece, toSquare);
				alreadyProcessedQuiets++;
			}
		}
	}
}

template<ThreadType T> bool search<T>::isQuiet(position &pos) {
	Value evaluationDiff = pos.GetStaticEval() - QSearch<true>(-VALUE_MATE, VALUE_MATE, pos, 0);
	return std::abs(evaluationDiff) <= 30;
}