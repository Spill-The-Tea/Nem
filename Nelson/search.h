#pragma once

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "types.h"
#include "book.h"
#include "board.h"
#include "position.h"
#include "settings.h"
#include "timemanager.h"

#ifdef TB
#include "syzygy/tbprobe.h"
#endif

#ifdef STAT
#include "stat.h"
#endif


enum ThreadType { SINGLE, MASTER, SLAVE };

struct baseSearch {
public:
	bool UciOutput = false;
	bool PrintCurrmove = true;
	std::atomic<bool> PonderMode;
	ValuatedMove BestMove;
	int64_t NodeCount = 0;
	int64_t QNodeCount = 0;
	int MaxDepth = 0;
	std::atomic<bool> Stop;
	std::atomic<bool> Exit;
	//bool Stop = false;
	double BranchingFactor = 0;
	std::string BookFile = "";
	int rootMoveCount = 0;
	ValuatedMove * rootMoves = nullptr;
	position rootPosition;
	std::vector<Move> searchMoves;
	int MultiPv = 1;

	baseSearch();
	virtual ~baseSearch();
	void Reset();
	std::string PrincipalVariation(int depth = PV_MAX_LENGTH);
	void NewGame();
	inline int Depth() const { return _depth; }
	inline int64_t ThinkTime() const { return _thinkTime; }
	inline double cutoffAt1stMoveRate() const { return 100.0 * cutoffAt1stMove / cutoffCount; }
	inline double cutoffAverageMove() const { return 1 + 1.0 * cutoffMoveIndexSum / cutoffCount; }
	inline Move PonderMove() const { return PVMoves[1]; }
	inline void StopThinking() {
		PonderMode.store(false);
		Stop.store(true);
	}
	Move GetBestBookMove(position& pos, ValuatedMove * moves, int moveCount);
	virtual ThreadType GetType() = 0;

	//protected:
	polyglot::book * book = nullptr;
	CounterMoveHistoryManager cmHistory;
	uint64_t cutoffAt1stMove = 0;
	uint64_t cutoffCount = 0;
	uint64_t cutoffMoveIndexSum = 0;
    HistoryManager History;
	ExtendedMove killer[2 * MAX_DEPTH];
	//Move excludedMoves[MAX_DEPTH];
	timemanager timeManager;
	Move PVMoves[PV_MAX_LENGTH];
	int _depth = 0;
	int64_t _thinkTime;
	Move counterMove[12 * 64];
#ifdef TB
	uint64_t tbHits = 0;
	bool probeTB = true;
#endif

	inline bool Stopped() { return Stop; }

};

template<ThreadType T> struct search : public baseSearch {
public:
	baseSearch * master = nullptr;

	search();
	~search();
	inline ValuatedMove Think(position &pos);

	void startHelper();
	void initializeSlave(baseSearch * masterSearch, int id);
	ThreadType GetType() { return T; }
	std::condition_variable cvStart;
	std::mutex mtxStart;
	search<SLAVE> * slaves = nullptr;
	std::vector<std::thread> subThreads;
	bool isQuiet(position &pos);

private:
	template<bool PVNode> Value Search(Value alpha, Value beta, position &pos, int depth, Move * pv, bool prune = true, bool skipNullMove = false);
	Value SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv, int startWithMove = 0);
	template<bool WithChecks> Value QSearch(Value alpha, Value beta, position &pos, int depth);
	void updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex);

	int id = 0;



};



void startThread(search<SLAVE> & slave);

template<ThreadType T> inline ValuatedMove search<T>::Think(position &pos) {
	_thinkTime = 1;
	rootPosition = pos;
	pos.ResetPliesFromRoot();
	EngineSide = pos.GetSideToMove();
	tt::newSearch();
	ValuatedMove * generatedMoves = pos.GenerateMoves<LEGAL>();
	rootMoveCount = pos.GeneratedMoveCount();
	if (rootMoveCount == 1) {
		BestMove = *generatedMoves; //if there is only one legal move save time and return move immediately (although there is no score assigned)
		goto END;
	}
	//check for book
	if (USE_BOOK && BookFile.size() > 0) {
		if (book == nullptr) book = new polyglot::book(BookFile);
		Move bookMove = book->probe(pos, false, generatedMoves, rootMoveCount);
		if (bookMove != MOVE_NONE) {
			BestMove.move = bookMove;
			BestMove.score = VALUE_ZERO;
			if (UciOutput) std::cout << "info string book move" << std::endl;
			goto END;
		}
	}
	if (searchMoves.size()) {
		rootMoveCount = int(searchMoves.size());
		rootMoves = new ValuatedMove[rootMoveCount];
		for (int i = 0; i < rootMoveCount; ++i) rootMoves[i].move = searchMoves[i];
		searchMoves.clear();
	}
	else {
		rootMoves = new ValuatedMove[rootMoveCount];
		memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	}
#ifdef TB
	tbHits = 0;
	probeTB = true;
	if (pos.GetMaterialTableEntry()->IsTablebaseEntry()) {
		std::vector<ValuatedMove> tbMoves(rootMoves, rootMoves + rootMoveCount);
		Value score;
		bool tbHit = Tablebases::root_probe(pos, tbMoves, score);
		if (!tbHit) tbHit = Tablebases::root_probe_wdl(pos, tbMoves, score);
		if (tbHit) {
			tbHits++;
			delete[] rootMoves;
			rootMoveCount = (int)tbMoves.size();
			rootMoves = new ValuatedMove[rootMoveCount];
			std::copy(tbMoves.begin(), tbMoves.end(), rootMoves);
			if (rootMoveCount == 1) {
				BestMove = rootMoves[0]; //if tablebase probe only returns one move play => play it and done!
				goto END;
			}
			probeTB = false;
		}
	}
#endif
	std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	if (T == MASTER) {
		if (slaves == nullptr) {
			slaves = new search<SLAVE>[HelperThreads];
			for (int i = 0; i < HelperThreads; ++i) subThreads.push_back(std::thread(&startThread, std::ref(slaves[i])));
		}
		for (int i = 0; i < HelperThreads; ++i) {
			//slaves[i].initializeSlave(this);			
			slaves[i].mtxStart.lock();
			slaves[i].initializeSlave(this, i + 1);
			slaves[i].mtxStart.unlock();
			slaves[i].cvStart.notify_one();
		}
	}
	if (timeManager.GetMaxDepth() == 0) {
		BestMove.move = MOVE_NONE;
		BestMove.score = pos.evaluate();
		if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD)) std::cout << "info score cp " << BestMove.score << std::endl;
		else {
			int pliesToMate;
			if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
			std::cout << "info score mate " << pliesToMate / 2 << std::endl;
		}
		return BestMove;
	}
	//Iterativ Deepening Loop
	for (_depth = 1; _depth <= timeManager.GetMaxDepth(); ++_depth) {
		Value alpha, beta, score, delta = Value(20);
		for (int pvIndx = 0; pvIndx < MultiPv; ++pvIndx) {
			if (_depth >= 5 && MultiPv == 1) {
				alpha = std::max(score - delta, -VALUE_INFINITE);
				beta = std::min(score + delta, VALUE_INFINITE);
			}
			else {
				alpha = -VALUE_MATE;
				beta = VALUE_MATE;
			}
			while (true) {
				score = SearchRoot(alpha, beta, pos, _depth, &PVMoves[0], pvIndx);
				//Best move is already in first place, this is assured by SearchRoot
				//therefore we sort only the other moves
				std::stable_sort(rootMoves + pvIndx + 1, &rootMoves[rootMoveCount], sortByScore);
				if (Stopped()) break;
				else if (score <= alpha) {
					beta = (alpha + beta) / 2;
					alpha = std::max(score - delta, -VALUE_INFINITE);
					if (!PonderMode.load()) timeManager.reportFailLow();
				}
				else if (score >= beta) {
					alpha = (alpha + beta) / 2;
					beta = std::min(score + delta, VALUE_INFINITE);
				}
				else {
					BestMove = rootMoves[0];
					score = BestMove.score;
					break; 
				}
				delta += delta / 4 + 5;
			}
			int64_t tNow = now();
			_thinkTime = std::max(tNow - timeManager.GetStartTime(), int64_t(1));
			Stop.load();
			//PonderMode.load();
			if (!Stopped()) {
				if (!timeManager.ContinueSearch(_depth, BestMove, NodeCount, tNow, PonderMode)) Stop.store(true);
			}
			if (T == MASTER || UciOutput) {
				if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
					std::cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score cp " << BestMove.score << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
					<< " hashfull " << tt::GetHashFull()
#ifdef TB
					<< " tbhits " << tbHits
#endif
					<< " time " << _thinkTime
					<< " pv " << PrincipalVariation(_depth) << std::endl;
				else {
					int pliesToMate;
					if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
					std::cout << "info depth " << _depth << " seldepth " << MaxDepth << " multipv " << pvIndx + 1 << " score mate " << pliesToMate / 2 << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
						<< " hashfull " << tt::GetHashFull()
#ifdef TB
						<< " tbhits " << tbHits
#endif
						<< " time " << _thinkTime
						<< " pv " << PrincipalVariation(_depth) << std::endl;
				}
			}
			if (Stopped()) break;
		}
		if (Stopped()) break;
	}

	if (T == MASTER) {
		if (_depth >= MIN_SMP_DEPTH) {
			for (int i = 0; i < HelperThreads; ++i) slaves[i].StopThinking();
			//for (int i = 0; i < HelperThreads; ++i) subThreads[i].join();
		}
		//delete[] slaves;
	}

END:	while (PonderMode.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	if (BestMove.move == MOVE_NONE) BestMove = rootMoves[0];
	if (rootMoves) delete[] rootMoves;
	rootMoves = nullptr;
	return BestMove;
}

template<ThreadType T> void search<T>::initializeSlave(baseSearch * masterSearch, int Id) {
	if (T == SLAVE) {
		master = masterSearch;
		id = Id;
		rootMoveCount = masterSearch->rootMoveCount;
		rootMoves = new ValuatedMove[rootMoveCount];
		memcpy(rootMoves, masterSearch->rootMoves, rootMoveCount * sizeof(ValuatedMove));
		rootPosition = position(masterSearch->rootPosition);
		rootPosition.copy(masterSearch->rootPosition);
		rootPosition.InitMaterialPointer();
		//this->DblHistory = masterSearch->DblHistory;
		Stop.store(false);
	}
}

template<ThreadType T> void search<T>::startHelper() {
	while (!Exit.load()) {
		std::unique_lock<std::mutex> lckStart(mtxStart);
		while (master == nullptr) cvStart.wait(lckStart);
		int depth = 1;
		std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
		while (!Stop && depth <= MAX_DEPTH) {
			Value alpha = -VALUE_MATE;
			Value beta = VALUE_MATE;
			SearchRoot(alpha, beta, rootPosition, depth, &PVMoves[0]);
			std::stable_sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
			++depth;
		}
		master = nullptr;
	}
	delete[] rootMoves;
}

template<ThreadType T> search<T>::search() {  }

template<ThreadType T> search<T>::~search() {
	if (T == MASTER && slaves != nullptr) {
		for (int i = 0; i < HelperThreads; ++i) {
			slaves[i].Exit.store(true);
			slaves[i].master = this;
			slaves[i].cvStart.notify_one();
			slaves[i].StopThinking();
			subThreads[i].join();
		}
		delete[] slaves;
	}
}

template<ThreadType T> Value search<T>::SearchRoot(Value alpha, Value beta, position &pos, int depth, Move * pv, int startWithMove) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Value bonus;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool ZWS = false;
	bool lmr = !pos.Checked() && depth >= 3;
	//	int repetitions = T == SINGLE ? 1 : 1;
	//	for (int repetition = 0; repetition < repetitions; ++repetition){
	for (int i = startWithMove; i < rootMoveCount; ++i) {
		if (T != SLAVE) {
			if (PrintCurrmove && depth > 5 && (now() - timeManager.GetStartTime()) > 3000) {
				std::cout << "info depth " << depth << " currmove " << toString(rootMoves[i].move) << " currmovenumber " << i + 1 << std::endl;
			}
		}
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		//Check if other thread is already processing this node
		//if (T != SINGLE) {
		//	if (repetition == 0 && i > 0 && tt::GetNProc(next.GetHash())> 0) continue; else tt::IncrementNProc(next.GetHash());
		//}
		int reduction = 0;
		if (lmr && i >= startWithMove + 5 && pos.IsQuietAndNoCastles(rootMoves[i].move) && !next.Checked()) {
			reduction = 1;
		}
		if (type(rootMoves[i].move) == CASTLING) bonus = BONUS_CASTLING; else bonus = VALUE_ZERO;
		if (ZWS) {
			score = bonus - Search<false>(Value(bonus - alpha - 1), bonus - alpha, next, depth - 1 - reduction, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			if (score > alpha && score < beta) score = bonus - Search<true>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
		}
		else {
			score = bonus - Search<true>(bonus - beta, bonus - alpha, next, depth - 1 - reduction, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			if (reduction > 0 && score > alpha && score < beta) score = bonus - Search<true>(bonus - beta, bonus - alpha, next, depth - 1, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
		}
		//if (T != SINGLE) tt::DecrementNProc(next.GetHash());
		if (Stopped()) break;
		rootMoves[i].score = score;
		if (score > bestScore) {
			bestScore = score;
			pv[0] = rootMoves[i].move;
			memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
			if (i > 0) {
				//make sure that best move is always in first place 
				ValuatedMove bm = rootMoves[i];
				for (int j = i; j > 0; --j) rootMoves[j] = rootMoves[j - 1];
				rootMoves[0] = bm;
			}
			if (score >= beta) return score;
			else if (score > alpha)
			{
				ZWS = true;
				alpha = score;
			}
		}
	}
	//	}
	return bestScore;
}

template<ThreadType T> template<bool PVNode> Value search<T>::Search(Value alpha, Value beta, position &pos, int depth, Move * pv, bool prune, bool skipNullMove) {
	++NodeCount;
	if (T != SLAVE) {
		if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
	}
	if (Stopped()) return VALUE_ZERO;
	if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	//Mate distance pruning
	alpha = Value(std::max(int(-VALUE_MATE) + pos.GetPliesFromRoot(), int(alpha)));
	beta = Value(std::min(int(VALUE_MATE) - pos.GetPliesFromRoot() - 1, int(beta)));
	if (alpha >= beta) return alpha;
	if (depth <= 0) {
		return QSearch<true>(alpha, beta, pos, depth);
	}
	//TT lookup
	bool ttFound;
	tt::Entry ttEntry;
	tt::Entry* ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	Move ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		if (ttMove && pos.IsQuiet(ttMove)) updateCutoffStats(ttMove, depth, pos, -1);
		return ttValue;
	}
#ifdef TB
	// Step 4a. Tablebase probe
	if (probeTB && depth >= SYZYGY_PROBE_DEPTH && pos.GetDrawPlyCount() == 0 && pos.GetMaterialTableEntry()->IsTablebaseEntry())
	{
		int found, v = Tablebases::probe_wdl(pos, &found);
		if (found)
		{
			tbHits++;
			Value value = v < -1 ? -VALUE_MATE + MAX_DEPTH + pos.GetPliesFromRoot()
				: v >  1 ? VALUE_MATE - MAX_DEPTH - pos.GetPliesFromRoot()
				: VALUE_DRAW + 2 * v;
			if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(value, pos.GetPliesFromRoot()), tt::EXACT, MAX_DEPTH - 1, MOVE_NONE, VALUE_NOTYETDETERMINED);
			return value;
		}
	}
#endif
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	bool checked = pos.Checked();
	Value staticEvaluation = checked ? VALUE_NOTYETDETERMINED :
		ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate() + BONUS_TEMPO;
	if (!skipNullMove && prune && !checked) {
		//Check if Value from TT is better
		Value effectiveEvaluation = staticEvaluation;
		if (ttFound &&
			((ttValue > staticEvaluation && ttEntry.type() == tt::LOWER_BOUND)
				|| (ttValue < staticEvaluation && ttEntry.type() == tt::UPPER_BOUND))) effectiveEvaluation = ttValue;

		//Razoring a la SF 
		if (!checked && depth < 4
			&& (effectiveEvaluation + Value(256 + 16 * depth)) <= alpha
			&& ttMove == MOVE_NONE
			&& !pos.PawnOn7thRank())
		{
			if (depth <= 1 && (effectiveEvaluation + 302) <= alpha) return QSearch<true>(alpha, beta, pos, 0);
			Value razorAlpha = alpha - Value(256 + 16 * depth);
			Value razorScore = QSearch<true>(razorAlpha, Value(razorAlpha + 1), pos, 0);
			if (razorScore <= razorAlpha) return razorScore;
		}
		// Beta Pruning
		if (depth < 7
			&& effectiveEvaluation < VALUE_KNOWN_WIN
			&& (effectiveEvaluation - BETA_PRUNING_FACTOR * depth) >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove()))
			return effectiveEvaluation - BETA_PRUNING_FACTOR * depth;
		//Null Move Pruning
		if (depth > 1 //only if there is available depth to reduce
			&& effectiveEvaluation >= beta
			&& pos.NonPawnMaterial(pos.GetSideToMove())
			) {
			int reduction = (depth + 14) / 5;
			Square epsquare = pos.GetEPSquare();
			pos.NullMove();
			Value nullscore = -Search<false>(-beta, -alpha, pos, depth - reduction, &subpv[0], false, true);
			pos.NullMove(epsquare);
			if (nullscore >= beta) {
				if (nullscore >= VALUE_MATE_THRESHOLD) nullscore = beta;
				if (depth < 9 && beta < VALUE_KNOWN_WIN) return nullscore;
				// Do verification search at high depths
				Value verificationScore = Search<false>(beta - 1, beta, pos, depth - reduction, &subpv[0], false);
				if (verificationScore >= beta) return nullscore;
			}
		}

		// ProbCut (copied from SF)
		if (!PVNode
			&&  depth >= 5
			&& std::abs(int(beta)) < VALUE_MATE_THRESHOLD)
		{
			Value rbeta = std::min(Value(beta + 90), VALUE_INFINITE);
			int rdepth = depth - 4;

			position cpos(pos);
			cpos.copy(pos);
			Value limit = PieceValuesMG[GetPieceType(pos.getCapturedInLastMove())];
			Move ttm = ttMove;
			if (ttm != MOVE_NONE && cpos.SEE(ttMove) < limit) ttm = MOVE_NONE;
			cpos.InitializeMoveIterator<QSEARCH>(&History, &cmHistory, nullptr, MOVE_NONE, ttm, limit);
			Move move;
			while ((move = cpos.NextMove())) {
				position next(cpos);
				if (next.ApplyMove(move)) {
					Value score = -Search<false>(-rbeta, Value(-rbeta + 1), next, rdepth, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
					if (score >= rbeta)
						return score;
				}
			}
		}
	}
	//IID: No Hash move available => Try to find one with a reduced search
	if (!ttMove && PVNode ? depth >= 4 : depth > 6) {
		position next(pos);
		next.copy(pos);
		Search<PVNode>(alpha, beta, next, PVNode ? depth - 2 : depth / 2, &subpv[0], true);
		if (Stopped()) return VALUE_ZERO;
		ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
		ttMove = ttFound ? ttEntry.move() : MOVE_NONE;
	}
	Move counter = pos.GetCounterMove(counterMove);
	//Futility Pruning I: If quiet moves can't raise alpha, only generate tactical moves and moves which give check
	bool futilityPruning = !skipNullMove && !checked && depth <= FULTILITY_PRUNING_DEPTH && beta < VALUE_MATE_THRESHOLD && pos.NonPawnMaterial(pos.GetSideToMove());
	if (futilityPruning && alpha > (staticEvaluation + FUTILITY_PRUNING_LIMIT[depth]))
		pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, &cmHistory, nullptr, counter, ttMove);
	else
		pos.InitializeMoveIterator<MAIN_SEARCH>(&History, &cmHistory, &killer[2 * pos.GetPliesFromRoot()], counter, ttMove);
	Value score;
	Value bestScore = -VALUE_MATE;
	tt::NodeType nodeType = tt::UPPER_BOUND;
	bool lmr = !checked && depth >= 3;
	Move move;
	int moveIndex = 0;
	bool ZWS = false;
	Square recaptureSquare = pos.GetLastAppliedMove() && pos.Previous()->GetPieceOnSquare(to(pos.GetLastAppliedMove())) != BLANK ? to(pos.GetLastAppliedMove()) : OUTSIDE;
	while ((move = pos.NextMove())) {
		bool critical = move == counter || pos.GetMoveGenerationType() < QUIETS_POSITIVE || moveIndex == 0;
		if (!PVNode && !checked && !critical && depth <= 3 && moveIndex >= LMPMoveCount[depth]) {
			moveIndex++;
			continue;
		}
		bool prunable = !PVNode && !checked && move != ttMove && move != counter && std::abs(int(bestScore)) <= VALUE_MATE_THRESHOLD
			&& move != killer[2 * pos.GetPliesFromRoot()].move && move != killer[2 * pos.GetPliesFromRoot() + 1].move && pos.IsQuietAndNoCastles(move);
		if (prunable) {
			//if (depth <= 3 && pos.GetPreviousMovingPiece() != BLANK && History.getValue(pos.GetPieceOnSquare(from(move)), to(move)) < VALUE_ZERO
			//	&& cmHistory.getValue(pos.GetPreviousMovingPiece(), to(pos.GetLastAppliedMove()), pos.GetPieceOnSquare(from(move)), to(move)) < VALUE_ZERO) continue;
			// late-move pruning
			if (depth <= 3 && moveIndex >= depth * 4) {
				moveIndex++;
				continue;
			}
			//SEE pruning
			if (depth <= 3 && pos.SEE_Sign(move) < 0) {
				moveIndex++;
				continue;
			}
			//}
		}
		position next(pos);
		if (next.ApplyMove(move)) {
			//Check extension
			int extension = (next.Checked() && pos.SEE_Sign(move) >= VALUE_ZERO) ? 1 : 0;
			if (!extension && to(move) == recaptureSquare
				//&& (PieceValuesMG[next.GetPieceOnSquare(to(move))] < PieceValuesMG[pos.GetPieceOnSquare(to(move))])
				) {
				++extension;
			}
			int reduction = 0;
			//LMR: Late move reduction
			if (lmr && !critical && moveIndex >= 2 && !extension && !next.Checked()) {
				if (PVNode) {
					if (moveIndex >= 5) reduction = 1;
				}
				else {
					if (moveIndex >= 5) reduction = depth / 3; else reduction = 1;
				}
			}
			if (ZWS) {
				score = -Search<false>(Value(-alpha - 1), -alpha, next, depth - 1 - reduction + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
				if (score > alpha && score < beta) score = -Search<PVNode>(-beta, -alpha, next, depth - 1 + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			}
			else {
				score = -Search<PVNode>(-beta, -alpha, next, depth - 1 - reduction + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
				if (score > alpha && reduction > 0) score = -Search<PVNode>(-beta, -alpha, next, depth - 1 + extension, &subpv[0], !next.GetMaterialTableEntry()->SkipPruning());
			}

			if (score >= beta) {
				updateCutoffStats(move, depth, pos, moveIndex);
				if (T != SINGLE)  ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(score, pos.GetPliesFromRoot()), tt::LOWER_BOUND, depth, move, staticEvaluation);
				return score;
			}
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha)
				{
					ZWS = true;
					nodeType = tt::EXACT;
					alpha = score;
					pv[0] = move;
					memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
				}
			}
		}
		moveIndex++;
	}
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), tt::toTT(bestScore, pos.GetPliesFromRoot()), nodeType, depth, pv[0], staticEvaluation);
	return bestScore;
}

template<ThreadType T> template<bool WithChecks> Value search<T>::QSearch(Value alpha, Value beta, position &pos, int depth) {
	++QNodeCount;
	if (!WithChecks) {
		MaxDepth = std::max(MaxDepth, pos.GetPliesFromRoot());
		++NodeCount;
		if (T == MASTER) {
			if (!Stop && ((NodeCount & MASK_TIME_CHECK) == 0 && timeManager.ExitSearch(NodeCount))) Stop.store(true);
		}
		if (Stopped()) return VALUE_ZERO;
		if (pos.GetResult() != OPEN)  return pos.evaluateFinalPosition();
	}
	//TT lookup
	tt::Entry ttEntry;
	bool ttFound;
	tt::Entry* ttPointer;
	ttPointer = (T == SINGLE) ? tt::probe<tt::UNSAFE>(pos.GetHash(), ttFound, ttEntry) : tt::probe<tt::THREAD_SAFE>(pos.GetHash(), ttFound, ttEntry);
	Value ttValue = ttFound ? tt::fromTT(ttEntry.value(), pos.GetPliesFromRoot()) : VALUE_NOTYETDETERMINED;
	if (ttFound
		&& ttEntry.depth() >= depth
		&& ttValue != VALUE_NOTYETDETERMINED
		&& ((ttEntry.type() == tt::EXACT) || (ttValue >= beta && ttEntry.type() == tt::LOWER_BOUND) || (ttValue <= alpha && ttEntry.type() == tt::UPPER_BOUND))) {
		return ttValue;
	}
	bool checked = pos.Checked();
	int ttDepth = WithChecks || checked ? 0 : -1;
	Value standPat;
	if (checked) {
		standPat = VALUE_NOTYETDETERMINED;
	}
	else {
		standPat = ttFound && ttEntry.evalValue() != VALUE_NOTYETDETERMINED ? ttEntry.evalValue() : pos.evaluate() + BONUS_TEMPO;
		//check if ttValue is better
		if (ttFound && ttValue != VALUE_NOTYETDETERMINED && ((ttValue > standPat && ttEntry.type() == tt::LOWER_BOUND) || (ttValue < standPat && ttEntry.type() == tt::UPPER_BOUND))) standPat = ttValue;
		if (standPat >= beta) {
			if (T == SINGLE) ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, MOVE_NONE, standPat);
			else ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, MOVE_NONE, standPat);
			return beta;
		}
		//Delta Pruning
		if (!pos.GetMaterialTableEntry()->IsLateEndgame()) {
			Value delta = PieceValuesEG[pos.GetMostValuableAttackedPieceType()] + int(pos.PawnOn7thRank()) * (PieceValuesEG[QUEEN] - PieceValuesEG[PAWN]) + DELTA_PRUNING_SAFETY_MARGIN;
			if (standPat + delta < alpha) return alpha;
		}
		if (alpha < standPat) alpha = standPat;
	}
	if (WithChecks) pos.InitializeMoveIterator<QSEARCH_WITH_CHECKS>(&History, &cmHistory, nullptr, MOVE_NONE);
	else pos.InitializeMoveIterator<QSEARCH>(&History, &cmHistory, nullptr, MOVE_NONE);
	Move move;
	Value score;
	tt::NodeType nt = tt::UPPER_BOUND;
	while ((move = pos.NextMove())) {
		if (pos.SEE_Sign(move) < VALUE_ZERO) continue;
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<false>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, move, standPat);
				else ttPointer->update<tt::UNSAFE>(pos.GetHash(), beta, tt::LOWER_BOUND, ttDepth, move, standPat);
				return beta;
			}
			if (score > alpha) {
				nt = tt::EXACT;
				alpha = score;
			}
		}
	}
	if (T != SINGLE) ttPointer->update<tt::THREAD_SAFE>(pos.GetHash(), alpha, nt, ttDepth, MOVE_NONE, standPat);
	else ttPointer->update<tt::UNSAFE>(pos.GetHash(), alpha, nt, ttDepth, MOVE_NONE, standPat);
	return alpha;
}

template<ThreadType T> void search<T>::updateCutoffStats(const Move cutoffMove, int depth, position &pos, int moveIndex) {
	if (moveIndex >= 0) {
		cutoffMoveIndexSum += moveIndex;
		cutoffCount++;
		cutoffAt1stMove += moveIndex == 0;
	}
	if (moveIndex < 0 || pos.GetMoveGenerationType() >= QUIETS_POSITIVE) {
		Square toSquare = to(cutoffMove);
		Piece movingPiece = pos.GetPieceOnSquare(from(cutoffMove));
		if (moveIndex >= 0) {
			ExtendedMove killerMove(movingPiece, cutoffMove);
			int pfr = pos.GetPliesFromRoot();
			killer[2 * pfr + 1] = killer[2 * pfr];
			killer[2 * pfr] = killerMove;
		}
		Value v = Value(depth * depth);
		History.update(v, movingPiece, toSquare);
		Piece prevPiece = BLANK;
		Square prevTo = OUTSIDE;
		Move lastApplied;
		if ((lastApplied = FixCastlingMove(pos.GetLastAppliedMove()))) {
			prevTo = to(lastApplied);
			prevPiece = pos.GetPieceOnSquare(prevTo);
			counterMove[(pos.GetPieceOnSquare(prevTo) << 6) + prevTo] = cutoffMove;
			cmHistory.update(v, prevPiece, prevTo, movingPiece, toSquare);
		}
		if (moveIndex > 0) {
			ValuatedMove * alreadyProcessedQuiets = pos.GetMovesOfCurrentPhase();
			for (int i = 0; i < pos.GetMoveNumberInPhase() - 1; ++i) {
				Move alreadyProcessedMove = FixCastlingMove(alreadyProcessedQuiets->move);
				movingPiece = pos.GetPieceOnSquare(from(alreadyProcessedMove));
				toSquare = to(alreadyProcessedMove);
				History.update(-v, movingPiece, toSquare);
				if (pos.GetLastAppliedMove())
					cmHistory.update(-v, prevPiece, prevTo, movingPiece, toSquare);
				alreadyProcessedQuiets++;
			}
		}
	}
}

template<ThreadType T> bool search<T>::isQuiet(position &pos) {
	Value evaluationDiff = pos.GetStaticEval() - QSearch<true>(-VALUE_MATE, VALUE_MATE, pos, 0);
	return std::abs(evaluationDiff) <= 30;
}