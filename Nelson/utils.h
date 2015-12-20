#pragma once
#include <string>
#include <vector>

//copied from SF
enum SyncCout { IO_LOCK, IO_UNLOCK };
std::ostream& operator<<(std::ostream&, SyncCout);

#define sync_cout std::cout << IO_LOCK
#define sync_endl std::endl << IO_UNLOCK

namespace utils {

	class logger {
	public:
		logger();
		~logger();
		inline void add(std::string line) { entries.push_back(line); }
		void flush();

		static logger * instance();
	private:
		std::vector<std::string> entries;
		static logger * _instance;
	};
}


