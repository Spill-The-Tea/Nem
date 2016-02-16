#include "book.h"
#include <vector>
#include <cstdlib>
#include <time.h>
#include <string>
#include <algorithm>
#include <iostream>
#ifdef NBF
#include <intrin.h>
#endif
#include "utils.h"
#include "pgn.h"


namespace polyglot {

	book::book() {
		this->open(fileName, std::ifstream::in | std::ifstream::binary);
		this->seekg(0, std::ios::end);
		count = size_t(this->tellg() / sizeof(Entry));
		srand(uint32_t(time(NULL)));;
	}

	book::book(const std::string& filename)
	{
		fileName = filename;
		this->open(fileName, std::ifstream::in | std::ifstream::binary);
		this->seekg(0, std::ios::end);
		count = size_t(this->tellg() / sizeof(Entry));
		srand(uint32_t(time(NULL)));
	}


	book::~book()
	{
		if (is_open()) close();
	}

	Entry book::read() {
		Entry result;
		result.key = 0;
		result.move = 0;
		result.weight = 0;
		for (int i = 0; i < 8; ++i) result.key = (result.key << 8) + std::ifstream::get();
		for (int i = 0; i < 2; ++i) result.move = (result.move << 8) + std::ifstream::get();
		for (int i = 0; i < 2; ++i) result.weight = (result.weight << 8) + std::ifstream::get();
		return result;
	}

	Move book::probe(position& pos, bool pickBest, ValuatedMove * moves, int moveCount) {
		if (!is_open()) return MOVE_NONE;
		//Make a binary search to find the right Entry
		size_t low = 0;
		size_t high = count - 1;
		Entry entry;
		size_t searchPoint = 0;
		while (low < high && good()) {
			searchPoint = low + (high - low) / 2;
			seekg(searchPoint * sizeof(Entry), ios_base::beg);
			entry = read();
			if (pos.GetHash() == entry.key) break;
			else if (pos.GetHash() < entry.key) high = searchPoint; else low = searchPoint + 1;
		}
#pragma warning(suppress: 6001)
		if (entry.key != pos.GetHash()) return MOVE_NONE;
		std::vector<Entry> entries;
		entries.push_back(entry);
		size_t searchPointStore = searchPoint;
		while (searchPoint > 0 && good()) {
			searchPoint--;
			seekg(searchPoint * sizeof(Entry), ios_base::beg);
			entry = read();
			if (pos.GetHash() != entry.key) break; else entries.push_back(entry);
		}
		searchPoint = searchPointStore;
		while (searchPoint < (count - 1) && good()) {
			searchPoint++;
			seekg(searchPoint * sizeof(Entry), ios_base::beg);
			entry = read();
			if (pos.GetHash() != entry.key) break; else entries.push_back(entry);
		}
		Entry best = entries[0];
		uint32_t sum = best.weight;
		for (size_t i = 1; i < entries.size(); ++i) {
			Entry e = entries[i];
			if (e.weight > best.weight) best = e;
			sum += e.weight;
		}
		Move move = best.move;
		if (!pickBest && entries.size() > 0) {
			uint32_t indx = rand() % sum;
			sum = 0;
			for (size_t i = 1; i < entries.size(); ++i) {
				Entry e = entries[i];
				sum += e.weight;
				if (sum > indx) {
					move = e.move;
					break;
				}
			}
		}
		//Now move contains move in polyglot representation
		int pt = (move >> 12) & 7;
		if (pt)
			move = createMove<PROMOTION>(from(move), to(move), PieceType(4 - pt));
		else {
			//Castling is stored as king captures rook
			if (pos.GetSideToMove() == WHITE && from(move) == InitialKingSquare[WHITE]) {
				if (to(move) == InitialRookSquare[0] && pos.CastlingAllowed(W0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[WHITE], G1);
				}
				else if (to(move) == InitialRookSquare[1] && pos.CastlingAllowed(W0_0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[WHITE], C1);
				}
			}
			else if (pos.GetSideToMove() == BLACK && from(move) == InitialKingSquare[BLACK]) {
				if (to(move) == InitialRookSquare[2] && pos.CastlingAllowed(B0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[BLACK], G8);
				}
				else if (to(move) == InitialRookSquare[3] && pos.CastlingAllowed(B0_0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[BLACK], C8);
				}
			}
		}
		//Now check if polyglot move is part of generated move list
		move &= 0x3FFF;
		for (int i = 0; i < moveCount; i++) {
			if ((moves[i].move & 0x3FFF) == move) return moves[i].move;
		}
		return MOVE_NONE;
	}

}

#ifdef NBF
namespace positionbook {

	book::book() {
		this->open(fileName, std::ifstream::in | std::ifstream::binary);
		this->seekg(0, std::ios::end);
		count = size_t(this->tellg() / Entry::size);
		srand(uint32_t(time(NULL)));;
	}

	book::book(const std::string& filename)
	{
		fileName = filename;
		this->open(fileName, std::ifstream::in | std::ifstream::binary);
		this->seekg(0, std::ios::end);
		count = size_t(this->tellg() / Entry::size);
		srand(uint32_t(time(NULL)));
	}


	book::~book()
	{
		if (is_open()) close();
	}

	Entry book::read() {
		Entry result;
		result.key = 0;
		result.move = MOVE_NONE;
		for (int i = 0; i < 8; ++i)
			result.key = (result.key << 8) + std::ifstream::get();
		for (int i = 0; i < 2; ++i) result.move = (result.move << 8) + std::ifstream::get();
		result.depth = std::ifstream::get();
		result.score = std::ifstream::get();
		return result;
	}

	ValuatedMove book::probe(position & pos, ValuatedMove * moves, int moveCount, uint8_t minDepth)
	{
		ValuatedMove result;
		result.move = MOVE_NONE;
		result.score = VALUE_ZERO;
		if (!is_open()) {
			return result;
		}
		//Make a binary search to find the right Entry
		size_t low = 0;
		size_t high = count - 1;
		Entry entry;
		size_t searchPoint = 0;
		uint64_t key_low = 0;
		uint64_t key_high = UINT64_MAX;
		searchPoint = size_t(count * 1.0 * pos.GetHash() / UINT64_MAX) ;
		low = searchPoint; high = searchPoint;
		seekg(searchPoint * Entry::size, ios_base::beg);
		entry = read();
		if (entry.key < pos.GetHash()) {
			while (entry.key < pos.GetHash()) {
				high += size_t(2 * (pos.GetHash() - entry.key) * 1.0 * count / UINT64_MAX);
				seekg(high * Entry::size, ios_base::beg);
				entry = read();
			}
		}
		else {
			while (entry.key > pos.GetHash()) {
				low -= size_t(2 * (entry.key - pos.GetHash()) * 1.0 * count / UINT64_MAX);
				seekg(low * Entry::size, ios_base::beg);
				entry = read();
			}
		}
		while (low < high && good()) {
			searchPoint = low + (high - low) / 2;
			seekg(searchPoint * Entry::size, ios_base::beg);
			entry = read();
			if (pos.GetHash() == entry.key) break;
			else if (pos.GetHash() < entry.key) high = searchPoint; else low = searchPoint + 1;
		}
		if (entry.key != pos.GetHash()) return result;
		//Check if move is part of generated move list
		for (int i = 0; i < moveCount; i++) {
			if (moves[i].move == entry.move) {
				result.move = entry.move;
				result.score = entry.getScore();
				break;
			}
		}
		return result;
	}

	void createBookFile(std::string epdFile)
	{
		std::ifstream file(epdFile);
		std::string line;
		std::vector<Entry> entries;
		while (std::getline(file, line))
		{
			line = utils::Trim(line);
			size_t indx = 0;
			int count = 0;
			while (indx != std::string::npos && count < 4) {
				indx = line.find(' ', indx + 1);
				count++;
			}
			if (count != 4) continue;
			position pos(line.substr(0, indx));
			Entry entry;
			entry.key = pos.GetHash();
			std::vector<std::string> opcodeTokens = utils::split(line.substr(indx + 1), ';');
			for (auto opcode : opcodeTokens) {
				opcode = utils::Trim(opcode);
				indx = opcode.find(' ');
				if (indx == std::string::npos) continue;
				std::string opKey = opcode.substr(0, indx);
				std::string value = opcode.substr(indx + 1);
				if (!opKey.compare("acd")) entry.depth = uint8_t(stoi(value));
				else if (!opKey.compare("bm")) entry.move = pgn::parseSANMove(value, pos);
				else if (!opKey.compare("ce")) entry.setScore(Value(stoi(value)));
			}
			entries.push_back(entry);
			if ((entries.size() & ((1 << 13) - 1)) == 0) {
				std::cout << '.';
				if ((entries.size() & ((1 << 19) - 1)) == 0) {
					std::cout << std::endl << entries.size() << std::endl;
				}
			}
		}
		file.close();
		std::cout << std::endl << "Finished reading Input file: " << entries.size() << " Entries" << std::endl;
		std::sort(entries.begin(), entries.end());
		std::cout << "Sorting done" << std::endl;
		std::cout << "Remove duplicates" << std::endl;
		uint64_t pkey = 0;
		for (auto& entry : entries) {
			if (entry.key == pkey) entry.key = 0; else pkey = entry.key;
		}
		std::cout << "Removing duplicates done" << std::endl;
		std::string pboFile;
		pboFile = epdFile;
		utils::replaceExt(pboFile, "nbf");
		std::cout << "Starting to write " << pboFile << std::endl;
		std::ofstream ofile(pboFile, std::ofstream::out | std::ofstream::binary);
		uint32_t count = 0;
		for (auto entry : entries) {
			if (entry.key == 0) continue;
			byte bites[12];
			for (int i = 0; i < 8; ++i) {
				bites[i] = (entry.key >> ((7-i)*8)) & 0xFF;
			}
			bites[8] = entry.move >> 8;
			bites[9] = entry.move & 0xFF;
			bites[10] = entry.depth;
			bites[11] = entry.score;
			ofile.write((const char*)bites, 12);
			++count;
			if ((count & ((1 << 13) - 1))== 0) {
				std::cout << '.';
				if ((count & ((1 << 19) - 1))==0) {
					std::cout << std::endl << count << std::endl;
				}
			}
		}
		ofile.close();
		std::cout << "Done " << pboFile << std::endl;
	}

}
#endif
