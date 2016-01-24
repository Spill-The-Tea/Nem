#pragma once
#include <string>
#include <vector>
#include "search.h"

class UCIInterface {
public:
	void loop();

private:
	baseSearch * Engine = new search < SINGLE >;
	position * _position = nullptr;
	std::thread * Mainthread = nullptr;
	int64_t ponderStartTime = 0;
	bool ponderActive = false;
	bool initialized = false;

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
	void deleteThread();

	void copySettings(baseSearch * source, baseSearch * destination);
};

