#ifdef NOMAD
#include "nomad.h"
#include <thread>
#include <vector>
#include "search.h"
#include "settings.h"

namespace nomad {

	const int ThreadCount = 7;
	std::vector<std::string> packages[ThreadCount];
	double results[ThreadCount];

	void processNomadPackage(int packageIndex, double result) {
		baseSearch * Engine = new search < SINGLE >;
		Engine->UciOutput = false;
		std::vector<std::string>::iterator it;
		double localresult = 0;
		for (it = packages[packageIndex].begin(); it < packages[packageIndex].end(); it++) {
			position pos(*it);
			Value score = (dynamic_cast<search<SINGLE>*>(Engine))->qscore(&pos);
			if (pos.GetSideToMove() == BLACK) score = -score;
			double error = result - utils::sigmoid(score);
			localresult += error * error;
		}
		results[packageIndex] = localresult;
	}

	double nomad::calculateError()
	{
		std::vector<std::thread> threads;
		double totalResult = 0;
		int totalCount = 0;
		if (true) {
			//use WDL data from Nirvanachess
			const std::string params[3] = { settings::OPTION_TEXEL_TUNING_LOSSES, settings::OPTION_TEXEL_TUNING_DRAWS, settings::OPTION_TEXEL_TUNING_WINS };
			for (int i = 0; i < 3; ++i) {
				std::string filename = settings::options.getString(params[i]);
				if (filename.length() <= 0) exit(1);
			}
			for (int i = 0; i < 3; ++i) {
				for (int p = 0; p < ThreadCount; ++p) {
					packages[p].clear();
					results[p] = 0;
				}
				threads.clear();
				double result = 0.5 * i;
				std::string filename = settings::options.getString(params[i]);
				std::ifstream infile(filename);
				int count = 0;
				for (std::string line; getline(infile, line); )
				{
					packages[count % ThreadCount].push_back(line);
					++count;
				}
				totalCount += count;
				for (int t = 0; t < ThreadCount; ++t) {
					threads.push_back(std::thread(processNomadPackage, t, result));
				}
				for (int t = 0; t < ThreadCount; ++t) {
					threads[t].join();
				}
				for (int t = 0; t < ThreadCount; ++t) {
					totalResult += results[t];
				}
			}
		}
		else {
			//use labeled data from Zurichess or Alvaro Bequé
			std::string filename = settings::options.getString(settings::OPTION_TEXEL_TUNING_LABELLED);
			if (filename.length() <= 0) {
				sync_cout << "info string no input data file specified: setoption name TTLabeled value <filename>" << sync_endl;
				return 0;
			}
			std::ifstream infile(filename);
			std::vector<std::string> epds[3];
			for (std::string line; getline(infile, line); )
			{
				std::size_t found = line.find("c9");
				if (found != std::string::npos) {
					//position pos(line.substr(0, found));
					std::string rs = line.substr(found + 3);
					int indx = 2;
					if (!rs.compare("\"1/2-1/2\";")) indx = 1;
					else if (!rs.compare("\"0-1\";")) indx = 0;
					epds[indx].push_back(line.substr(0, found));
				}
			}
			for (int i = 0; i < 3; ++i) {
				for (int p = 0; p < ThreadCount; ++p) {
					packages[p].clear();
					results[p] = 0;
				}
				threads.clear();
				double result = 0.5 * i;
				std::vector<std::string>::iterator it;
				int count = 0;
				for (it = epds[i].begin(); it < epds[i].end(); it++) {
					packages[count % ThreadCount].push_back(*it);
					++count;
				}
				totalCount += count;
				for (int t = 0; t < ThreadCount; ++t) {
					threads.push_back(std::thread(processNomadPackage, t, result));
				}
				for (int t = 0; t < ThreadCount; ++t) {
					threads[t].join();
				}
				for (int t = 0; t < ThreadCount; ++t) {
					totalResult += results[t];
				}
			}
		}
		return totalResult / totalCount;
	}

}

int main(int argc, const char* argv[]) {
	if (argc > 1 && argv[1]) {
		std::ifstream in(argv[1]);
		double p[10];
		for (int i = PieceType::PAWN; i <= PieceType::PAWN; i++) {
			in >> p[2 * i];
			PieceValues[i].mgScore = Value((int)p[2 * i]); 
			in >> p[2 * i + 1];
            PieceValues[i].egScore = Value((int)p[2 * i + 1]);
		}
		Initialize();
		std::cout << nomad::calculateError() << std::endl;
		exit(0);
	}
	else exit(1);
}

#endif