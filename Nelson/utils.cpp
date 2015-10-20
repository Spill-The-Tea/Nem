#include <fstream>
#include <vector>
#include <exception>

#include "types.h"
#include "uci.h"
#include "position.h"
#include "search.h"
#include "utils.h"


namespace utils {

	int dispatch(int argc, const char* argv[]) {
		pawnPositionExtractor("C:\\Users\\chrgu_000\\Desktop\\Data\\tools\\CCRL-4040.uci", "C:\\Users\\chrgu_000\\Desktop\\Data\\tools\\CCRL-4040_quiet.fen");
		return 0;
	}

	void pawnPositionExtractor(std::string inputFile, std::string outputFile) {
		search<SINGLE> * srch = new search<SINGLE>();
		std::string line;
		std::ifstream text(inputFile);
		std::ofstream output(outputFile);
		output << "FEN;Result;BBWhite;BBBlack;Phase;" //5
			<< "PPW4;PPW5;PPW6;PPW7;PPB4;PPB5;PPB6;PPB7;PPPW;PPPB;" //10
			<< "CPW2;CPW3;CPW4;CPW5;CPB2;CPB3;CPB4;CPB5;" //8
			<< "ISW;ISB;ICW;ICB;BPW;BPB;DPW;DPB" << std::endl; //8
		if (text.is_open())
		{
			int result = 0;
			while (getline(text, line)) {
				if (line.size() == 0) continue;
				if (line[0] == '[') continue;
				try {
					std::vector<std::string> tokens = split(line);
					Phase_t gamePhase = 0;
					position pos;
					pos.ResetPliesFromRoot();
					PawnKey_t storedPawnHash = 0;
					std::string sresult = tokens.back();
					tokens.pop_back();
					if (!sresult.compare("1-0")) result = 1;
					else if (!sresult.compare("0-1")) result = -1;
					else if (!sresult.compare("1/2-1/2")) result = 0;
					else result = 0;
					for (int i = 0; i < tokens.size(); ++i) {
						position next(pos);
						next.ApplyMove(parseMoveInUCINotation(tokens[i], next));
						pos = next;
						//Only consider quiet positions with equal material after move 10				
						if (i < 20 || storedPawnHash == next.GetPawnKey() || next.Checked() || gamePhase == next.GetMaterialTableEntry()->Phase
							|| std::abs(next.GetMaterialScore()) > 10) continue;
						bool isQuiet = srch->isQuiet(pos);
						if (isQuiet) {
							gamePhase = next.GetMaterialTableEntry()->Phase;
							Bitboard bbWhite = next.PieceBB(PAWN, WHITE);
							Bitboard bbBlack = next.PieceBB(PAWN, BLACK);
							Bitboard bbFilesWhite = FileFill(bbWhite);
							Bitboard bbFilesBlack = FileFill(bbBlack);
							Bitboard attacksWhite = ((bbWhite << 9) & NOT_A_FILE) | ((bbWhite << 7) & NOT_H_FILE);
							Bitboard attacksBlack = ((bbBlack >> 9) & NOT_H_FILE) | ((bbBlack >> 7) & NOT_A_FILE);
							//frontspans
							Bitboard bbWFrontspan = FrontFillNorth(bbWhite);
							Bitboard bbBFrontspan = FrontFillSouth(bbBlack);
							//attacksets
							Bitboard bbWAttackset = FrontFillNorth(attacksWhite);
							Bitboard bbBAttackset = FrontFillSouth(attacksBlack);
							Bitboard ppW = bbWhite & (~(bbBAttackset | bbBFrontspan));
							Bitboard ppB = bbBlack & (~(bbWAttackset | bbWFrontspan));
							int ppw4 = popcount(ppW & RANK4);
							int ppw5 = popcount(ppW & RANK5);
							int ppw6 = popcount(ppW & RANK6);
							int ppw7 = popcount(ppW & RANK7);
							int ppb4 = popcount(ppB & RANK5);
							int ppb5 = popcount(ppB & RANK4);
							int ppb6 = popcount(ppB & RANK3);
							int ppb7 = popcount(ppB & RANK2);
							//bonus for protected passed pawns (on 5th rank or further)
							int pppw = popcount(ppW & attacksWhite & HALF_OF_BLACK);
							int pppb = popcount(ppB & attacksBlack & HALF_OF_WHITE);
							//Candidate passed pawns
							int cpw[4] = { 0,0,0,0 };
							int cpb[4] = { 0,0,0,0 };
							Bitboard candidates = EMPTY;
							Bitboard potentialCandidates = bbWhite & ~bbBFrontspan & bbBAttackset & ~attacksBlack; //open, but not passed pawns
							while (potentialCandidates) {
								Bitboard candidateBB = isolateLSB(potentialCandidates);
								Bitboard sentries = FrontFillNorth(((candidateBB << 17) & NOT_A_FILE) | ((candidateBB << 15) & NOT_H_FILE)) & bbBlack;
								Bitboard helper = FrontFillSouth(sentries >> 16) & bbWhite;
								if (popcount(helper) >= popcount(sentries)) {
									candidates |= candidateBB;
									int rank = (lsb(candidateBB) >> 3) - 1;
									assert(rank >= 0 && rank < 4);
									cpw[rank] += 1;
								}
								potentialCandidates &= potentialCandidates - 1;
							}
							potentialCandidates = bbBlack & ~bbWFrontspan & bbWAttackset & ~attacksWhite; //open, but not passed pawns
							while (potentialCandidates) {
								Bitboard candidateBB = isolateLSB(potentialCandidates);
								Bitboard sentries = FrontFillSouth(((candidateBB >> 15) & NOT_A_FILE) | ((candidateBB >> 17) & NOT_H_FILE)) & bbWhite;
								Bitboard helper = FrontFillNorth(sentries << 16) & bbBlack;
								if (popcount(helper) >= popcount(sentries)) {
									candidates |= candidateBB;
									int rank = (6 - (lsb(candidateBB) >> 3));
									assert(rank >= 0 && rank < 4);
									cpb[rank] += 1;
								}
								potentialCandidates &= potentialCandidates - 1;
							}
							//isolated pawns
							int isw = popcount(IsolatedFiles(bbFilesWhite));
							int isb = popcount(IsolatedFiles(bbFilesBlack));
							//pawn islands
							int icw = popcount(IslandsEastFiles(bbBlack));
							int icb = popcount(IslandsEastFiles(bbWhite));
							//backward pawns
							Bitboard bbWBackward = bbWhite & ~bbBFrontspan; //Backward pawns are open pawns
							Bitboard frontspan = FrontFillNorth(bbWBackward << 8);
							Bitboard stopSquares = frontspan & attacksBlack; //Where the advancement is stopped by an opposite pawn
							stopSquares &= ~bbWAttackset; //and the stop square isn't part of the own pawn attack
							bbWBackward &= FrontFillSouth(stopSquares);
							Bitboard bbBBackward = bbBlack & ~bbWFrontspan; //Backward pawns are open pawns
							frontspan = FrontFillSouth(bbBBackward >> 8);
							stopSquares = frontspan & attacksWhite; //Where the advancement is stopped by an opposite pawn
							stopSquares &= ~bbBAttackset; //and the stop square isn't part of the own pawn attack
							bbBBackward &= FrontFillNorth(stopSquares);
							int bpw = popcount(bbWBackward);
							int bpb = popcount(bbBBackward);
							//doubled pawns
							Bitboard doubled = FrontFillNorth(bbWhite << 8) & bbWhite;
							int dpw = popcount(doubled);
							doubled = FrontFillSouth(bbBlack >> 8) & bbBlack;
							int dpb = popcount(doubled);
							output << next.fen() << ";" << result << ";" << pos.PieceBB(PAWN, WHITE) << ";" << pos.PieceBB(PAWN, BLACK) << ";" << gamePhase << ";" //5
								<< ppw4 << ";" << ppw5 << ";" << ppw6 << ";" << ppw7 << ";" << ppb4 << ";" << ppb5 << ";" << ppb6 << ";" << ppb7 << ";"  << pppw << ";" << pppb << ";"; //10
							for (int j = 0; j < 4; ++j) output << cpw[j] << ";";
							for (int j = 0; j < 4; ++j) output << cpb[j] << ";"; //8
							output << isw << ";" << isb << ";" << icw << ";" << icb << ";" << bpw << ";" << bpb << ";" << dpw << ";" << dpb; //8
							output << std::endl;
							storedPawnHash = next.GetPawnKey();
						}
					}
				}
				catch (std::exception& e) {
					std::cout << e.what() << std::endl;
					std::cout << line << std::endl;
				}
			}
			text.close();
			output.close();
			delete srch;
		}
	}

}
