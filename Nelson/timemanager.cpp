#include <iostream>
#include "timemanager.h"

timemanager::timemanager() { }

timemanager::timemanager(const timemanager &tm) {
	_starttime = tm._starttime;
	_mode = tm._mode;
	_time = tm._time;
	_inc = tm._inc;
	_movestogo = tm._movestogo;
	_maxDepth = tm._maxDepth;
	_maxNodes = tm._maxNodes;

	_hardStopTime.store(tm._hardStopTime);
	_stopTime.store(tm._stopTime);
}

timemanager::~timemanager() { }

void timemanager::initialize(int time, int inc, int movestogo) {
	_starttime = now();
	_time = time;
	_inc = inc;
	_movestogo = movestogo;
	_mode = UNDEF;
	_maxDepth = MAX_DEPTH;
	_maxNodes = INT64_MAX;
    _hardStopTime = INT64_MAX;
	_stopTime = INT64_MAX;
	init();
}

void timemanager::initialize(TimeMode mode, int movetime, int depth, int64_t nodes, int time, int inc, int movestogo, int64_t starttime, bool ponder) {
	_starttime = starttime;
	_time = time;
	_inc = inc;
	_movestogo = movestogo;
	_maxNodes = nodes & (~MASK_TIME_CHECK);
	_mode = UNDEF;
	_maxDepth = std::min(MAX_DEPTH, depth);
	_hardStopTime = INT64_MAX;
	_stopTime = INT64_MAX;
	if (mode == FIXED_TIME_PER_MOVE) {
		_mode = mode;
		_hardStopTime = _starttime + _time - EmergencyTime;
	}
	else if (mode == FIXED_DEPTH) {
		_mode = mode;
		_maxDepth = std::min(MAX_DEPTH, depth);
	}
	else if (mode == NODES) {
		_mode = mode;
		_maxNodes = nodes;
	}
	else if (mode == INFINIT) {
		_hardStopTime = _stopTime = INT64_MAX;
	}
	else init();
	if (ponder) {
		_hardStopTimeSave = _hardStopTime;
		_stopTimeSave = _stopTime;
		_hardStopTime.store(INT64_MAX);
		_stopTime.store(INT64_MAX);
	}
}

void timemanager::PonderHit() {
	int64_t pondertime = now() - _starttime;
	_hardStopTime.fetch_add(pondertime);
	_hardStopTime.store(_hardStopTimeSave + pondertime);
	_stopTime.store(_stopTimeSave + pondertime);
}

void timemanager::init() {
	if (_time == 0) _mode = INFINIT;
	else {
		int remainingMoves;
		if (_movestogo == 0){
			if (_inc == 0) _mode = SUDDEN_DEATH; else _mode = SUDDEN_DEATH_WITH_INC;
			remainingMoves = 30;
		}
		else {
			if (_inc == 0) _mode = CLASSICAL; else _mode = CLASSICAL_WITH_INC;
			remainingMoves = _movestogo;
		}
		//Leave in any case some emergency time for remaining moves
		int64_t remainingTime = _time + (_movestogo * _inc);
		//int64_t emergencySpareTime = _movestogo > 1 ? remainingTime / 3 : EmergencyTime;
		//_hardStopTime = std::min(_starttime + _time - EmergencyTime, _starttime + _time - emergencySpareTime);
		//Give at least 10 ms
		_hardStopTime = std::max(_starttime + _time - EmergencyTime, _starttime + 10);
		_stopTime = std::min(int64_t(_starttime + remainingTime / remainingMoves), int64_t((_hardStopTime + _starttime)/2));
		//_hardStopTime is now avoiding time forfeits, nevertheless it's still possible that an instable search uses up all time, so that
		//all further moves have to be played a tempo. To avoid this _hardStopTime is reduced so that for further move a reasonable amount of time
		//is left
		if (_inc == 0 && _movestogo > 1) {
			int64_t spareTime = _time / 3; //Leave 1/3 of time for further moves
			_hardStopTime -= spareTime;
		}
	}
}

bool timemanager::ContinueSearch(int currentDepth, ValuatedMove bestMove, int64_t nodecount, int64_t tnow, bool ponderMode) {
	_bestMoves[currentDepth - 1] = bestMove;
	_nodeCounts[currentDepth - 1] = nodecount;
	//_iterationTimes[currentDepth - 1] = tnow - _starttime;
	//if (ponderMode) return true;
	if (ExitSearch(tnow)) return false;
	switch (_mode)
	{
	case INFINIT:
		return true;
	case FIXED_DEPTH:
		return currentDepth < _maxDepth;
	case FIXED_TIME_PER_MOVE:
		return _hardStopTime > tnow;
	default:
		//double bf = 2;
		bool stable = currentDepth > 3 && _bestMoves[currentDepth - 1].move == _bestMoves[currentDepth - 2].move && _bestMoves[currentDepth - 1].move == _bestMoves[currentDepth - 3].move
			&& std::abs(_bestMoves[currentDepth - 1].score - _bestMoves[currentDepth - 2].score) < 0.1;
		//if stable only start iteration if there is 3 times more time available than already spent - if unstable start next iteration even if only 2 times the spent time is left
		double factor = stable ? 3 : 2;
		//if (currentDepth > 3 && _iterationTimes[currentDepth - 3] > 0) bf = sqrt(1.0 * _iterationTimes[currentDepth - 1] / _iterationTimes[currentDepth - 3]);
		return tnow < _stopTime && (_starttime + int64_t(factor * (tnow - _starttime))) <= _stopTime;
	}

}

double timemanager::GetEBF(int depth) const {
	int d = 0;
	double wbf = 0;
	int64_t n = 0;
	for (d = 2; d < depth; ++d) {
		double bf = sqrt(1.0 * _nodeCounts[d] / _nodeCounts[d - 2]);
		wbf += bf * _nodeCounts[d - 1];
		n += _nodeCounts[d - 1];
	}
	return wbf / n;
}
