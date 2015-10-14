#include <fstream>
#include <vector>

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
		if (text.is_open())
		{
			char result = '*';
			while (getline(text, line)) {
				if (line.size() == 0) continue;
				else if (!line.compare(0, 7, "[Result")) {
					if (!line.compare(0, 14, "[Result \"1-0\"]")) result = '+';
					else if (!line.compare(0, 14, "[Result \"0-1\"]")) result = '-';
					else if (!line.compare(0, 18, "[Result \"1/2-1/2\"]")) result = '=';
				}
				else {
					std::vector<std::string> tokens = split(line);
					position pos;
					pos.ResetPliesFromRoot();
					PawnKey_t storedPawnHash = 0;
					for (int i = 0; i < tokens.size(); ++i) {
						position next(pos);
						next.ApplyMove(parseMoveInUCINotation(tokens[i], next));
						pos = next;
						//Only consider quiet positions with equal material after move 10				
						if (i<20 || storedPawnHash == next.GetPawnKey() || next.Checked() || std::abs(next.GetMaterialScore()) > 10 || !srch->isQuiet(next)) continue;						
						output << next.fen() << ";" << result << ";" << pos.PieceBB(PAWN, WHITE) << ";" << pos.PieceBB(PAWN, BLACK)  << std::endl;
						storedPawnHash = next.GetPawnKey();
					}
				}
			}
			text.close();
			output.close();
			delete srch;
		}
	}

}
