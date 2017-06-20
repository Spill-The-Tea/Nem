#include <iostream>
#include <fstream>
#include <algorithm>
#include <regex>
#include "settings.h"
#include "utils.h"
#include "hashtables.h"

namespace settings {

	Parameters parameter;

	void Parameters::Initialize() {
		for (int depth = 0; depth < 64; depth++) {
			for (int moves = 0; moves < 64; moves++) {
				double reduction = std::log(moves) * std::log(depth) / 2; //F
				if (reduction < 0.8) LMR_REDUCTION[depth][moves] = 0;
				else LMR_REDUCTION[depth][moves] = int(std::round(reduction));
			}
		}
		for (int i = 0; i < 100; ++i)
			KING_SAFETY[i] = Value(std::min(KING_SAFETY_MAXVAL, int(i*i / (KING_SAFETY_MAXINDEX * KING_SAFETY_MAXINDEX / KING_SAFETY_MAXVAL) + KING_SAFETY_LINEAR * i)));
	}

	void Initialize() {
		parameter.Initialize();
	}

	int Parameters::LMRReduction(int depth, int moveNumber)
	{
		return LMR_REDUCTION[std::min(depth, 63)][std::min(moveNumber, 63)];
	}

#ifdef TUNE
	bool Parameters::parse(std::string input)
	{
		static std::regex rgxKV("(\\w+)(\\[\\d+\\])?\\s*=\\s*(\\S.*)");
		static std::regex rgxIndx("\\[(\\d+)\\]");
		//static std::regex rgx("(-?\\d+)[\\s,]*");
		std::smatch m;
		std::string key;
		int index = -1;
		std::string value;
		std::vector<int> indices;
		if (std::regex_search(input, m, rgxKV)) {
			key = m[1].str();
			if (m[2].length() > 0) {
				std::string vi = m[2].str();
				std::smatch mi;
				while (std::regex_search(vi, mi, rgxIndx)) {
					int ind = std::stoi(m[1].str());
					indices.push_back(ind);
					vi = mi.suffix().str();
				}
				index = indices[0];
			}
			value = m[3].str();
			std::vector<int> values = parseValue(value);
			if (!key.compare("SCALE_BISHOP_PAIR_WITH_PAWNS"))
				setEval(parameter.SCALE_BISHOP_PAIR_WITH_PAWNS, values, index);
			else if (!key.compare("BONUS_BISHOP_PAIR_NO_OPP_MINOR"))
				setEval(parameter.BONUS_BISHOP_PAIR_NO_OPP_MINOR, values, index);
			else if (!key.compare("SCALE_EXCHANGE_WITH_PAWNS"))
				setEval(parameter.SCALE_EXCHANGE_WITH_PAWNS, values, index);
			else if (!key.compare("SCALE_BISHOP_PAIR_WITH_PAWNS"))
				setEval(parameter.SCALE_EXCHANGE_WITH_MAJORS, values, index);
			else if (!key.compare("SCALE_EXCHANGE_WITH_MAJORS"))
				setEval(parameter.HANGING, values, index);
			else if (!key.compare("HANGING"))
				setEval(parameter.KING_ON_ONE, values, index);
			else if (!key.compare("KING_ON_ONE"))
				setEval(parameter.KING_ON_MANY, values, index);
			else if (!key.compare("KING_ON_MANY"))
				setEval(parameter.ROOK_ON_OPENFILE, values, index);
			else if (!key.compare("ROOK_ON_OPENFILE"))
				setEval(parameter.SCALE_BISHOP_PAIR_WITH_PAWNS, values, index);
			else if (!key.compare("ROOK_ON_SEMIOPENFILE"))
				setEval(parameter.ROOK_ON_SEMIOPENFILE, values, index);
			else if (!key.compare("ROOK_ON_SEMIOPENFILE_WITH_KQ"))
				setEval(parameter.ROOK_ON_SEMIOPENFILE_WITH_KQ, values, index);
			else if (!key.compare("BONUS_BISHOP_PAIR"))
				setEval(parameter.BONUS_BISHOP_PAIR, values, index);
			else if (!key.compare("PAWN_SHELTER_2ND_RANK"))
				setEval(parameter.PAWN_SHELTER_2ND_RANK, values, index);
			else if (!key.compare("PAWN_SHELTER_3RD_RANK"))
				setEval(parameter.PAWN_SHELTER_3RD_RANK, values, index);
			else if (!key.compare("PAWN_SHELTER_4TH_RANK"))
				setEval(parameter.PAWN_SHELTER_4TH_RANK, values, index);
			else if (!key.compare("MALUS_ISOLATED_PAWN"))
				setEval(parameter.MALUS_ISOLATED_PAWN, values, index);
			else if (!key.compare("MALUS_BACKWARD_PAWN"))
				setEval(parameter.MALUS_BACKWARD_PAWN, values, index);
			else if (!key.compare("MALUS_ISOLATED_PAWN"))
				setEval(parameter.MALUS_ISOLATED_PAWN, values, index);
			else if (!key.compare("MALUS_ISLAND_COUNT"))
				setEval(parameter.MALUS_ISLAND_COUNT, values, index);
			else if (!key.compare("BONUS_CANDIDATE"))
				setEval(parameter.BONUS_CANDIDATE, values, index);
			else if (!key.compare("BONUS_LEVER"))
				setEval(parameter.BONUS_LEVER, values, index);
			else if (!key.compare("MALUS_DOUBLED_PAWN"))
				setEval(parameter.MALUS_DOUBLED_PAWN, values, index);
			else if (!key.compare("BETA_PRUNING_FACTOR"))
				setValue(parameter.BETA_PRUNING_FACTOR, values);
			else if (!key.compare("RAZORING_FACTOR"))
				setValue(parameter.RAZORING_FACTOR, values);
			else if (!key.compare("RAZORING_OFFSET"))
				setValue(parameter.RAZORING_OFFSET, values);
			else if (!key.compare("BONUS_CASTLING"))
				setValue(parameter.BONUS_CASTLING, values);
			else if (!key.compare("BONUS_TEMPO"))
				setValue(parameter.BONUS_TEMPO, values);
			else if (!key.compare("DELTA_PRUNING_SAFETY_MARGIN"))
				setValue(parameter.DELTA_PRUNING_SAFETY_MARGIN, values);
			else if (!key.compare("PROBCUT_MARGIN"))
				setValue(parameter.PROBCUT_MARGIN, values);
			else if (!key.compare("BONUS_KNIGHT_OUTPOST"))
				setValue(parameter.BONUS_KNIGHT_OUTPOST, values);
			else if (!key.compare("BONUS_BISHOP_OUTPOST"))
				setValue(parameter.BONUS_BISHOP_OUTPOST, values);
			else if (!key.compare("KING_SAFETY_MAXVAL"))
				setValue(parameter.KING_SAFETY_MAXVAL, values);
			else if (!key.compare("KING_SAFETY_MAXINDEX"))
				setValue(parameter.KING_SAFETY_MAXINDEX, values);
			else if (!key.compare("ATTACK_UNITS_SAFE_CONTACT_CHECK"))
				setValue(parameter.ATTACK_UNITS_SAFE_CONTACT_CHECK, values);
			else if (!key.compare("LIMIT_QSEARCH"))
				setValue(parameter.LIMIT_QSEARCH, values);
			else if (!key.compare("LIMIT_QSEARCH_TT"))
				setValue(parameter.LIMIT_QSEARCH_TT, values);
			else if (!key.compare("FULTILITY_PRUNING_DEPTH "))
				setValue(parameter.FULTILITY_PRUNING_DEPTH, values);
		}
		else return false;
		return true;
	}

	std::vector<int> Parameters::parseValue(std::string input)
	{
		std::vector<int> result;
		static std::regex rgx("(-?\\d+)[\\s,]*");
		std::smatch m;
		int index = -1;
		while (std::regex_search(input, m, rgx)) {
			int value = m[1].length() > 0 ? std::stoi(m[1].str()) : 0;
			result.push_back(value);
			input = m.suffix().str();
		}
	}

	void Parameters::setEval(Eval & e, std::vector<int>& v, int index)
	{
		if (index = -1 && v.size() == 1) e.mgScore = e.egScore = Value(v[0]);
		else if (index = -1 && v.size() == 2) {
			e.mgScore = Value(v[0]);
			e.egScore = Value(v[1]);
		}
		else if (index == 0)  e.mgScore = Value(v[0]);
		else if (index == 1) e.egScore = Value(v[0]);
	}

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
			//	PASSED_PAWN_BONUS[indx] = Eval(val);
			//}
			//else if (!key.compare(0, 4, "BPPP")) {
			//	int indx = stoi(key.substr(4));
			//	BONUS_PROTECTED_PASSED_PAWN[indx] = Eval(val);
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
		parameter.HelperThreads = _value - 1;
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
#ifdef TUNE
		(*this)[OPTION_TEXEL_TUNING_WINS] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_WINS, "", true));
		(*this)[OPTION_TEXEL_TUNING_DRAWS] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_DRAWS, "", true));
		(*this)[OPTION_TEXEL_TUNING_LOSSES] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_LOSSES, "", true));
		(*this)[OPTION_TEXEL_TUNING_LABELLED] = (Option *)(new OptionString(OPTION_TEXEL_TUNING_LABELLED, "", true));
		(*this)[OPTION_PIECE_VALUES_QUEEN_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_QUEEN_MG, settings::parameter.PieceValues[QUEEN].mgScore, 0, 2 * settings::parameter.PieceValues[QUEEN].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_QUEEN_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_QUEEN_EG, settings::parameter.PieceValues[QUEEN].egScore, 0, 2 * settings::parameter.PieceValues[QUEEN].egScore, false));
		(*this)[OPTION_PIECE_VALUES_ROOK_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_ROOK_MG, settings::parameter.PieceValues[ROOK].mgScore, 0, 2 * settings::parameter.PieceValues[ROOK].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_ROOK_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_ROOK_EG, settings::parameter.PieceValues[ROOK].egScore, 0, 2 * settings::parameter.PieceValues[ROOK].egScore, false));
		(*this)[OPTION_PIECE_VALUES_BISHOP_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_BISHOP_MG, settings::parameter.PieceValues[BISHOP].mgScore, 0, 2 * settings::parameter.PieceValues[BISHOP].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_BISHOP_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_BISHOP_EG, settings::parameter.PieceValues[BISHOP].egScore, 0, 2 * settings::parameter.PieceValues[BISHOP].egScore, false));
		(*this)[OPTION_PIECE_VALUES_KNIGHT_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_KNIGHT_MG, settings::parameter.PieceValues[KNIGHT].mgScore, 0, 2 * settings::parameter.PieceValues[KNIGHT].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_KNIGHT_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_KNIGHT_EG, settings::parameter.PieceValues[KNIGHT].egScore, 0, 2 * settings::parameter.PieceValues[KNIGHT].egScore, false));
		(*this)[OPTION_PIECE_VALUES_PAWN_MG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_PAWN_MG, settings::parameter.PieceValues[PAWN].mgScore, 0, 2 * settings::parameter.PieceValues[PAWN].mgScore, false));
		(*this)[OPTION_PIECE_VALUES_PAWN_EG] = (Option *)(new OptionSpin(OPTION_PIECE_VALUES_PAWN_EG, settings::parameter.PieceValues[PAWN].egScore, 0, 2 * settings::parameter.PieceValues[PAWN].egScore, false));
#endif
		(*this)[OPTION_SYZYGY_PATH] = (Option *)(new OptionString(OPTION_SYZYGY_PATH));
		(*this)[OPTION_SYZYGY_PROBE_DEPTH] = (Option *)(new OptionSpin(OPTION_SYZYGY_PROBE_DEPTH, parameter.TBProbeDepth, 0, MAX_DEPTH + 1));
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
		parameter.Contempt = Value(_value);
	}

	void OptionContempt::set(int value)
	{
		_value = value;
		parameter.Contempt = Value(_value);
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
