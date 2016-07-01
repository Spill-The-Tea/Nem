#pragma once

#include <map>
#include <memory>
#include "types.h"
#include "board.h"
#include "utils.h"

extern int HelperThreads;     //Number of slave threads
extern Value Contempt;        //Only accept draws, when evaluation is less then -Contempt
extern Color EngineSide;      //Side played by the engine
extern Protocol protocol;

const int EmergencyTime = 100; //100 ms Emergency time
const int PV_MAX_LENGTH = 32; //Maximum Length of displayed Principal Variation
const int MASK_TIME_CHECK = (1 << 14) - 1; //Time is only checked each MASK_TIME_CHECK nodes

//Material Values
//King piece value is set to a very large number, to ensure that any capture sequence where the king is "captured" is << 0
const eval PieceValues[]{ eval(950), eval(490, 550), eval(325), eval(325), eval(80, 100), eval(VALUE_KNOWN_WIN), eval(0) };

const int PAWN_TABLE_SIZE = 1 << 14; //has to be power of 2

//Mobility bonus values
#define MBV(MGV, EGV) eval(9*MGV/8, 9*EGV/8)

const eval MOBILITY_BONUS_KNIGHT[] = { MBV(-21, -17), MBV(-14, -10), MBV(-3, -3), MBV(1, 0), MBV(5, 3), MBV(9, 7), MBV(12, 9), MBV(14, 10), MBV(15, 11) };
const eval MOBILITY_BONUS_BISHOP[] = { MBV(-17, -16), MBV(-9, -8), MBV(2, 0), MBV(7, 5), MBV(11, 10), MBV(16, 14), MBV(20, 18), MBV(23, 21), MBV(24, 23),
MBV(26, 24), MBV(27, 25), MBV(27, 26), MBV(28, 26), MBV(29, 27) };
const eval MOBILITY_BONUS_ROOK[] = { MBV(-16, -18), MBV(-10, -9), MBV(-2, 0), MBV(0, 5), MBV(2, 11), MBV(4, 16), MBV(6, 21), MBV(7, 27), MBV(9, 32),
MBV(10, 36), MBV(10, 38), MBV(11, 40), MBV(12, 41), MBV(12, 41), MBV(12, 42) };
const eval MOBILITY_BONUS_QUEEN[] = { MBV(-14, -13), MBV(-9, -8), MBV(-2, -2), MBV(0, 0), MBV(2, 3), MBV(4, 6), MBV(4, 10), MBV(6, 13), MBV(7, 13), MBV(7, 14),
MBV(7, 14), MBV(7, 14), MBV(7, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14),
MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14), MBV(8, 14) };

#undef MBV

//King safety parameters
const int KING_SAFETY_MAXVAL = 500;
const int KING_SAFETY_MAXINDEX = 61;
const double KING_SAFETY_LINEAR = 0.5;

#define KSV(I) Value(std::min(KING_SAFETY_MAXVAL, int(I*I / (KING_SAFETY_MAXINDEX * KING_SAFETY_MAXINDEX / KING_SAFETY_MAXVAL) + KING_SAFETY_LINEAR * I)))

const Value KING_SAFETY[100] = {
	Value(0), KSV(1), KSV(2), KSV(3), KSV(4), KSV(5), KSV(6), KSV(7), KSV(8), KSV(9),
	KSV(10), KSV(11), KSV(12), KSV(13), KSV(14), KSV(15), KSV(16), KSV(17), KSV(18), KSV(19),
	KSV(20), KSV(21), KSV(22), KSV(23), KSV(24), KSV(25), KSV(26), KSV(27), KSV(28), KSV(29),
	KSV(30), KSV(31), KSV(32), KSV(33), KSV(34), KSV(35), KSV(36), KSV(37), KSV(38), KSV(39),
	KSV(40), KSV(41), KSV(42), KSV(43), KSV(44), KSV(45), KSV(46), KSV(47), KSV(48), KSV(49),
	KSV(50), KSV(51), KSV(52), KSV(53), KSV(54), KSV(55), KSV(56), KSV(57), KSV(58), KSV(59),
	KSV(60), KSV(61), KSV(62), KSV(63), KSV(64), KSV(65), KSV(66), KSV(67), KSV(68), KSV(69),
	KSV(70), KSV(71), KSV(72), KSV(73), KSV(74), KSV(75), KSV(76), KSV(77), KSV(78), KSV(79),
	KSV(80), KSV(81), KSV(82), KSV(83), KSV(84), KSV(85), KSV(86), KSV(87), KSV(88), KSV(89),
	KSV(90), KSV(91), KSV(92), KSV(93), KSV(94), KSV(95), KSV(96), KSV(97), KSV(98), KSV(99)
};

#undef KSV


// Threat[defended/weak][minor/major attacking][attacked PieceType] contains
// bonuses according to which piece type attacks which one.
const eval Threat[][2][6] = {
	{ { eval(0, 0), eval(0, 0), eval(10, 18), eval(12, 19), eval(22, 49), eval(18, 53) }, // Defended Minor
	{ eval(0, 0), eval(0, 0), eval(5, 7), eval(5, 7), eval(4, 7), eval(12, 24) } }, // Defended Major
	{ { eval(0, 0), eval(0, 16), eval(17, 21), eval(16, 25), eval(21, 50), eval(18, 52) }, // Weak Minor
	{ eval(0, 0), eval(0, 14), eval(13, 29), eval(13, 29), eval(0, 22), eval(12, 26) } } // Weak Major
};

const eval Hanging(16, 13);
const eval KingOnOne(1, 29);
const eval KingOnMany(3, 63);
const eval ROOK_ON_OPENFILE(10, 0);
const eval ROOK_ON_SEMIOPENFILE(10, 0);
const eval ROOK_ON_SEMIOPENFILE_WITH_KQ(5, 0);
const eval ROOK_ON_7TH(20, 0);

const Value BONUS_KNIGHT_OUTPOST = Value(5);
const Value BONUS_BISHOP_OUTPOST = Value(0);

//Parameters for Pawn Structure Evaluation
const eval PASSED_PAWN_BONUS[6] = { eval(0), eval(0), eval(30), eval(37), eval(77), eval(140) };
const eval BONUS_PROTECTED_PASSED_PAWN[6] = { eval(0), eval(0), eval(0), eval(30), eval(30), eval(30) };
const eval MALUS_ISOLATED_PAWN = eval(5);
const eval MALUS_BACKWARD_PAWN = eval(20);
const eval MALUS_ISLAND_COUNT = eval(5);
const eval BONUS_CANDIDATE = eval(5);
const eval BONUS_LEVER = eval(5);
const eval MALUS_DOUBLED_PAWN = eval(0,20);

////Bonus for Passed Pawns (dynamic evaluations)
//const eval BONUS_PASSED_PAWN_BACKED = eval(0, 15); //Passed pawn with own rook behind
//const eval MALUS_PASSED_PAWN_BLOCKED[6] = { eval(0, 3), eval(0,10), eval(0,10), eval(0,30), EVAL_ZERO, eval(0,30) }; //Passed pawn with own rook behind

const eval BONUS_BISHOP_PAIR = eval(50);
const Value BONUS_CASTLING = Value(0);

const Value BONUS_TEMPO = Value(5);

const Value DELTA_PRUNING_SAFETY_MARGIN = Value(PieceValues[PAWN].egScore);

const Value PAWN_SHELTER_2ND_RANK = Value(20);
const Value PAWN_SHELTER_3RD_RANK = Value(10);

//const Value BETA_PRUNING_MARGIN[8] = { Value(0), Value(200), Value(400), Value(600), Value(800), Value(1000), Value(1200), Value(1400) };
const Value BETA_PRUNING_FACTOR = Value(100);

const Value PROBCUT_MARGIN = Value(90);

const int FULTILITY_PRUNING_DEPTH = 3;
const Value FUTILITY_PRUNING_LIMIT[FULTILITY_PRUNING_DEPTH + 1] = { VALUE_ZERO, PieceValues[BISHOP].mgScore, PieceValues[ROOK].mgScore, PieceValues[QUEEN].mgScore };

enum CAPTURES {             
	QxP, BxP, NxP, RxP, QxN,
	QxR, RxN, QxB, BxN, RxB,
	KxP, EP, PxP, KxN, PxB,
	BxR, PxN, NxR, NxN, NxB,
	RxR, BxB, PxR, KxB, KxR,
	QxQ, 
	RxQ, BxQ, NxQ, PxQ, KxQ //Winning Captures of Queen
};

const Value CAPTURE_SCORES[6][6] = {
	// Captured:  QUEEN, ROOK,   BISHOP,   KNIGHT,    PAWN,      EP-Capture
		{ Value(QxQ), Value(QxR),  Value(QxB),  Value(QxN),  Value(QxP),  Value(0) },   // QUEEN
		{ Value(RxQ), Value(RxR), Value(RxB), Value(RxN),  Value(RxP),  Value(0) },   // ROOK
		{ Value(BxQ), Value(BxR), Value(BxB), Value(BxN),  Value(BxP),  Value(0) },   // BISHOP
		{ Value(NxQ), Value(NxR), Value(NxB), Value(NxN), Value(NxP),  Value(0) },   // KNIGHT
		{ Value(PxQ), Value(PxR), Value(PxB), Value(PxN), Value(PxP), Value(EP)},   // PAWN
		{ Value(KxQ), Value(KxR), Value(KxB), Value(KxN), Value(KxP), Value(0) }    // KING
};

const int LMPMoveCount[16] = { 3, 5, 8, 12, 18, 25, 33, 42, 52, 62, 74, 87, 101, 115, 131, 147 };
namespace settings {

	void Initialize();
	int LMRReduction(int depth, int moveNumber);

	const eval SCALE_BISHOP_PAIR_WITH_PAWNS(0); //Reduce Bonus Bishop Pair by this value for each pawn on the board
	const eval BONUS_BISHOP_PAIR_NO_OPP_MINOR(0); //Bonus for Bishop pair, if opponent has no minor piece for exchange
	const eval SCALE_EXCHANGE_WITH_PAWNS(0); //Decrease Value of Exchange with number of pawns
	const eval SCALE_EXCHANGE_WITH_MAJORS(0); //Decrease Value of Exchange with number of majors

	enum OptionType { SPIN, CHECK, BUTTON, STRING };

	const std::string OPTION_HASH = "Hash";
	const std::string OPTION_CHESS960 = "UCI_Chess960";
	const std::string OPTION_CLEAR_HASH = "Clear Hash";
	const std::string OPTION_CONTEMPT = "Contempt";
	const std::string OPTION_MULTIPV = "MultiPV";
	const std::string OPTION_THREADS = "Threads";
	const std::string OPTION_PONDER = "Ponder";
	const std::string OPTION_BOOK_FILE = "BookFile";
	const std::string OPTION_OWN_BOOK = "OwnBook";
	const std::string OPTION_OPPONENT = "UCI_Opponent";
#ifdef TB
	const std::string OPTION_SYZYGY_PATH = "SyzygyPath";
	const std::string OPTION_SYZYGY_PROBE_DEPTH = "SyzygyProbeDepth";
#endif

	extern const eval PSQT[12][64];
	void initPSQT();

	class Option {
	public:
		Option() { };
		Option(std::string Name, OptionType Type = OptionType::BUTTON, std::string DefaultValue = "", std::string MinValue = "", std::string MaxValue = "");
		void virtual set(std::string value) = 0;
		void virtual read(std::vector<std::string> &tokens) = 0;
		std::string printUCI();
		inline std::string getName() { return name; }
	protected:
		std::string name;
		OptionType otype;
		std::string defaultValue;
		std::string minValue;
		std::string maxValue;
	};



	class OptionSpin: public Option {
	public:
		OptionSpin(std::string Name, int Value, int Min, int Max) : Option(Name, OptionType::SPIN, std::to_string(Value), std::to_string(Min), std::to_string(Max)) { };
		inline void set(std::string value) { _value = stoi(value); }
		inline void set(int value) { _value = value; }
		inline int getValue() { 
			if (_value == INT_MIN) set(defaultValue); 
			return _value; 
		}
		inline void read(std::vector<std::string> &tokens) { set(tokens[4]); }
	protected:
		int _value = INT_MIN;
	};

	class OptionCheck : public Option {
	public:
		OptionCheck(std::string Name, bool value);
		void set(std::string value);
		inline bool getValue() { return _value; }
		inline void read(std::vector<std::string> &tokens) { set(tokens[4]); }
		inline void set(bool value) { _value = value; };
	protected:
		bool _value = true;
	};

	class OptionString : public Option {
	public:
		OptionString(std::string Name, std::string defaultValue = "") : Option(Name, OptionType::STRING, defaultValue) { };
		void set(std::string value) { _value = value; }
		inline std::string getValue() { return _value; }
		void read(std::vector<std::string> &tokens);
	protected:
		std::string _value;
	};

	class OptionButton : public Option {
	public:
		OptionButton(std::string Name) : Option(Name, OptionType::BUTTON) { };
		void set(std::string value) { }
		inline void read(std::vector<std::string> &tokens) { }
	};

	class OptionThread : public OptionSpin {
	public:
		OptionThread() : OptionSpin(OPTION_THREADS, HelperThreads + 1, 1, 128) { };
		void set(std::string value);
	};
	
	class Option960 : public OptionCheck {
	public:
		Option960() : OptionCheck(OPTION_CHESS960, Chess960) { };
		void set(std::string value);
		inline void set(bool value) { Chess960 = value; };
	};

	class OptionContempt : public OptionSpin {
	public:
		OptionContempt() : OptionSpin(OPTION_CONTEMPT, Contempt, -1000, 1000) { };
		void set(std::string value);
		void set(int value);
	};

	class OptionHash : public OptionSpin {
	public:
		OptionHash() : OptionSpin(OPTION_HASH, 32, 1, 16384) { };
		void set(std::string value);
		void set(int value);
	};

	class Options : public std::map<std::string, Option *> {
	public:
		Options();
		void printUCI();
		void read(std::vector<std::string> &tokens);
		int getInt(std::string key);
		bool getBool(std::string key);
		std::string getString(std::string key);
		void set(std::string key, std::string value);
		void set(std::string key, int value);
		void set(std::string key, bool value);
	private:
		void initialize();
	};

	extern Options options;

}