#include <iostream>
#include <sstream>
#include "timemanager.h"
#include "utils.h"

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

void timemanager::initialize(TimeMode mode, int movetime, int depth, int64_t nodes, int time, int inc, int movestogo, Time_t starttime, bool ponder) {
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
		_mode = mode;
		_hardStopTime = _stopTime = INT64_MAX;
	}
	else init();
	if (ponder) {
		_hardStopTimeSave = _hardStopTime;
		_stopTimeSave = _stopTime;
		_hardStopTime.store(INT64_MAX);
		_stopTime.store(INT64_MAX);
	}
	//utils::debugInfo(print());
}

void timemanager::init() {
	if (_time == 0) _mode = INFINIT;
	else {
		_failLowDepth = 0;
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
		_stopTime = std::min(int64_t(_starttime + remainingTime / remainingMoves), Time_t((_hardStopTime + _starttime)/2));
		//_hardStopTime is now avoiding time forfeits, nevertheless it's still possible that an instable search uses up all time, so that
		//all further moves have to be played a tempo. To avoid this _hardStopTime is reduced so that for further move a reasonable amount of time
		//is left
		if (_inc == 0 && _movestogo > 1) {
			int64_t spareTime = _time / 3; //Leave 1/3 of time for further moves
			_hardStopTime -= spareTime;
		}
	}
}

void timemanager::PonderHit() {
	int64_t tnow = now();
	int64_t pondertime = tnow - _starttime;
	_hardStopTime.store(std::min(Time_t(_hardStopTimeSave + pondertime - EmergencyTime), Time_t(_hardStopTime.load())));
	_stopTime.store(_stopTimeSave);
	//check if current iteration will be finished 
	if (_mode != FIXED_TIME_PER_MOVE) {
		Time_t usedForCompletedIteration = _completionTimeOfLastIteration - _starttime;
		Time_t usedInLastIteration = tnow - _completionTimeOfLastIteration;
		if (usedInLastIteration < 2 * usedForCompletedIteration) { //If current iteration is not about to be finished, let's check if it's better to return immediately
			bool stable = _completedDepth > 3 && _bestMoves[_completedDepth - 1].move == _bestMoves[_completedDepth - 2].move && _bestMoves[_completedDepth - 1].move == _bestMoves[_completedDepth - 3].move
				&& std::abs(_bestMoves[_completedDepth - 1].score - _bestMoves[_completedDepth - 2].score) < 0.1;
			//if stable continue iteration if there is 3 times more time available than already spent - if unstable continue next iteration even if only 2 times the spent time is left
			double factor = stable ? 3 : 2;
			//If a fail low has occurred assign even more time
			if (_failLowDepth > 0) factor = factor / 2;
			if (_starttime + Time_t(factor * usedForCompletedIteration) >= _stopTime) {
				_hardStopTime.store(tnow); //stop search by adjusting hardStopTime
			}
		}
	}
	//utils::debugInfo(print());
}

// This method is called from search after the completion of each iteration. It returns true when a new iteration shall be started and false if not
bool timemanager::ContinueSearch(int currentDepth, ValuatedMove bestMove, int64_t nodecount, Time_t tnow, bool ponderMode) {
	_completedDepth = currentDepth;
	_completionTimeOfLastIteration = tnow;
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

		bool stable = currentDepth > 3 && _bestMoves[currentDepth - 1].move == _bestMoves[currentDepth - 2].move && _bestMoves[currentDepth - 1].move == _bestMoves[currentDepth - 3].move
			&& std::abs(_bestMoves[currentDepth - 1].score - _bestMoves[currentDepth - 2].score) < 0.1;
		//if stable only start iteration if there is 3 times more time available than already spent - if unstable start next iteration even if only 2 times the spent time is left
		double factor = stable ? 3 : 2;
		//If a fail low has occurred assign even more time
		if (_failLowDepth > 0) factor = factor / 2;
		return tnow < _stopTime && (_starttime + Time_t(factor * (tnow - _starttime))) <= _stopTime;
	}

}

void timemanager::switchToInfinite() {
	_mode = INFINIT;
}

void timemanager::updateTime(int64_t time) {
	Time_t tnow = now();
	_hardStopTime.store(std::min(int64_t(_hardStopTime.load()), tnow + Time_t(time - EmergencyTime)));
	//std::cout << "Hardstop time updated: " << _hardStopTime - _starttime;
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

std::string timemanager::print() {
	std::ostringstream s;
	s << "Start: " << _starttime << "  Time: " << _time << "  MTG: " << _movestogo << "  INC: " << _inc;
	s << " Stop:  " << _stopTime - _starttime << "  Hardstop: " << _hardStopTime - _starttime << "  SavedStop:  " << _stopTimeSave - _starttime << "  SavedHardstop: " << _hardStopTimeSave -_starttime;
	return s.str();
}
