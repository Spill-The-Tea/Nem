#pragma once
#include <fstream>
#include <string>
#include "types.h"


//Copied from Stockfish
namespace polyglot {

	class RKISS {

		struct S { uint64_t a, b, c, d; } s; // Keep variables always together

		uint64_t rotate(uint64_t x, uint64_t k) const {
			return (x << k) | (x >> (64 - k));
		}

		uint64_t rand64() {

			const uint64_t
				e = s.a - rotate(s.b, 7);
			s.a = s.b ^ rotate(s.c, 13);
			s.b = s.c + rotate(s.d, 37);
			s.c = s.d + e;
			return s.d = e + s.a;
		}

	public:
		RKISS(int seed = 73) {

			s.a = 0xf1ea5eed;
			s.b = s.c = s.d = 0xd4e12c77;
			for (int i = 0; i < seed; ++i) // Scramble a few rounds
				rand64();
		}

		template<typename T> T rand() { return T(rand64()); }
	};

	class polyglotbook : private std::ifstream {
	public:
		polyglotbook();
		~polyglotbook();
		polyglotbook(const polyglotbook &obj);

		Move probe(position& pos, const std::string& fName, bool pickBest, ValuatedMove * moves, int moveCount);

	private:
		template<typename T> polyglotbook& operator>>(T& n);
		bool open(const char* fName);
		size_t find_first(uint64_t key);

		RKISS rkiss;
		std::string fileName;
	};

}