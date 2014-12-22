#pragma once

#include "types.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include <atomic>

enum NodeType { ROOT, PV, NULL_MOVE, STANDARD };

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

	inline void Reset() {
		BestMove.move = MOVE_NONE;
		BestMove.score = VALUE_ZERO;
		NodeCount = 0;
		QNodeCount = 0;
		Stop = false;
	}

private:
	SearchStopCriteria searchStopCriteria;
	int rootMoveCount;
	ValuatedMove * rootMoves;
	Move PVMoves[PV_MAX_LENGTH];
	template<NodeType NT> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv);
	template<> Value Search<ROOT>(Value alpha, Value beta, position &pos, int depth, Move * pv);
	template<NodeType NT> Value QSearch(Value alpha, Value beta, position &pos, int depth);

};