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
#ifdef NBF
namespace positionbook {
	enum EngineCode { UNKNOWN, STOCKFISH, KOMODO, HOUDINI };
	//A position book is a series of 14 byte entries
	class Entry {
	public:
		uint64_t key = 0;
		uint16_t move = MOVE_NONE;
		int8_t score = 0;
		uint8_t depth = 0;
		const static int size = 12;

		void setScore(Value value) {
			score = int8_t(utils::clamp(value, Value(-507), Value(507))/4);
		}

		Value getScore() {
			return Value(score * 4);
		}

		bool operator>(const Entry& other) const
		{
			if (key != other.key)
			return (key > other.key);
			return depth < other.depth;
		}

		bool operator<(const Entry& other) const
		{
			if (key != other.key)
				return (key < other.key);
			return depth > other.depth;
		}
	};

	void createBookFile(std::string epdFile);

	class book : private std::ifstream
	{
	public:
		book();
		explicit book(const std::string& filename);
		~book();
		ValuatedMove probe(position& pos, ValuatedMove * moves, int moveCount, uint8_t minDepth = 0);
	private:
		std::string fileName = "book.nbf";
		size_t count;
		Entry read();
	};

}
#endif
