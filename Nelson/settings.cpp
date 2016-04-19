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
