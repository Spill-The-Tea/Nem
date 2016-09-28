#include <iostream>
#include "settings.h"
#include "utils.h"
#include "hashtables.h"

int HashSizeMB = 32;

std::string BOOK_FILE = "book.bin";
int HelperThreads = 0;
Value Contempt = VALUE_ZERO;
Color EngineSide = WHITE;
Protocol protocol = NO_PROTOCOL;
int EmergencyTime = 100;

//Value PASSED_PAWN_BONUS[4] = { Value(10), Value(30), Value(60), Value(100) };
//Value BETA_PRUNING_FACTOR = Value(200);

#ifdef TUNE
eval PieceValues[7]{ eval(950), eval(490, 550), eval(325), eval(325), eval(80, 100), eval(VALUE_KNOWN_WIN), eval(0) };
eval PASSED_PAWN_BONUS[6] = { eval(0), eval(0), eval(30), eval(37), eval(77), eval(140) };
eval BONUS_PROTECTED_PASSED_PAWN[6] = { eval(0), eval(0), eval(0), eval(30), eval(30), eval(30) };
#endif


namespace settings {

	static int LMR_REDUCTION[64][64];
	const eval PSQT[12][64]{
		{  // Piece Type 0
			eval(-10,-8), eval(-7,-5), eval(-4,-2), eval(-2,0), eval(-2,0), eval(-4,-2), eval(-7,-5), eval(-10,-8),  // Rank 1
			eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 2
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 3
			eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 4
			eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 5
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 6
			eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 7
			eval(-7,-8), eval(-4,-5), eval(-2,-2), eval(0,0), eval(0,0), eval(-2,-2), eval(-4,-5), eval(-7,-8)  // Rank 8
		},
		{  // Piece Type 1
			eval(7,8), eval(4,5), eval(2,2), eval(0,0), eval(0,0), eval(2,2), eval(4,5), eval(7,8),  // Rank 1
			eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 2
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 3
			eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 4
			eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 5
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 6
			eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 7
			eval(10,8), eval(7,5), eval(4,2), eval(2,0), eval(2,0), eval(4,2), eval(7,5), eval(10,8)  // Rank 8
		},
		{  // Piece Type 2
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 1
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 2
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 3
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 4
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 5
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 6
			eval(3,0), eval(3,0), eval(4,0), eval(6,0), eval(6,0), eval(4,0), eval(3,0), eval(3,0),  // Rank 7
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0)  // Rank 8
		},
		{  // Piece Type 3
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 1
			eval(-3,0), eval(-3,0), eval(-4,0), eval(-6,0), eval(-6,0), eval(-4,0), eval(-3,0), eval(-3,0),  // Rank 2
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 3
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 4
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 5
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 6
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 7
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0)  // Rank 8
		},
		{  // Piece Type 4
			eval(-12,-8), eval(-10,-5), eval(-7,-2), eval(-4,0), eval(-4,0), eval(-7,-2), eval(-10,-5), eval(-12,-8),  // Rank 1
			eval(-4,-5), eval(3,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(3,-2), eval(-4,-5),  // Rank 2
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 3
			eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 4
			eval(0,0), eval(5,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(5,2), eval(0,0),  // Rank 5
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 6
			eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 7
			eval(-7,-8), eval(-4,-5), eval(-2,-2), eval(0,0), eval(0,0), eval(-2,-2), eval(-4,-5), eval(-7,-8)  // Rank 8
		},
		{  // Piece Type 5
			eval(7,8), eval(4,5), eval(2,2), eval(0,0), eval(0,0), eval(2,2), eval(4,5), eval(7,8),  // Rank 1
			eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 2
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 3
			eval(0,0), eval(-5,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-5,-2), eval(0,0),  // Rank 4
			eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 5
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 6
			eval(4,5), eval(-3,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(-3,2), eval(4,5),  // Rank 7
			eval(12,8), eval(10,5), eval(7,2), eval(4,0), eval(4,0), eval(7,2), eval(10,5), eval(12,8)  // Rank 8
		},
		{  // Piece Type 6
			eval(-19,-18), eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(-3,-2), eval(-8,-7), eval(-14,-12), eval(-19,-18),  // Rank 1
			eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(4,5), eval(4,5), eval(-3,-2), eval(-8,-7), eval(-14,-12),  // Rank 2
			eval(-8,-7), eval(-3,-2), eval(4,5), eval(15,16), eval(15,16), eval(4,5), eval(-3,-2), eval(-8,-7),  // Rank 3
			eval(-3,-2), eval(4,5), eval(15,16), eval(21,22), eval(21,22), eval(15,16), eval(4,5), eval(-3,-2),  // Rank 4
			eval(-3,-2), eval(4,5), eval(21,16), eval(26,22), eval(26,22), eval(21,16), eval(4,5), eval(-3,-2),  // Rank 5
			eval(-8,-7), eval(-3,-2), eval(10,5), eval(31,16), eval(31,16), eval(10,5), eval(-3,-2), eval(-8,-7),  // Rank 6
			eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(4,5), eval(4,5), eval(-3,-2), eval(-8,-7), eval(-14,-12),  // Rank 7
			eval(-19,-18), eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(-3,-2), eval(-8,-7), eval(-14,-12), eval(-19,-18)  // Rank 8
		},
		{  // Piece Type 7
			eval(19,18), eval(14,12), eval(8,7), eval(3,2), eval(3,2), eval(8,7), eval(14,12), eval(19,18),  // Rank 1
			eval(14,12), eval(8,7), eval(3,2), eval(-4,-5), eval(-4,-5), eval(3,2), eval(8,7), eval(14,12),  // Rank 2
			eval(8,7), eval(3,2), eval(-10,-5), eval(-31,-16), eval(-31,-16), eval(-10,-5), eval(3,2), eval(8,7),  // Rank 3
			eval(3,2), eval(-4,-5), eval(-21,-16), eval(-26,-22), eval(-26,-22), eval(-21,-16), eval(-4,-5), eval(3,2),  // Rank 4
			eval(3,2), eval(-4,-5), eval(-15,-16), eval(-21,-22), eval(-21,-22), eval(-15,-16), eval(-4,-5), eval(3,2),  // Rank 5
			eval(8,7), eval(3,2), eval(-4,-5), eval(-15,-16), eval(-15,-16), eval(-4,-5), eval(3,2), eval(8,7),  // Rank 6
			eval(14,12), eval(8,7), eval(3,2), eval(-4,-5), eval(-4,-5), eval(3,2), eval(8,7), eval(14,12),  // Rank 7
			eval(19,18), eval(14,12), eval(8,7), eval(3,2), eval(3,2), eval(8,7), eval(14,12), eval(19,18)  // Rank 8
		},
		{  // Piece Type 8
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0),  // Rank 1
			eval(-17,-11), eval(-9,-11), eval(-7,-11), eval(-9,-11), eval(-9,-11), eval(-7,-11), eval(-9,-11), eval(-17,-11),  // Rank 2
			eval(-15,-9), eval(-12,-9), eval(-7,-9), eval(-1,-9), eval(-1,-9), eval(-7,-9), eval(-12,-9), eval(-15,-9),  // Rank 3
			eval(-17,-6), eval(-9,-6), eval(-7,-6), eval(6,-6), eval(6,-6), eval(-7,-6), eval(-9,-6), eval(-17,-6),  // Rank 4
			eval(-9,-3), eval(-7,-3), eval(-4,-3), eval(9,-3), eval(9,-3), eval(-4,-3), eval(-7,-3), eval(-9,-3),  // Rank 5
			eval(3,9), eval(6,9), eval(9,9), eval(11,9), eval(11,9), eval(9,9), eval(6,9), eval(3,9),  // Rank 6
			eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20),  // Rank 7
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0)  // Rank 8
		},
		{  // Piece Type 9
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0),  // Rank 1
			eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20),  // Rank 2
			eval(-3,-9), eval(-6,-9), eval(-9,-9), eval(-11,-9), eval(-11,-9), eval(-9,-9), eval(-6,-9), eval(-3,-9),  // Rank 3
			eval(9,3), eval(7,3), eval(4,3), eval(-9,3), eval(-9,3), eval(4,3), eval(7,3), eval(9,3),  // Rank 4
			eval(17,6), eval(9,6), eval(7,6), eval(-6,6), eval(-6,6), eval(7,6), eval(9,6), eval(17,6),  // Rank 5
			eval(15,9), eval(12,9), eval(7,9), eval(1,9), eval(1,9), eval(7,9), eval(12,9), eval(15,9),  // Rank 6
			eval(17,11), eval(9,11), eval(7,11), eval(9,11), eval(9,11), eval(7,11), eval(9,11), eval(17,11),  // Rank 7
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0)  // Rank 8
		},
		{  // Piece Type 10
			eval(16,-20), eval(17,-15), eval(16,-9), eval(14,-4), eval(14,-4), eval(14,-9), eval(17,-15), eval(16,-20),  // Rank 1
			eval(16,-15), eval(16,-9), eval(14,-4), eval(11,7), eval(11,7), eval(14,-4), eval(16,-9), eval(16,-15),  // Rank 2
			eval(11,-9), eval(11,-4), eval(11,7), eval(8,23), eval(8,23), eval(11,7), eval(11,-4), eval(11,-9),  // Rank 3
			eval(8,-4), eval(8,7), eval(3,23), eval(-2,28), eval(-2,28), eval(3,23), eval(8,7), eval(8,-4),  // Rank 4
			eval(3,-4), eval(0,7), eval(-2,23), eval(-7,28), eval(-7,28), eval(-2,23), eval(0,7), eval(3,-4),  // Rank 5
			eval(-7,-9), eval(-7,-4), eval(-12,7), eval(-18,23), eval(-18,23), eval(-12,7), eval(-7,-4), eval(-7,-9),  // Rank 6
			eval(-12,-15), eval(-12,-9), eval(-18,-4), eval(-18,7), eval(-18,7), eval(-18,-4), eval(-12,-9), eval(-12,-15),  // Rank 7
			eval(-18,-20), eval(-18,-15), eval(-18,-9), eval(-18,-4), eval(-18,-4), eval(-18,-9), eval(-18,-15), eval(-18,-20)  // Rank 8
		},
		{  // Piece Type 11
			eval(18,20), eval(18,15), eval(18,9), eval(18,4), eval(18,4), eval(18,9), eval(18,15), eval(18,20),  // Rank 1
			eval(12,15), eval(12,9), eval(18,4), eval(18,-7), eval(18,-7), eval(18,4), eval(12,9), eval(12,15),  // Rank 2
			eval(7,9), eval(7,4), eval(12,-7), eval(18,-23), eval(18,-23), eval(12,-7), eval(7,4), eval(7,9),  // Rank 3
			eval(-3,4), eval(0,-7), eval(2,-23), eval(7,-28), eval(7,-28), eval(2,-23), eval(0,-7), eval(-3,4),  // Rank 4
			eval(-8,4), eval(-8,-7), eval(-3,-23), eval(2,-28), eval(2,-28), eval(-3,-23), eval(-8,-7), eval(-8,4),  // Rank 5
			eval(-11,9), eval(-11,4), eval(-11,-7), eval(-8,-23), eval(-8,-23), eval(-11,-7), eval(-11,4), eval(-11,9),  // Rank 6
			eval(-16,15), eval(-16,9), eval(-14,4), eval(-11,-7), eval(-11,-7), eval(-14,4), eval(-16,9), eval(-16,15),  // Rank 7
			eval(-16,20), eval(-17,15), eval(-16,9), eval(-14,4), eval(-14,4), eval(-14,9), eval(-17,15), eval(-16,20)  // Rank 8
		}
	};

	void initPSQT()
	{
#ifdef _DEBUG
		//std::stringstream ssc;
		//ssc << "{ " << std::endl;
		//for (int pt = int(WQUEEN); pt <= int(BKING); ++pt) {
		//	ssc << "\t{  // Piece Type " << pt << std::endl;
		//	for (int rank = 0; rank < 8; ++rank) {
		//		ssc << "\t\t";
		//		for (int file = 0; file < 8; ++file) {
		//			ssc << " eval(" << PSQT[pt][8 * rank + file].mgScore << "," << PSQT[pt][8 * rank + file].egScore << ")";
		//			if (file < 7) ssc << ",";
		//		}
		//		if (rank < 7) ssc << ",";
		//		ssc << "  // Rank " << rank + 1 << std::endl;
		//	}
		//	if (pt < int(BKING)) ssc << "\t}," << std::endl; else ssc << "\t}" << std::endl;
		//}
		//ssc << "};" << std::endl;
		//std::cout << ssc.str() << std::endl;
		//std::stringstream ssx;
		//ssx << "PieceType;Rank;File;MGScore;EGScore" << std::endl;
		//for (int pt = int(QUEEN); pt <= int(KING); ++pt) {
		//	for (int rank = 0; rank < 8; ++rank) {
		//		for (int file = 0; file < 8; ++file) {
		//			ssx << pt << ";" << rank << ";" << file << ";" << PSQT[2 * pt][8 * rank + file].mgScore << ";" << PSQT[2 * pt][8 * rank + file].egScore << std::endl;
		//		}
		//	}
		//}
		//std::cout << ssx.str() << std::endl;
#endif
	}

	void Initialize() {
		initPSQT();
		for (int depth = 0; depth < 64; depth++) {
			for (int moves = 0; moves < 64; moves++) {
				double reduction = std::log(moves) * std::log(depth) / 2; //F
				if (reduction < 0.8) LMR_REDUCTION[depth][moves] = 0;
				else LMR_REDUCTION[depth][moves] = int(std::round(reduction));
			}
		}
	}

	int LMRReduction(int depth, int moveNumber)
	{
		return LMR_REDUCTION[std::min(depth, 63)][std::min(moveNumber, 63)];
	}

#ifdef TB
	int TBProbeDepth = 8;
#endif

#ifdef TUNE
	void processParameter(std::vector<std::string> parameters)
	{
		for (int i = 1; i < parameters.size(); ++i) {
			sync_cout << "info string " << parameters[i] << sync_endl;
			size_t indx = parameters[i].find('=');
			if (indx == std::string::npos) continue;
			std::string key = parameters[i].substr(0, indx);
			std::string value = parameters[i].substr(indx + 1);
			int val = stoi(value);
			if (!key.compare("QMG")) PieceValues[QUEEN].mgScore = Value(val);
			else if (!key.compare("QEG")) PieceValues[QUEEN].egScore = Value(val);
			else if (!key.compare("RMG")) PieceValues[ROOK].mgScore = Value(val);
			else if (!key.compare("REG")) PieceValues[ROOK].egScore = Value(val);
			else if(!key.compare("BMG")) PieceValues[BISHOP].mgScore = Value(val);
			else if (!key.compare("BEG")) PieceValues[BISHOP].egScore = Value(val);
			else if (!key.compare("NMG")) PieceValues[KNIGHT].mgScore = Value(val);
			else if (!key.compare("NEG")) PieceValues[KNIGHT].egScore = Value(val);
			else if (!key.compare("PMG")) PieceValues[PAWN].mgScore = Value(val);
			else if (!key.compare("PEG")) PieceValues[PAWN].mgScore = Value(val);
			else if (!key.compare(0, 3, "PPB")) {
				int indx = stoi(key.substr(3));
				PASSED_PAWN_BONUS[indx] = eval(val);
			}
			else if (!key.compare(0, 4, "BPPP")) {
				int indx = stoi(key.substr(4));
				BONUS_PROTECTED_PASSED_PAWN[indx] = eval(val);
			}
		}
		sync_cout << "info string parameters set" << sync_endl;
		for (int i = 0; i < 7; ++i) sync_cout << "info string " << PieceValues[i].mgScore << " " << PieceValues[i].egScore << sync_endl;
	}
#endif

	Options options;

	Option::Option(std::string Name, OptionType Type, std::string DefaultValue, std::string MinValue, std::string MaxValue)
	{
		name = Name;
		otype = Type;
		defaultValue = DefaultValue;
		maxValue = MaxValue;
		minValue = MinValue;
	}

	std::string Option::printUCI()
	{
		std::stringstream ss;
		ss << "option name " << name << " type ";
		switch (otype)
		{
		case OptionType::BUTTON:
			ss << "button";
			return ss.str();
		case OptionType::CHECK:
			ss << "check";
			if (defaultValue.size() > 0) ss << " default " << defaultValue;
			return ss.str();
		case OptionType::STRING:
			ss << "string";
			if (defaultValue.size() > 0) ss << " default " << defaultValue;
			return ss.str();
		case OptionType::SPIN:
			ss << "spin";
			if (defaultValue.size() > 0) ss << " default " << defaultValue;
			if (minValue.size() > 0) ss << " min " << minValue;
			if (maxValue.size() > 0) ss << " max " << maxValue;
			return ss.str();
		default:
			break;
		}
		return ss.str();
	}

	void OptionThread::set(std::string value)
	{
		_value = stoi(value);
		HelperThreads = _value - 1;
	}

	void Option960::set(std::string value)
	{
		Chess960 = !value.compare("true");
	}

	Options::Options()
	{
		initialize();
	}

	void Options::printUCI()
	{
		for (auto it = begin(); it != end(); ++it) {
			sync_cout << it->second->printUCI() << sync_endl;
		}
	}

	void Options::read(std::vector<std::string> &tokens)
	{
		at(tokens[2])->read(tokens);
	}

	int Options::getInt(std::string key)
	{
		return ((OptionSpin *)at(key))->getValue();
	}

	bool Options::getBool(std::string key)
	{
		return ((OptionCheck *)at(key))->getValue();
	}

	std::string Options::getString(std::string key)
	{
		return find(key) == end() ? "" : ((OptionString *)at(key))->getValue();
	}

	void Options::set(std::string key, std::string value)
	{
		if (find(key) == end()) {
			(*this)[key] = (Option *)(new OptionString(key));
		}
		((OptionString *)at(key))->set(value);
	}

	void Options::set(std::string key, int value)
	{
		((OptionSpin *)at(key))->set(value);
	}

	void Options::set(std::string key, bool value)
	{
		((OptionCheck *)at(key))->set(value);
	}

	void Options::initialize()
	{
		(*this)[OPTION_CHESS960] = (Option *)(new Option960());
		(*this)[OPTION_HASH] = (Option *)(new OptionHash());
		(*this)[OPTION_CLEAR_HASH] = (Option *)(new OptionButton(OPTION_CLEAR_HASH));
		(*this)[OPTION_MULTIPV] = (Option *)(new OptionSpin(OPTION_MULTIPV, 1, 1, 216));
		(*this)[OPTION_THREADS] = (Option *)(new OptionThread());
		(*this)[OPTION_PONDER] = (Option *)(new OptionCheck(OPTION_PONDER, false));
		(*this)[OPTION_CONTEMPT] = (Option *)(new OptionContempt());
		(*this)[OPTION_BOOK_FILE] = (Option *)(new OptionString(OPTION_BOOK_FILE, "book.bin"));
		(*this)[OPTION_OWN_BOOK] = (Option *)(new OptionCheck(OPTION_OWN_BOOK, false));
		(*this)[OPTION_OPPONENT] = (Option *)(new OptionString(OPTION_OPPONENT));
		(*this)[OPTION_EMERGENCY_TIME] = (Option *)(new OptionSpin(OPTION_EMERGENCY_TIME, 100, 0, 60000));
#ifdef TB
		(*this)[OPTION_SYZYGY_PATH] = (Option *)(new OptionString(OPTION_SYZYGY_PATH));
		(*this)[OPTION_SYZYGY_PROBE_DEPTH] = (Option *)(new OptionSpin(OPTION_SYZYGY_PROBE_DEPTH, TBProbeDepth, 0, MAX_DEPTH+1));
#endif 
	}

	OptionCheck::OptionCheck(std::string Name, bool value)
	{
		name = Name;
		otype = OptionType::CHECK;
		defaultValue = utils::bool2String(value);
		_value = value;
		maxValue = "";
		minValue = "";
	}

	void OptionCheck::set(std::string value)
	{
		_value = !value.compare("true");
	}

	void OptionContempt::set(std::string value)
	{
		_value = stoi(value);
		Contempt = Value(_value);
	}

	void OptionContempt::set(int value)
	{
		_value = value;
		Contempt = Value(_value);
	}

	void OptionString::read(std::vector<std::string>& tokens)
	{
		if ((int)tokens.size() > 5) {
			std::stringstream ss;
			ss << tokens[4];
			for (int i = 5; i < (int)tokens.size(); ++i) {
				ss << " " << tokens[i];
			}
			_value = ss.str();
		}
		else if ((int)tokens.size() == 5) _value = tokens[4];
		else _value = "";
	}

	void OptionHash::set(std::string value)
	{
		_value = stoi(value);
		tt::InitializeTranspositionTable();
	}

	void OptionHash::set(int value)
	{
		_value = value;
		tt::InitializeTranspositionTable();
	}
}
