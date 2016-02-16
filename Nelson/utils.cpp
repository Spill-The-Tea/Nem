#include <fstream>
#include <vector>
#include <exception>
#include <mutex>
#include <map>

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

	const std::string WHITESPACE = " \n\r\t";

	std::string TrimLeft(const std::string& s)
	{
		size_t startpos = s.find_first_not_of(WHITESPACE);
		return (startpos == std::string::npos) ? "" : s.substr(startpos);
	}

	std::string TrimRight(const std::string& s)
	{
		size_t endpos = s.find_last_not_of(WHITESPACE);
		return (endpos == std::string::npos) ? "" : s.substr(0, endpos + 1);
	}

	std::string Trim(const std::string& s)
	{
		return TrimRight(TrimLeft(s));
	}

	void replaceExt(std::string& s, const std::string& newExt) {
		std::string::size_type i = s.rfind('.', s.length());
		if (i != std::string::npos) {
			s.replace(i + 1, newExt.length(), newExt);
		}
	}

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


	std::vector<std::string> split(std::string str, char sep) {
		std::vector<std::string> tokens;
		std::stringstream ss(str); // Turn the string into a stream.
		std::string tok;
		while (std::getline(ss, tok, sep)) {
			tokens.push_back(tok);
		}
		return tokens;
	}

#ifdef TRACE
	const std::string s_desc[13] = { "UND", "MDP", "TT", "TB", "RAZ", "BP", "NMP", "PC", "CUT", "EXA", "FIN", "SP", "DP" };

	traceEntry::traceEntry(uint64_t nodeNumber, std::string path, int depth, uint64_t hash)
	{
		NodeNumber = nodeNumber;
		Path = path;
		Depth = depth;
		Score = VALUE_NOTYETDETERMINED;
		Type = ScoreType::UNDEFINED;
		Hash = hash;
	}

	std::vector<traceEntry> searchTree;

	void clearSearchTree()
	{
		searchTree.clear();
	}

	size_t addTraceEntry(uint64_t nodeNumber, std::string path, int depth, uint64_t hash) {
		searchTree.push_back(traceEntry(nodeNumber, path, depth, hash));
		return searchTree.size() - 1;
	}

	Value setScore(Value score, size_t index, ScoreType type)
	{
		searchTree[index].Score = score;
		searchTree[index].Type = type;
		return score;
	}

	void dumpSearchTreeToFile() {
		std::ofstream file;
		file.open("searchTree.txt");      
		for (auto const& entry : searchTree) {
			file << std::setw(8) << std::right << entry.NodeNumber 
				 << std::setw(17) << std::right << std::hex << entry.Hash
				 << std::setw(4) << std::right << std::dec << entry.Depth
				 << std::setw(7) << std::right << entry.Score
				 << " " << std::setw(4) << std::left << s_desc[(int)entry.Type]
				 << entry.Path << std::endl;
		}
		file.close();              
	}

#endif
}
