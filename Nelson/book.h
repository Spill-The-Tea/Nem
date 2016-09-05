#pragma once
#include <fstream>
#include <string>
#include "types.h"
#include "position.h"
#include "utils.h"

namespace polyglot {

	//A Polyglot book is a series of "entries" of 16 bytes
	//All integers are stored highest byte first(regardless of size)
	//The entries are ordered according to key.Lowest key first.
	struct Entry {
		uint64_t key;
		uint16_t move;
		uint16_t weight;
		uint32_t learn;
	};

	class book : private std::ifstream
	{
	public:
		book();
		explicit book(const std::string& filename);
		~book();

		Move probe(position& pos, bool pickBest, ValuatedMove * moves, int moveCount);

	private:
		std::string fileName = "book.bin";
		size_t count;
		Entry read();
	};

}
