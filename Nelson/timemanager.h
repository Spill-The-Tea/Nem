#pragma once

#include <vector>
#include <atomic>
#include "types.h"
#include "settings.h"
#include "position.h"


enum TimeMode  { UNDEF, SUDDEN_DEATH, SUDDEN_DEATH_WITH_INC, CLASSICAL, CLASSICAL_WITH_INC, FIXED_TIME_PER_MOVE, INFINIT, FIXED_DEPTH, NODES };

	class timemanager
	{
	public:
		timemanager();
		timemanager(const timemanager &tm);
		~timemanager();

		void initialize(int time = 0, int inc = 0, int movestogo = 0);
		void initialize(TimeMode mode, int movetime = 0, int depth = MAX_DEPTH, int64_t nodes = INT64_MAX, int time = 0, int inc = 0, int movestogo = 0, int64_t starttime = now(), bool ponder = false);
		//Checks whether Search has to be exited even while in recursion
		inline bool ExitSearch(int64_t nodes = 0, int64_t tnow = now()) const { return tnow >= _hardStopTime || nodes >= _maxNodes; }
		//Checks whether a new iteration shall be started
		bool ContinueSearch(int currentDepth, ValuatedMove bestMove, int64_t nodecount, int64_t tnow = now(), bool ponderMode = false);
		double GetEBF(int depth = MAX_DEPTH) const;

		void PonderHit();

		inline int64_t GetStartTime() const { return _starttime; }
		inline int GetMaxDepth() const { return _maxDepth; }

		inline void reportFailLow() { _failLow = true; }
		std::string print();
		//Updating available time when pondering
		void updateTime(int64_t time);

		void switchToInfinite();
		inline TimeMode Mode() { return _mode; }

	private:
		TimeMode _mode = UNDEF;
		int _time;
		int _inc;
		int _movestogo;
		int64_t _starttime = 0;
		int _maxDepth = MAX_DEPTH;
		int64_t _maxNodes = INT64_MAX;

		std::atomic<long long> _hardStopTime;
		std::atomic<long long> _stopTime;

		int64_t _hardStopTimeSave = INT64_MAX;
		int64_t _stopTimeSave = INT64_MAX;
		bool _failLow = false;

		int64_t _iterationTimes[MAX_DEPTH];
		ValuatedMove _bestMoves[MAX_DEPTH];
		int64_t _nodeCounts[MAX_DEPTH];

		void init();
	};

