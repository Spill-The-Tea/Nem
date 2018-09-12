#pragma once
#include <string>
#include <vector>
#include "search.h"

class UCIInterface {
public:
	void loop();

private:
	Search * Engine = new Search;
	Position * _position = nullptr;
	std::thread * Mainthread = nullptr;
	int64_t ponderStartTime = 0;
	bool ponderActive = false;
	bool initialized = false;

	bool dispatch(std::string line);

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
	void deleteThread();
	void see(std::vector<std::string> &tokens);
	void qscore(std::vector<std::string> &tokens);
#ifdef TUNE
	double ttlabeled(); //Calculates the Loss function for Texel tuning with labelled input file
	double ttWDL(); //Calculates the  Loss function for Texel tuning with 3 different input files containing wins, draws and losses
#endif
	void dumpTT(std::vector<std::string> &tokens);
	void updateFromOptions();
	void tb();
	void copySettings(Search * source, Search * destination);
};

