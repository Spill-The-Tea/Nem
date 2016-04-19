#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include "types.h"

class Logger {

public:

	static Logger& instance()
	{
		static Logger _instance;
		return _instance;
	}

	// dtor
	~Logger() {

		if (myFile.is_open()) {
			myFile << std::endl;
			myFile.close();
		} // if

	}

	inline void save() {
		if (myFile.is_open()) {
			myFile << std::endl;
			myFile.close();
		}
	}

	// Overload << operator using C style strings
	// No need for std::string objects here
	friend Logger &operator << (Logger &logger, const char *text) {
		logger.myFile << text << std::endl;
		return logger;
	}

	friend Logger &operator << (Logger &logger, std::string text) {
		logger.myFile << text << std::endl;
		return logger;
	}

	// Make it Non Copyable (or you can inherit from sf::NonCopyable if you want)
	Logger(const Logger &);
	Logger &operator= (const Logger &);



private:

	Logger()
	{
		std::string fname = "log" + std::to_string(now()) + ".txt";
		myFile.open(fname);

		// Write the first lines
		if (myFile.is_open()) {
			myFile << "Log file created" << std::endl;
		} // if
	}

	std::ofstream myFile;

}; // class end

#define LOG(text) Logger::instance() << text;
#define LOGSAVE  Logger::instance().save();
