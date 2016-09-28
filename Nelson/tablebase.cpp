#include "tablebase.h"

namespace Tablebases {

	int MaxCardinality = 0;

	void init(const std::string& path) {
		tb_init(path.c_str());
		MaxCardinality = TB_LARGEST;
	}

	int probe_wdl(position& pos, int *success) {
		unsigned int result = tb_probe_wdl(
			pos.ColorBB(WHITE),
			pos.ColorBB(BLACK),
			pos.PieceTypeBB(KING),
			pos.PieceTypeBB(QUEEN),
			pos.PieceTypeBB(ROOK),
			pos.PieceTypeBB(BISHOP),
			pos.PieceTypeBB(KNIGHT),
			pos.PieceTypeBB(PAWN),
			pos.GetDrawPlyCount(),
			pos.GetCastles(),
			(unsigned int)(pos.GetEPSquare() & 63),
			pos.GetSideToMove() == WHITE);
		*success = int(result != TB_RESULT_FAILED);
		return (int)result - 2;
	}

	const Value SCORES[5] = { -VALUE_KNOWN_WIN, VALUE_DRAW - 1, VALUE_DRAW, VALUE_DRAW + 1, VALUE_KNOWN_WIN };

	void getResultMove(std::vector<ValuatedMove>& rootMoves, unsigned int result) {
		int from = TB_GET_FROM(result);
		int to = TB_GET_TO(result);
		unsigned promotionPiece = TB_GET_PROMOTES(result);
		Move move;
		if (promotionPiece == TB_PROMOTES_NONE)
			move = createMove(from, to);
		else move = createMove<PROMOTION>(Square(from), Square(to), (PieceType)(promotionPiece - 1));
		for (size_t i = 0; i < rootMoves.size(); ++i) {
			if (rootMoves[i].move == move) {
				rootMoves[0] = rootMoves[i];
				break;
			}
		}
		rootMoves.resize(1);
	}

	unsigned int getBestTBMove(position& pos, std::vector<ValuatedMove>& rootMoves) {
		unsigned int result = tb_probe_root(
			pos.ColorBB(WHITE),
			pos.ColorBB(BLACK),
			pos.PieceTypeBB(KING),
			pos.PieceTypeBB(QUEEN),
			pos.PieceTypeBB(ROOK),
			pos.PieceTypeBB(BISHOP),
			pos.PieceTypeBB(KNIGHT),
			pos.PieceTypeBB(PAWN),
			pos.GetDrawPlyCount(),
			pos.GetCastles(),
			(unsigned int)(pos.GetEPSquare() & 63),
			pos.GetSideToMove() == WHITE,
			nullptr);
		getResultMove(rootMoves, result);
		return result;
	}

	unsigned int getTBMoves(position& pos, std::vector<ValuatedMove>& rootMoves, bool repeated) {
		unsigned int moves[TB_MAX_MOVES];
		unsigned int result = tb_probe_root(
			pos.ColorBB(WHITE),
			pos.ColorBB(BLACK),
			pos.PieceTypeBB(KING),
			pos.PieceTypeBB(QUEEN),
			pos.PieceTypeBB(ROOK),
			pos.PieceTypeBB(BISHOP),
			pos.PieceTypeBB(KNIGHT),
			pos.PieceTypeBB(PAWN),
			pos.GetDrawPlyCount(),
			pos.GetCastles(),
			(unsigned int)(pos.GetEPSquare() & 63),
			pos.GetSideToMove() == WHITE,
			&moves[0]);
		if (result == TB_RESULT_FAILED) return result;
		unsigned dtz = TB_GET_DTZ(result);
		if (dtz > 97) {
			getResultMove(rootMoves, result);
		}
		else {
			unsigned wdl = TB_GET_WDL(result);
			int countGood = 0;
			for (int i = 0; i < TB_MAX_MOVES; ++i) {
				if (moves[i] == TB_RESULT_FAILED) break;
				unsigned wdlMove = TB_GET_WDL(moves[i]);
				unsigned dtzMove = TB_GET_DTZ(moves[i]);
				if (wdlMove >= wdl && dtzMove < 98) {
					//check if move is either decreasing dtz or resetting draw ply counter
					Square fromSquare = Square(TB_GET_FROM(moves[i]));
					Square toSquare = Square(TB_GET_TO(moves[i]));
					unsigned promotionPiece = TB_GET_PROMOTES(moves[i]);
					if (repeated 
						&& GetPieceType(pos.GetPieceOnSquare(fromSquare)) != PieceType::PAWN
						&& pos.GetPieceOnSquare(toSquare) == Piece::BLANK
						&& promotionPiece == TB_PROMOTES_NONE
						&& dtz < dtzMove + 1) continue;
					Move move;
					if (promotionPiece == TB_PROMOTES_NONE)
						move = createMove(fromSquare, toSquare);
					else move = createMove<PROMOTION>(fromSquare, toSquare, (PieceType)(promotionPiece - 1));
					for (size_t j = 0; j < rootMoves.size(); ++j) {
						if (rootMoves[j].move == move) {
							ValuatedMove tmp = rootMoves[countGood];
							rootMoves[countGood] = rootMoves[j];
							rootMoves[j] = tmp;
							countGood++;
							break;
						}
					}
				}
			}
			if (countGood > 0)  rootMoves.resize(countGood);
			else result = TB_RESULT_FAILED;
		}
		return result;
	}

	bool root_probe(position& pos, std::vector<ValuatedMove>& rootMoves, Value& score) {
		//unsigned int result;
		//if (pos.hasRepetition()) {
		//	result = getBestTBMove(pos, rootMoves);
		//} else {
		//	result = getTBMoves(pos, rootMoves);
		//	if (result == TB_RESULT_FAILED) {
		//		result = getBestTBMove(pos, rootMoves);
		//	}
		//}
		//if (result == TB_RESULT_FAILED || rootMoves.size() == 0) return false;
		//else {
		//	score = SCORES[TB_GET_WDL(result)];
		//	return true;
		//}
		unsigned int result;
		bool repeated = pos.hasRepetition();
		result = getTBMoves(pos, rootMoves, repeated);
		if (result == TB_RESULT_FAILED || rootMoves.size() == 0) result = getBestTBMove(pos, rootMoves);
		if (result == TB_RESULT_FAILED || rootMoves.size() == 0) return false;
		else {
			score = SCORES[TB_GET_WDL(result)];
			return true;
		}
	}

}