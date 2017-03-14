#include <iostream>
#include <fstream>
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
eval PieceValues[7]{ eval(1025), eval(490, 550), eval(325), eval(325), eval(80, 100), eval(VALUE_KNOWN_WIN), eval(0) };
eval PASSED_PAWN_BONUS[6] = { eval(22), eval(18), eval(23), eval(39), eval(81), eval(101) };
eval BONUS_PROTECTED_PASSED_PAWN[6] = { eval(0), eval(0), eval(0), eval(30), eval(30), eval(30) };
#endif

enum CAPTURES {
	QxP, BxP, NxP, RxP, QxN,
	QxR, RxN, QxB, BxN, RxB,
	KxP, EP, PxP, KxN, PxB,
	BxR, PxN, NxR, NxN, NxB,
	RxR, BxB, PxR, KxB, KxR,
	QxQ,
	RxQ, BxQ, NxQ, PxQ, KxQ //Winning Captures of Queen
};

//enum CAPTURES {
//	QxP, NxP, BxP, RxP, QxN,
//	QxR, RxN, QxB, BxN,
//	KxP, RxB, EP, PxP, KxN, PxB,
//	BxR, PxN, NxR, NxN, NxB,
//	BxB, RxR, PxR, KxB,
//	QxQ, KxR,
//	RxQ, NxQ, BxQ, PxQ, KxQ //Winning Captures of Queen
//};

const Value CAPTURE_SCORES[6][7] = {
	// Captured:  QUEEN, ROOK,   BISHOP,   KNIGHT,    PAWN,   King,   EP-Capture/Promotion
	{ Value(QxQ), Value(QxR),  Value(QxB),  Value(QxN),  Value(QxP),  Value(0),  Value(0) },   // QUEEN
	{ Value(RxQ), Value(RxR), Value(RxB), Value(RxN),  Value(RxP),  Value(0),  Value(0) },   // ROOK
	{ Value(BxQ), Value(BxR), Value(BxB), Value(BxN),  Value(BxP),  Value(0),  Value(0) },   // BISHOP
	{ Value(NxQ), Value(NxR), Value(NxB), Value(NxN), Value(NxP),  Value(0),  Value(0) },   // KNIGHT
	{ Value(PxQ), Value(PxR), Value(PxB), Value(PxN), Value(PxP),  Value(0), Value(EP) },   // PAWN
	{ Value(KxQ), Value(KxR), Value(KxB), Value(KxN), Value(KxP), Value(0),  Value(0) }    // KING
};


namespace settings {

	static int LMR_REDUCTION[64][64];
	const eval PSQT[12][64]{
		{  // White Queens
			eval(-10,-8), eval(-7,-5), eval(-4,-2), eval(-2,0), eval(-2,0), eval(-4,-2), eval(-7,-5), eval(-10,-8),  // Rank 1
			eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 2
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 3
			eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 4
			eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 5
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 6
			eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 7
			eval(-7,-8), eval(-4,-5), eval(-2,-2), eval(0,0), eval(0,0), eval(-2,-2), eval(-4,-5), eval(-7,-8)  // Rank 8
		},
		{  // Black Queen
			eval(7,8), eval(4,5), eval(2,2), eval(0,0), eval(0,0), eval(2,2), eval(4,5), eval(7,8),  // Rank 1
			eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 2
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 3
			eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 4
			eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 5
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 6
			eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 7
			eval(10,8), eval(7,5), eval(4,2), eval(2,0), eval(2,0), eval(4,2), eval(7,5), eval(10,8)  // Rank 8
		},
		{  // White Rook
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 1
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 2
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 3
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 4
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 5
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0),  // Rank 6
			eval(3,0), eval(3,0), eval(4,0), eval(6,0), eval(6,0), eval(4,0), eval(3,0), eval(3,0),  // Rank 7
			eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0)  // Rank 8
		},
		{  // Black Rook
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 1
			eval(-3,0), eval(-3,0), eval(-4,0), eval(-6,0), eval(-6,0), eval(-4,0), eval(-3,0), eval(-3,0),  // Rank 2
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 3
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 4
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 5
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 6
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0),  // Rank 7
			eval(1,0), eval(1,0), eval(0,0), eval(-1,0), eval(-1,0), eval(0,0), eval(1,0), eval(1,0)  // Rank 8
		},
		{  // White Bishop
			eval(-12,-8), eval(-10,-5), eval(-7,-2), eval(-4,0), eval(-4,0), eval(-7,-2), eval(-10,-5), eval(-12,-8),  // Rank 1
			eval(-4,-5), eval(3,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(3,-2), eval(-4,-5),  // Rank 2
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 3
			eval(0,0), eval(3,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(3,2), eval(0,0),  // Rank 4
			eval(0,0), eval(5,2), eval(5,5), eval(8,8), eval(8,8), eval(5,5), eval(5,2), eval(0,0),  // Rank 5
			eval(-2,-2), eval(0,0), eval(3,2), eval(5,5), eval(5,5), eval(3,2), eval(0,0), eval(-2,-2),  // Rank 6
			eval(-4,-5), eval(-2,-2), eval(0,0), eval(3,2), eval(3,2), eval(0,0), eval(-2,-2), eval(-4,-5),  // Rank 7
			eval(-7,-8), eval(-4,-5), eval(-2,-2), eval(0,0), eval(0,0), eval(-2,-2), eval(-4,-5), eval(-7,-8)  // Rank 8
		},
		{  // Black Bishop
			eval(7,8), eval(4,5), eval(2,2), eval(0,0), eval(0,0), eval(2,2), eval(4,5), eval(7,8),  // Rank 1
			eval(4,5), eval(2,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(2,2), eval(4,5),  // Rank 2
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 3
			eval(0,0), eval(-5,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-5,-2), eval(0,0),  // Rank 4
			eval(0,0), eval(-3,-2), eval(-5,-5), eval(-8,-8), eval(-8,-8), eval(-5,-5), eval(-3,-2), eval(0,0),  // Rank 5
			eval(2,2), eval(0,0), eval(-3,-2), eval(-5,-5), eval(-5,-5), eval(-3,-2), eval(0,0), eval(2,2),  // Rank 6
			eval(4,5), eval(-3,2), eval(0,0), eval(-3,-2), eval(-3,-2), eval(0,0), eval(-3,2), eval(4,5),  // Rank 7
			eval(12,8), eval(10,5), eval(7,2), eval(4,0), eval(4,0), eval(7,2), eval(10,5), eval(12,8)  // Rank 8
		},
		{  // White Knight
			eval(-19,-18), eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(-3,-2), eval(-8,-7), eval(-14,-12), eval(-19,-18),  // Rank 1
			eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(4,5), eval(4,5), eval(-3,-2), eval(-8,-7), eval(-14,-12),  // Rank 2
			eval(-8,-7), eval(-3,-2), eval(4,5), eval(15,16), eval(15,16), eval(4,5), eval(-3,-2), eval(-8,-7),  // Rank 3
			eval(-3,-2), eval(4,5), eval(15,16), eval(21,22), eval(21,22), eval(15,16), eval(4,5), eval(-3,-2),  // Rank 4
			eval(-3,-2), eval(4,5), eval(21,16), eval(26,22), eval(26,22), eval(21,16), eval(4,5), eval(-3,-2),  // Rank 5
			eval(-8,-7), eval(-3,-2), eval(10,5), eval(31,16), eval(31,16), eval(10,5), eval(-3,-2), eval(-8,-7),  // Rank 6
			eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(4,5), eval(4,5), eval(-3,-2), eval(-8,-7), eval(-14,-12),  // Rank 7
			eval(-19,-18), eval(-14,-12), eval(-8,-7), eval(-3,-2), eval(-3,-2), eval(-8,-7), eval(-14,-12), eval(-19,-18)  // Rank 8
		},
		{  // Black Knight
			eval(19,18), eval(14,12), eval(8,7), eval(3,2), eval(3,2), eval(8,7), eval(14,12), eval(19,18),  // Rank 1
			eval(14,12), eval(8,7), eval(3,2), eval(-4,-5), eval(-4,-5), eval(3,2), eval(8,7), eval(14,12),  // Rank 2
			eval(8,7), eval(3,2), eval(-10,-5), eval(-31,-16), eval(-31,-16), eval(-10,-5), eval(3,2), eval(8,7),  // Rank 3
			eval(3,2), eval(-4,-5), eval(-21,-16), eval(-26,-22), eval(-26,-22), eval(-21,-16), eval(-4,-5), eval(3,2),  // Rank 4
			eval(3,2), eval(-4,-5), eval(-15,-16), eval(-21,-22), eval(-21,-22), eval(-15,-16), eval(-4,-5), eval(3,2),  // Rank 5
			eval(8,7), eval(3,2), eval(-4,-5), eval(-15,-16), eval(-15,-16), eval(-4,-5), eval(3,2), eval(8,7),  // Rank 6
			eval(14,12), eval(8,7), eval(3,2), eval(-4,-5), eval(-4,-5), eval(3,2), eval(8,7), eval(14,12),  // Rank 7
			eval(19,18), eval(14,12), eval(8,7), eval(3,2), eval(3,2), eval(8,7), eval(14,12), eval(19,18)  // Rank 8
		},
		{  // White Pawn
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0),  // Rank 1
			eval(-17,-11), eval(-9,-11), eval(-7,-11), eval(-9,-11), eval(-9,-11), eval(-7,-11), eval(-9,-11), eval(-17,-11),  // Rank 2
			eval(-15,-9), eval(-12,-9), eval(-7,-9), eval(-1,-9), eval(-1,-9), eval(-7,-9), eval(-12,-9), eval(-15,-9),  // Rank 3
			eval(-17,-6), eval(-9,-6), eval(-7,-6), eval(6,-6), eval(6,-6), eval(-7,-6), eval(-9,-6), eval(-17,-6),  // Rank 4
			eval(-9,-3), eval(-7,-3), eval(-4,-3), eval(9,-3), eval(9,-3), eval(-4,-3), eval(-7,-3), eval(-9,-3),  // Rank 5
			eval(3,9), eval(6,9), eval(9,9), eval(11,9), eval(11,9), eval(9,9), eval(6,9), eval(3,9),  // Rank 6
			eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20), eval(22,20),  // Rank 7
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0)  // Rank 8
		},
		{  //  Black Pawn
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0),  // Rank 1
			eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20), eval(-22,-20),  // Rank 2
			eval(-3,-9), eval(-6,-9), eval(-9,-9), eval(-11,-9), eval(-11,-9), eval(-9,-9), eval(-6,-9), eval(-3,-9),  // Rank 3
			eval(9,3), eval(7,3), eval(4,3), eval(-9,3), eval(-9,3), eval(4,3), eval(7,3), eval(9,3),  // Rank 4
			eval(17,6), eval(9,6), eval(7,6), eval(-6,6), eval(-6,6), eval(7,6), eval(9,6), eval(17,6),  // Rank 5
			eval(15,9), eval(12,9), eval(7,9), eval(1,9), eval(1,9), eval(7,9), eval(12,9), eval(15,9),  // Rank 6
			eval(17,11), eval(9,11), eval(7,11), eval(9,11), eval(9,11), eval(7,11), eval(9,11), eval(17,11),  // Rank 7
			eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0), eval(0,0)  // Rank 8
		},
		{  // White King
			eval(16,-20), eval(17,-15), eval(16,-9), eval(14,-4), eval(14,-4), eval(16,-9), eval(17,-15), eval(16,-20),  // Rank 1
			eval(16,-15), eval(16,-9), eval(14,-4), eval(11,7), eval(11,7), eval(14,-4), eval(16,-9), eval(16,-15),  // Rank 2
			eval(11,-9), eval(11,-4), eval(11,7), eval(8,23), eval(8,23), eval(11,7), eval(11,-4), eval(11,-9),  // Rank 3
			eval(8,-4), eval(8,7), eval(3,23), eval(-2,28), eval(-2,28), eval(3,23), eval(8,7), eval(8,-4),  // Rank 4
			eval(3,-4), eval(0,7), eval(-2,23), eval(-7,28), eval(-7,28), eval(-2,23), eval(0,7), eval(3,-4),  // Rank 5
			eval(-7,-9), eval(-7,-4), eval(-12,7), eval(-18,23), eval(-18,23), eval(-12,7), eval(-7,-4), eval(-7,-9),  // Rank 6
			eval(-12,-15), eval(-12,-9), eval(-18,-4), eval(-18,7), eval(-18,7), eval(-18,-4), eval(-12,-9), eval(-12,-15),  // Rank 7
			eval(-18,-20), eval(-18,-15), eval(-18,-9), eval(-18,-4), eval(-18,-4), eval(-18,-9), eval(-18,-15), eval(-18,-20)  // Rank 8
		},
		{  // Black King
			eval(18,20), eval(18,15), eval(18,9), eval(18,4), eval(18,4), eval(18,9), eval(18,15), eval(18,20),  // Rank 1
			eval(12,15), eval(12,9), eval(18,4), eval(18,-7), eval(18,-7), eval(18,4), eval(12,9), eval(12,15),  // Rank 2
			eval(7,9), eval(7,4), eval(12,-7), eval(18,-23), eval(18,-23), eval(12,-7), eval(7,4), eval(7,9),  // Rank 3
			eval(-3,4), eval(0,-7), eval(2,-23), eval(7,-28), eval(7,-28), eval(2,-23), eval(0,-7), eval(-3,4),  // Rank 4
			eval(-8,4), eval(-8,-7), eval(-3,-23), eval(2,-28), eval(2,-28), eval(-3,-23), eval(-8,-7), eval(-8,4),  // Rank 5
			eval(-11,9), eval(-11,4), eval(-11,-7), eval(-8,-23), eval(-8,-23), eval(-11,-7), eval(-11,4), eval(-11,9),  // Rank 6
			eval(-16,15), eval(-16,9), eval(-14,4), eval(-11,-7), eval(-11,-7), eval(-14,4), eval(-16,9), eval(-16,15),  // Rank 7
			eval(-16,20), eval(-17,15), eval(-16,9), eval(-14,4), eval(-14,4), eval(-16,9), eval(-17,15), eval(-16,20)  // Rank 8
		}
	};

	void Initialize() {
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
			//if (!key.compare("QMG")) PieceValues[QUEEN].mgScore = Value(val);
			//else if (!key.compare("QEG")) PieceValues[QUEEN].egScore = Value(val);
			//else if (!key.compare("RMG")) PieceValues[ROOK].mgScore = Value(val);
			//else if (!key.compare("REG")) PieceValues[ROOK].egScore = Value(val);
			//else if (!key.compare("BMG")) PieceValues[BISHOP].mgScore = Value(val);
			//else if (!key.compare("BEG")) PieceValues[BISHOP].egScore = Value(val);
			//else if (!key.compare("NMG")) PieceValues[KNIGHT].mgScore = Value(val);
			//else if (!key.compare("NEG")) PieceValues[KNIGHT].egScore = Value(val);
			//else if (!key.compare("PMG")) PieceValues[PAWN].mgScore = Value(val);
			//else if (!key.compare("PEG")) PieceValues[PAWN].mgScore = Value(val);
			//else if (!key.compare(0, 3, "PPB")) {
			//	int indx = stoi(key.substr(3));
			//	PASSED_PAWN_BONUS[indx] = eval(val);
			//}
			//else if (!key.compare(0, 4, "BPPP")) {
			//	int indx = stoi(key.substr(4));
			//	BONUS_PROTECTED_PASSED_PAWN[indx] = eval(val);
			//}
		}
		sync_cout << "info string parameters set" << sync_endl;
		//for (int i = 0; i < 7; ++i) sync_cout << "info string " << PieceValues[i].mgScore << " " << PieceValues[i].egScore << sync_endl;
	}
#endif

	Options options;

	Option::Option(std::string Name, OptionType Type, std::string DefaultValue, std::string MinValue, std::string MaxValue, bool Technical)
	{
		name = Name;
		otype = Type;
		defaultValue = DefaultValue;
		maxValue = MaxValue;
		minValue = MinValue;
		technical = Technical;
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

	std::string Option::printInfo() const
	{
		std::stringstream ss;
		ss << "info string " << name << " ";
		switch (otype)
		{
		case OptionType::CHECK:
			ss << ((OptionCheck *)this)->getValue();
			return ss.str();
		case OptionType::STRING:
			ss << ((OptionString *)this)->getValue();
			return ss.str();
		case OptionType::SPIN:
			ss << ((OptionSpin *)this)->getValue();
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
			if (!it->second->isTechnical())
				sync_cout << it->second->printUCI() << sync_endl;
		}
	}

	void Options::printInfo()
	{
		for (auto it = begin(); it != end(); ++it) {
				sync_cout << it->second->printInfo() << sync_endl;
		}
	}

	void Options::read(std::vector<std::string> &tokens)
	{
		if (find(tokens[2]) == end()) return;
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
		(*this)[OPTION_PRINT_OPTIONS] = (Option *)(new OptionButton(OPTION_PRINT_OPTIONS, true));
		(*this)[OPTION_MULTIPV] = (Option *)(new OptionSpin(OPTION_MULTIPV, 1, 1, 216));
		(*this)[OPTION_THREADS] = (Option *)(new OptionThread());
		(*this)[OPTION_PONDER] = (Option *)(new OptionCheck(OPTION_PONDER, false));
		(*this)[OPTION_CONTEMPT] = (Option *)(new OptionContempt());
		(*this)[OPTION_BOOK_FILE] = (Option *)(new OptionString(OPTION_BOOK_FILE, "book.bin"));
		(*this)[OPTION_OWN_BOOK] = (Option *)(new OptionCheck(OPTION_OWN_BOOK, false));
		(*this)[OPTION_OPPONENT] = (Option *)(new OptionString(OPTION_OPPONENT));
		(*this)[OPTION_EMERGENCY_TIME] = (Option *)(new OptionSpin(OPTION_EMERGENCY_TIME, 100, 0, 60000));
		(*this)[OPTION_NODES_TIME] = (Option *)(new OptionSpin(OPTION_NODES_TIME, 0, 0, INT_MAX, true));
		(*this)[OPTION_TEXEL_TUNING_WINS] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_WINS, "", true));
		(*this)[OPTION_TEXEL_TUNING_DRAWS] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_DRAWS, "", true));
		(*this)[OPTION_TEXEL_TUNING_LOSSES] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_LOSSES, "", true));
		(*this)[OPTION_TEXEL_TUNING_LABELLED] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_LABELLED, "", true));
#ifdef TUNE
		(*this)[OPTION_PIECE_VALUES_QUEEN_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_QUEEN_MG, PieceValues[QUEEN].mgScore, 0, 2 * PieceValues[QUEEN].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_QUEEN_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_QUEEN_EG, PieceValues[QUEEN].egScore, 0, 2 * PieceValues[QUEEN].egScore, false));
		(*this)[OPTION_PIECE_VALUES_ROOK_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_ROOK_MG, PieceValues[ROOK].mgScore, 0, 2 * PieceValues[ROOK].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_ROOK_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_ROOK_EG, PieceValues[ROOK].egScore, 0, 2 * PieceValues[ROOK].egScore, false));
		(*this)[OPTION_PIECE_VALUES_BISHOP_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_BISHOP_MG, PieceValues[BISHOP].mgScore, 0, 2 * PieceValues[BISHOP].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_BISHOP_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_BISHOP_EG, PieceValues[BISHOP].egScore, 0, 2 * PieceValues[BISHOP].egScore, false));
		(*this)[OPTION_PIECE_VALUES_KNIGHT_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_KNIGHT_MG, PieceValues[KNIGHT].mgScore, 0, 2 * PieceValues[KNIGHT].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_KNIGHT_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_KNIGHT_EG, PieceValues[KNIGHT].egScore, 0, 2 * PieceValues[KNIGHT].egScore, false));
		(*this)[OPTION_PIECE_VALUES_PAWN_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_PAWN_MG, PieceValues[PAWN].mgScore, 0, 2 * PieceValues[PAWN].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_PAWN_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_PAWN_EG, PieceValues[PAWN].egScore, 0, 2 * PieceValues[PAWN].egScore, false));
#endif
#ifdef TB
		(*this)[OPTION_SYZYGY_PATH] = (Option *)(new OptionString(OPTION_SYZYGY_PATH));
		(*this)[OPTION_SYZYGY_PROBE_DEPTH] = (Option *)(new OptionSpin(OPTION_SYZYGY_PROBE_DEPTH, TBProbeDepth, 0, MAX_DEPTH + 1));
#endif 
	}

	OptionCheck::OptionCheck(std::string Name, bool value, bool Technical)
	{
		name = Name;
		otype = OptionType::CHECK;
		defaultValue = utils::bool2String(value);
		_value = value;
		maxValue = "";
		minValue = "";
		technical = Technical;
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
