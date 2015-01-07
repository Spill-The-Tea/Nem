#pragma once

#include "types.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include <atomic>
#include <iostream>

enum NodeType { ROOT, PV, NULL_MOVE, STANDARD, QSEARCH_FULL, QSEARCH_REDUCED };

struct search {
public:
	ValuatedMove BestMove;
	int64_t NodeCount;
	int64_t QNodeCount;
	atomic<bool> Stop = false;
	bool UciOutput = false;
	double BranchingFactor = 0;

	ValuatedMove Think(position &pos, SearchStopCriteria ssc);
	string PrincipalVariation(int depth = PV_MAX_LENGTH);
	inline void Reset();
	inline int Depth() const { return _depth; }
	inline int64_t ThinkTime() const { return _thinkTime; }

private:
	SearchStopCriteria searchStopCriteria;
	int rootMoveCount;
	ValuatedMove * rootMoves;
	Move PVMoves[PV_MAX_LENGTH];
	HistoryStats History;
	int _depth = 0;
	int64_t _thinkTime;

	template<NodeType NT> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv);
	template<> Value Search<ROOT>(Value alpha, Value beta, position &pos, int depth, Move * pv);
	template<NodeType NT> Value QSearch(Value alpha, Value beta, position &pos, int depth);
	void updateCutoffStats(const Move cutoffMove, int depth, position &pos);

};

inline void search::Reset() {
	BestMove.move = MOVE_NONE;
	BestMove.score = VALUE_ZERO;
	NodeCount = 0;
	QNodeCount = 0;
	Stop = false;
	History.initialize();
}