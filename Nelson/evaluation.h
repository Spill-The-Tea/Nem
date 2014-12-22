#pragma once

#include "types.h"
#include "board.h"

struct evaluation
{
public:
	Value Material;

	inline Value GetScore(const Phase_t phase, const Color sideToMove) {
		return Material * (1 - 2 * sideToMove);
	}
};

typedef evaluation(*EvalFunction)(position&);

evaluation evaluate(position& pos);
evaluation evaluateFromScratch(position& pos);

 