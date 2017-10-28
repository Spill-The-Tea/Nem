#pragma once

#include <string>
#include <vector>
#include "position.h"
#include "syzygy/tbprobe.h"

namespace tablebases {

	extern int MaxCardinality;

	void init(const std::string& path);
	int probe_wdl(Position& pos, int *success);
	bool root_probe(Position& pos, std::vector<ValuatedMove>& rootMoves, Value& score);

	void probe(Position& pos);

	void printResult(int wdl);

}