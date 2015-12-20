#pragma once

#include <vector>
#include <thread>
#include <condition_variable> 
#include <mutex>
#include <atomic>

#include "types.h"
#include "position.h"
#include "search.h"
#include "board.h"

namespace cecp {

	enum State { Waiting, Thinking, Pondering, Exiting };
	enum XState { STANDARD, FORCE, ANALYZE };

	class xboard
	{
	public:
		xboard();
		~xboard();

		void loop();
	private:
		void deleteThread();
		void thinkAsync();
		void run();
		void printResult(Color stm, Value score);
		bool dispatch(std::string line);
		static std::vector<std::string> split(std::string str);
		void level(std::vector<std::string> tokens);
		void st(std::vector<std::string> tokens);
		void sd(std::vector<std::string> tokens);
		void protover(std::vector<std::string> tokens);
		void variant(std::vector<std::string> tokens);
		void usermove(std::vector<std::string> tokens);
		void option(std::vector<std::string> tokens);
		void setboard(std::string line);
		void memory(std::vector<std::string> tokens);
		void cores(std::vector<std::string> tokens);
		void includeMoves(std::vector<std::string> tokens);
		void analyze();
#ifdef TB
		void egtpath(std::vector<std::string> tokens);
#endif
		void moveNow();
		void newGame();
		void go();
		void clearPos();
		void prepareTimeManager(bool startPonder = false);
		bool processResult(Value score = VALUE_NOTYETDETERMINED);
		static std::string toXboardString(Move move);
		static Move parseMoveInXBoardNotation(const std::string& xboardMove, const position& pos);

		baseSearch * Engine = new search < SINGLE >;
		position * pos = new position;
		position * ponderpos = nullptr;
		std::vector<Move> moves;
		std::thread * Mainthread = nullptr;
		ValuatedMove BestMove;
		Move ponderMove = MOVE_NONE;
		Color EngineSide = BLACK;
		int64_t time[2];
		int protocolVersion = 1;
		XState xstate = STANDARD;
		int tc_increment = 0;
		int tc_moves = 0;
		int64_t tc_time = 0;
		int tc_movetime = 0;
		int tc_maxDepth = MAX_DEPTH;

		std::condition_variable cvStart;
		std::mutex mtxStart;
		std::atomic<bool> ponder;
		std::atomic<State> engineState;
		std::atomic<bool> drawOffered;
	};

}