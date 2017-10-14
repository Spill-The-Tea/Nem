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

/*Xboard protocol driver
  So far there are still disconnects and time losses when running Nemorino in xboard mode with ponder on.
  Therefore UCI-mode is still preferred
  The forfeits happen about once every 30 games and always while the engine is pondering. The reason is still unclear.
  It doesn't seem to be related to lack of time as it happens as well with longer time controls and while the engine is still
  having a lot of time available. I t might also be a cutechess issue, as at least once the engine was terminated although it has
  sent a move in time.
  This class manages 2 threads, the main thread is listening to STDIN and communicating with the GUI. It manages a 2nd thread (the 
  engine thread) by waking it up, sending it to sleep, updating the engine's timemanager and the engine's position
*/
namespace cecp {

	enum State { Waiting, Thinking, Pondering, Exiting };
	enum XState { STANDARD, FORCE, ANALYZE };

	class XBoard
	{
	public:
		XBoard();
		~XBoard();
		//The main thread, which basically is an endless loop listening to STDIN
		void loop();
	private:
		//The engine thread
		void run();
		//Stops the engine
		void deleteThread();
		//Dispatches commands from the GUI
		bool dispatch(std::string line);
		//Utility method tokenizing the GUI's commands
		static std::vector<std::string> split(std::string str);
		//Sends a result string to the GUI
		void printResult(Color stm, Value score);
		//The methods below are handlers for the different commands sent by the GUI
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
		void rating(std::vector<std::string> tokens);
		void includeMoves(std::vector<std::string> tokens);
		void analyze();
		void egtpath(std::vector<std::string> tokens);
		void moveNow();
		void newGame();
		//Wake up the engine and let it start thinking
		void go();
		//Resets the engine's position
		void clearPos();
		//Updates the engine'S timemanager with the actual time values
		void prepareTimeManager(bool startPonder = false);
		//Check if game is finished and sends the appropriate result message to the GUI
		bool processResult(Value score = VALUE_NOTYETDETERMINED);
		//Converts a move to the string representation defined by xboard protocol
		static std::string toXboardString(Move move);
		//Converts a move represented as xboard string to the internal representation
		static Move parseMoveInXBoardNotation(const std::string& xboardMove, const Position& pos);

		//The Engine
		Search * Engine = new Search;
		//The current position of the game
		Position * pos = new Position;
		//While pondering the position to ponder on
		Position * ponderpos = nullptr;
		//The move list of the current game
		std::vector<Move> moves;
		//The engine thread (executing method run()
		std::thread * Enginethread = nullptr;
		//The engine's best move
		ValuatedMove BestMove;
		//While pondering the move the engine is pondering on
		Move ponderMove = MOVE_NONE;
		//The side engine is playing
		Color EngineSide = BLACK;
		//The clocks  
		int64_t time[2] = { 0, 0 };
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