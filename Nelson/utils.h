#pragma once
#include <string>
#include <vector>

//copied from SF
enum SyncCout { IO_LOCK, IO_UNLOCK };
std::ostream& operator<<(std::ostream&, SyncCout);

#define sync_cout std::cout << IO_LOCK
#define sync_endl std::endl << IO_UNLOCK

namespace utils {

	void debugInfo(std::string info);
	void debugInfo(std::string info1, std::string info2);
	std::vector<std::string> split(std::string str, char sep = ' ');
	void replaceExt(std::string& s, const std::string& newExt);
	std::string TrimLeft(const std::string& s);
	std::string TrimRight(const std::string& s);
	std::string Trim(const std::string& s);

	uint64_t MurmurHash2A(uint64_t input, uint64_t seed = 0x4f4863d5038ea3a3);

	inline uint64_t Hash(uint64_t input) { return input * 14695981039346656037; }

	inline std::string bool2String(bool val) { if (val) return "true"; else return "false"; }

	template <class T> T clamp(T value, T lowerBound, T upperBound) { return std::max(lowerBound, std::min(value, upperBound)); }

	const double K = -1.13 / 400;
	inline double sigmoid(Value score) { return 1 / (1 + std::pow(10, K * (int)score)); }

	inline double interpolatedResult(double result, int ply, int totalPlies) {
		return 0.5 + ((result - 0.5) * ply) / totalPlies;
	}
	inline double winExpectation(Value score, double scale = 1.305) { return 1 / (1 + std::pow(10, -scale / 400 * (int)score)); }

	std::string mirrorFenVertical(std::string fen);

	double TexelTuneError(const char* argv[], int argc);

#ifdef TRACE
	enum ScoreType { UNDEFINED,
	                 MDP,   //Mate-Distance-Pruned
					 TT,    //Cut-off by TT lookup
		             EGTB,  //Cut-off by TB lookup
		             RAZ,   //Razoring
		             BP,    //Beta-Pruning
		             NMP,   //Null-Move-Pruning
		             PC,    //Probcut
		             BC,    //Beta-Cutoff
		             EXACT, //Exact (alpha-Cutoff)
		             FINAL, //Position is final
		             SP,    //Stand-Pat > beta
		             DP     //Delta-Pruning
	};

	class TraceEntry {
	public:
		uint64_t NodeNumber;
		std::string Path;
		int Depth;
		Value Score;
		ScoreType Type;
		uint64_t Hash;

		TraceEntry(uint64_t nodeNumber, std::string path, int depth, uint64_t hash);
	};

	void clearSearchTree();
	void dumpSearchTreeToFile();
	size_t addTraceEntry(uint64_t nodeNumber, std::string path, int depth, uint64_t hash);
	Value setScore(Value score, size_t index, ScoreType type = ScoreType::UNDEFINED);

#define SCORE(score) utils::setScore(score, traceIndex);
#define SCORE_MDP(score) utils::setScore(score, traceIndex, utils::ScoreType::MDP);
#define SCORE_TT(score) utils::setScore(score, traceIndex, utils::ScoreType::TT);
#define SCORE_TB(score) utils::setScore(score, traceIndex, utils::ScoreType::EGTB);
#define SCORE_RAZ(score) utils::setScore(score, traceIndex, utils::ScoreType::RAZ);
#define SCORE_BP(score) utils::setScore(score, traceIndex, utils::ScoreType::BP);
#define SCORE_NMP(score) utils::setScore(score, traceIndex, utils::ScoreType::NMP);
#define SCORE_PC(score) utils::setScore(score, traceIndex, utils::ScoreType::PC);
#define SCORE_BC(score) utils::setScore(score, traceIndex, utils::ScoreType::BC);
#define SCORE_EXACT(score) utils::setScore(score, traceIndex, utils::ScoreType::EXACT);
#define SCORE_FINAL(score) utils::setScore(score, traceIndex, utils::ScoreType::FINAL);
#define SCORE_SP(score) utils::setScore(score, traceIndex, utils::ScoreType::SP);
#define SCORE_DP(score) utils::setScore(score, traceIndex, utils::ScoreType::DP);
#else
#define SCORE(score) score
#define SCORE_MDP(score) score
#define SCORE_TT(score) score
#define SCORE_TB(score) score
#define SCORE_RAZ(score) score
#define SCORE_BP(score) score
#define SCORE_NMP(score) score
#define SCORE_PC(score) score
#define SCORE_BC(score) score
#define SCORE_EXACT(score) score
#define SCORE_FINAL(score) score
#define SCORE_SP(score) score
#define SCORE_DP(score) score
#endif
}


