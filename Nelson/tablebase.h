#pragma once

#include <string>
#include "position.h"
#include "syzygy/tbprobe.h"

namespace Tablebases {

	extern int MaxCardinality;

	void init(const std::string& path);
	int probe_wdl(position& pos, int *success);
	bool root_probe(position& pos, std::vector<ValuatedMove>& rootMoves, Value& score);

}