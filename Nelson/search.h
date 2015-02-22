#pragma once

#include "types.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include "polyglotbook.h"
#include <iostream>

enum NodeType { ROOT, PV, NULL_MOVE, EXPECTED_CUT_NODE, QSEARCH_DEPTH_0, QSEARCH_DEPTH_NEGATIVE };

struct search {
public:
	ValuatedMove BestMove;
	int64_t NodeCount;
	int64_t QNodeCount;
	bool Stop = false;
	bool UciOutput = false;
	bool PonderMode = false;
	double BranchingFactor = 0;
	std::string BookFile = "";

	ValuatedMove Think(position &pos, SearchStopCriteria ssc);
	std::string PrincipalVariation(int depth = PV_MAX_LENGTH);
	inline void Reset();
	inline void NewGame();
	inline int Depth() const { return _depth; }
	inline int64_t ThinkTime() const { return _thinkTime; }
	inline double cutoffAt1stMoveRate() const { return 100.0 * cutoffAt1stMove / cutoffCount; }
	inline double cutoffAverageMove() const { return 1 + 1.0 * cutoffMoveIndexSum / cutoffCount; }
	inline Move PonderMove() const { return PVMoves[1]; }
	inline void ExtendStopTimes(int64_t extensionTime) { searchStopCriteria.HardStopTime += extensionTime; searchStopCriteria.SoftStopTime += extensionTime; }
	Move GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount);

private:
	SearchStopCriteria searchStopCriteria;
	int rootMoveCount;
	ValuatedMove * rootMoves;
	Move PVMoves[PV_MAX_LENGTH];
	HistoryStats History;
	int _depth = 0;
	int64_t _thinkTime;
	uint64_t cutoffAt1stMove = 0;
	uint64_t cutoffCount = 0;
	uint64_t cutoffMoveIndexSum = 0;
	ExtendedMove killer[MAX_DEPTH][2];
	Move counterMove[12 * 64];
	Value gains[12 * 64];
	polyglot::polyglotbook book;

	template<NodeType NT> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv);
	template<NodeType NT> Value QSearch(Value alpha, Value beta, position &pos, int depth);
	void updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex);
	inline bool Stopped() { return Stop && (!PonderMode); }

};

	template<> Value search::Search<ROOT>(Value alpha, Value beta, position &pos, int depth, Move * pv);

inline void search::Reset() {
	BestMove.move = MOVE_NONE;
	BestMove.score = VALUE_ZERO;
	NodeCount = 0;
	QNodeCount = 0;
	cutoffAt1stMove = 0;
	cutoffCount = 0;
	cutoffMoveIndexSum = 0;
	Stop = false;
	PonderMode = false;
	History.initialize();
	for (int i = 0; i < MAX_DEPTH; ++i) killer[i][0] = killer[i][1] = EXTENDED_MOVE_NONE;
}

inline void search::NewGame() {
	Reset();
	for (int i = 0; i < 12 * 64; ++i) {
		counterMove[i] = MOVE_NONE;
		//gains[i] = VALUE_ZERO;
	}
	for (int i = 0; i < 12 * 64; ++i) gains[i] = VALUE_ZERO;
}
