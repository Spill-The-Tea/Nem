#include <fstream>
#include <vector>
#include <exception>
#include <mutex>

#include "types.h"
#include "uci.h"
#include "position.h"
#include "search.h"
#include "utils.h"



//copied from SF
std::ostream& operator<<(std::ostream& os, SyncCout sc) {

	static std::mutex m;

	if (sc == IO_LOCK) {
		m.lock();
	}

	if (sc == IO_UNLOCK)
		m.unlock();

	return os;
}


namespace utils {

	logger::logger() {

	}

	logger::~logger() {
		flush();
	}

	logger * logger::_instance = nullptr;

	logger * logger::instance() {
		if (_instance == nullptr) _instance = new logger();
		return _instance;
	}

	void logger::flush() {
		if (entries.size() > 0) {
			std::ofstream logfile("./log.txt", std::ofstream::out | std::ios::app);
			for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); ++it) {
				std::cout << *it << std::endl;
				logfile << *it << std::endl;
			}
			logfile.close();
			entries.clear();
		}
	}

	void debugInfo(std::string info)
	{
		switch (protocol) {
		case UCI: sync_cout << "info string " << info << sync_endl; break;
		case XBOARD: sync_cout << "# " << info << sync_endl; break;
		case NO_PROTOCOL: sync_cout << info << sync_endl; break;
		default: sync_cout << info << sync_endl; break;
		}
	}

	void debugInfo(std::string info1, std::string info2)
	{
		switch (protocol) {
		case UCI: sync_cout << "info string " << info1 << " " << info2 << sync_endl; break;
		case XBOARD: sync_cout << "# " << info1 << " " << info2 << sync_endl; break;
		case NO_PROTOCOL: sync_cout << info1 << " " << info2 << sync_endl; break;
		default: sync_cout << info1 << " " << info2 << sync_endl; break;
		}
	}

}
