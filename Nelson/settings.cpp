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

//Value PASSED_PAWN_BONUS[4] = { Value(10), Value(30), Value(60), Value(100) };
//Value BETA_PRUNING_FACTOR = Value(200);


namespace settings {

	static int LMR_REDUCTION[64][64];
	static const double LMR_BASE = 0.3;
	static const double LMR_NON_PV = 1.5;
	static const double LMR_PV = 2.25;
	eval PSQT[12][64];

	//PSQT Values are copied 1:1 from a post on talkchess - these are
	//the values from Hakkapeliitta
	//There might be a lot of room for improvement :-)
	const int pstPawnMg[64] = {
		0,  0,  0,  0,  0,  0,  0,  0,       
		-33,-18,-13,-18,-18,-13,-18,-33,
		-28,-23,-13, -3, -3,-13,-23,-28,
		-33,-18,-13, 12, 12,-13,-18,-33,
		-18,-13, -8, 17, 17, -8,-13,-18,
		7, 12, 17, 22, 22, 17, 12,  7,
		42, 42, 42, 42, 42, 42, 42, 42,
		0,  0,  0,  0,  0,  0,  0,  0     
	};

	const int pstPawnEg[64] = {
		0,  0,  0,  0,  0,  0,  0,  0,
		-22,-22,-22,-22,-22,-22,-22,-22,
		-17,-17,-17,-17,-17,-17,-17,-17,
		-12,-12,-12,-12,-12,-12,-12,-12,
		-7, -7, -7, -7, -7, -7, -7, -7,
		18, 18, 18, 18, 18, 18, 18, 18,
		38, 38, 38, 38, 38, 38, 38, 38,
		0,  0,  0,  0,  0,  0,  0,  0
	};

	const int pstKnightMg[64] = {
		-36,-26,-16, -6, -6,-16,-26,-36,
		-26,-16, -6,  9,  9, -6,-16,-26,
		-16, -6,  9, 29, 29,  9, -6,-16,
		-6,  9, 29, 39, 39, 29,  9, -6,
		-6,  9, 39, 49, 49, 39,  9, -6,
		-16, -6, 19, 59, 59, 19, -6,-16,
		-26,-16, -6,  9,  9, -6,-16,-26,
		-36,-26,-16, -6, -6,-16,-26,-36
	};

	const int pstKnightEg[64] = {
		-34,-24,-14, -4, -4,-14,-24,-34,
		-24,-14, -4, 11, 11, -4,-14,-24,
		-14, -4, 11, 31, 31, 11, -4,-14,
		-4, 11, 31, 41, 41, 31, 11, -4,
		-4, 11, 31, 41, 41, 31, 11, -4,
		-14, -4, 11, 31, 31, 11, -4,-14,
		-24,-14, -4, 11, 11, -4,-14,-24,
		-34,-24,-14, -4, -4,-14,-24,-34
	};

	const int pstBishopMg[64] = {
		-24,-19,-14, -9, -9,-14,-19,-24,
		-9,  6,  1,  6,  6,  1,  6, -9,
		-4,  1,  6, 11, 11,  6,  1, -4,
		1,  6, 11, 16, 16, 11,  6,  1,
		1, 11, 11, 16, 16, 11, 11,  1,
		-4,  1,  6, 11, 11,  6,  1, -4,
		-9, -4,  1,  6,  6,  1, -4, -9,
		-14, -9, -4,  1,  1, -5, -9,-14
	};

	const int pstBishopEg[64] = {
		-15,-10, -5,  0,  0, -5,-10,-15,
		-10, -5,  0,  5,  5,  0, -5,-10,
		-5,  0,  5, 10, 10,  5,  0, -5,
		0,  5, 10, 15, 15, 10,  5,  0,
		0,  5, 10, 15, 15, 10,  5,  0,
		-5,  0,  5, 10, 10,  5,  0, -5,
		-10, -5,  0,  5,  5,  0, -5,-10,
		-15,-10, -5,  0,  0, -5,-10,-15
	};

	const int pstRookMg[64] = {
		-3, -3, -1,  2,  2, -1, -3, -3,
		-3, -3, -1,  2,  2, -1, -3, -3,
		-3, -3, -1,  2,  2, -1, -3, -3,
		-3, -3, -1,  2,  2, -1, -3, -3,
		-3, -3, -1,  2,  2, -1, -3, -3,
		-3, -3, -1,  2,  2, -1, -3, -3,
		7,  7,  9, 12, 12,  9,  7,  7,
		-3, -3, -1,  2,  2, -1, -3, -3,
	};

	const int pstRookEg[64] = {
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
	};

	const int pstQueenMg[64] = {
		-19,-14, -9, -4, -4, -9,-14,-19,
		-9, -4,  1,  6,  6,  1, -4, -9,
		-4,  1,  6, 11, 11,  6,  1, -4,
		1,  6, 11, 16, 16, 11,  6,  1,
		1,  6, 11, 16, 16, 11,  6,  1,
		-4,  1,  6, 11, 11,  6,  1, -4,
		-9, -4,  1,  6,  6,  1, -4, -9,
		-14, -9, -4,  1,  1, -4, -9,-14
	};

	const int pstQueenEg[64] = {
		-15,-10, -5,  0,  0, -5,-10,-15,
		-10, -5,  0,  5,  5,  0, -5,-10,
		-5,  0,  5, 10, 10,  5,  0, -5,
		0,  5, 10, 15, 15, 10,  5,  0,
		0,  5, 10, 15, 15, 10,  5,  0,
		-5,  0,  5, 10, 10,  5,  0, -5,
		-10, -5,  0,  5,  5,  0, -5,-10,
		-15,-10, -5,  0,  0, -5,-10,-15
	};

	const int pstKingMg[64] = {
		5,  6,  4,  0,  0,  1,  6,  4,
		5,  5,  0, -5, -5,  0,  5,  5,
		-5, -5, -5,-10,-10, -5, -5, -5,
		-10,-10,-20,-30,-30,-20,-10,-10,
		-20,-25,-30,-40,-40,-30,-25,-20,
		-40,-40,-50,-60,-60,-50,-40,-40,
		-50,-50,-60,-60,-60,-60,-50,-50,
		-60,-60,-60,-60,-60,-60,-60,-60
	};

	const int pstKingEg[64] = {
		-38,-28,-18, -8, -8,-18,-28,-38,
		-28,-18, -8, 13, 13, -8,-18,-28,
		-18, -8, 13, 43, 43, 13, -8,-18,
		-8, 13, 43, 53, 53, 43, 13, -8,
		-8, 13, 43, 53, 53, 43, 13, -8,
		-18, -8, 13, 43, 43, 13, -8,-18,
		-28,-18, -8, 13, 13, -8,-18,-28,
		-38,-28,-18, -8, -8,-18,-28,-38,
	};

	void initPSQT()
	{
		for (Piece p = WQUEEN; p <= BKING; ++p) {
			for (Square s = A1; s <= H8; ++s) {
				PSQT[p][s] = EVAL_ZERO;
			}
		}
		//Take over values from const arrays
		for (int i = 0; i < 64; ++i) {
			PSQT[WPAWN][i] = eval(pstPawnMg[i], pstPawnEg[i]);
			PSQT[WKING][i] = eval(pstKingMg[i], pstKingEg[i]);
			PSQT[WQUEEN][i] = eval(pstQueenMg[i], pstQueenEg[i]);
			PSQT[WROOK][i] = eval(pstRookMg[i], pstRookEg[i]);
			PSQT[WBISHOP][i] = eval(pstBishopMg[i], pstBishopEg[i]);
			PSQT[WKNIGHT][i] = eval(pstKnightMg[i], pstKnightEg[i]);
		}
		for (PieceType p = QUEEN; p <= KING; ++p) {
			//Normalize (to ensure that PSQT doesn't add piece value
			eval offset = EVAL_ZERO;
			for (int rank = 0; rank < 8; ++rank) {
				for (int file = 0; file < 8; ++file) {
					offset += PSQT[2*p][8 * rank + file];
				}
			}
			offset.mgScore = offset.mgScore / 64;
			offset.egScore = offset.egScore / 64;
			for (int rank = 0; rank < 8; ++rank) {
				for (int file = 0; file < 8; ++file) {
					PSQT[2 * p][8 * rank + file] -= offset;
				}
			}
			//Add Values for Black Pieces
			for (int rank = 0; rank < 8; ++rank) {
				for (int file = 0; file < 8; ++file) {
					PSQT[2 * p + 1][8 * (7 - rank) + file] = -PSQT[2 * p][8 * rank + file];
				}
			}
		}
		for (Piece p = WQUEEN; p <= BKING; ++p) {
			for (Square s = A1; s <= H8; ++s) {
				PSQT[p][s] = PSQT[p][s] * 2 / 3;
			}
		}
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
		return ((OptionString *)at(key))->getValue();
	}

	void Options::set(std::string key, std::string value)
	{
		if (find(key) == end()) {
			(*this)[key] = (Option *)(new OptionString(key));
		}
		at(key)->set(value);
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
#ifdef TB
		(*this)[OPTION_SYZYGY_PATH] = (Option *)(new OptionString(OPTION_SYZYGY_PATH));
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
		if (tokens.size() > 5) {
			std::stringstream ss;
			ss << tokens[4];
			for (int i = 5; i < tokens.size(); ++i) {
				ss << " " << tokens[i];
			}
			_value = ss.str();
		}
		else if (tokens.size() == 5) _value = tokens[4];
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
