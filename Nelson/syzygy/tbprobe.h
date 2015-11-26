#ifndef TBPROBE_H
#define TBPROBE_H

#include <vector>

namespace Tablebases {

extern int MaxCardinality;

void init(const std::string& path);
int probe_wdl(position& pos, int *success);
int probe_dtz(position& pos, int *success);
bool root_probe(position& pos, std::vector<ValuatedMove>& rootMoves, Value& score);
bool root_probe_wdl(position& pos, std::vector<ValuatedMove>& rootMoves, Value& score);

}

#endif
