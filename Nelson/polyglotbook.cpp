#include "polyglotbook.h"
#include "position.h"

namespace polyglot {

	// A Polyglot book is a series of "entries" of 16 bytes. All integers are
	// stored in big-endian format, with highest byte first (regardless of size).
	// The entries are ordered according to the key in ascending order.
	struct Entry {
		uint64_t key;
		uint16_t move;
		uint16_t count;
		uint32_t learn;
	};

	polyglotbook::polyglotbook() : rkiss(now() & 0xFFFF)
	{

	}


	polyglotbook::~polyglotbook()
	{
		if (is_open()) close();
	}

	/// operator>>() reads sizeof(T) chars from the file's binary byte stream and
	/// converts them in a number of type T. A Polyglot book stores numbers in
	/// big-endian format.

	template<typename T> polyglotbook& polyglotbook::operator>>(T& n) {

		n = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
			n = T((n << 8) + std::ifstream::get());

		return *this;
	}

	template<> polyglotbook& polyglotbook::operator>>(Entry& e) {
		return *this >> e.key >> e.move >> e.count >> e.learn;
	}


	/// open() tries to open a book file with the given name after closing any
	/// exsisting one.

	bool polyglotbook::open(const char* fName) {

		if (is_open()) // Cannot close an already closed file
			close();

		std::ifstream::open(fName, std::ifstream::in | std::ifstream::binary);

		fileName = is_open() ? fName : "";
		std::ifstream::clear(); // Reset any error flag to allow retry ifstream::open()
		return !fileName.empty();
	}

	/// probe() tries to find a book move for the given position. If no move is
	/// found returns MOVE_NONE. If pickBest is true returns always the highest
	/// rated move, otherwise randomly chooses one, based on the move score.

	Move polyglotbook::probe(position& pos, const std::string& fName, bool pickBest, ValuatedMove * moves, int moveCount) {

		if (fileName != fName && !open(fName.c_str()))
			return MOVE_NONE;

		Entry e;
		uint16_t best = 0;
		unsigned sum = 0;
		Move move = MOVE_NONE;
		uint64_t key = pos.GetHash();

		seekg(find_first(key) * sizeof(Entry), ios_base::beg);

		while (*this >> e, e.key == key && good())
		{
			best = std::max(best, e.count);
			sum += e.count;

			// Choose book move according to its score. If a move has a very
			// high score it has higher probability to be choosen than a move
			// with lower score. Note that first entry is always chosen.
			if ((!pickBest && sum && rkiss.rand<unsigned>() % sum < e.count)
				|| (pickBest && e.count == best))
				move = Move(e.move);
		}

		if (!move)
			return MOVE_NONE;

		// A PolyGlot book move is encoded as follows:
		//
		// bit  0- 5: destination square (from 0 to 63)
		// bit  6-11: origin square (from 0 to 63)
		// bit 12-14: promotion piece (from KNIGHT == 1 to QUEEN == 4)
		//
		// Castling moves follow "king captures rook" representation. So in case book
		// move is a promotion we have to convert to our representation, in all the
		// other cases we can directly compare with a Move after having masked out
		// the special Move's flags (bit 14-15) that are not supported by PolyGlot.
		int pt = (move >> 12) & 7;
		if (pt)
			move = createMove<PROMOTION>(from(move), to(move), PieceType(4 - pt));
		else {
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
		move &= 0x3FFF;
		for (int i = 0; i < moveCount; i++) {
			if ((moves[i].move & 0x3FFF) == move) return moves[i].move;
		}
		return MOVE_NONE;
	}


	/// find_first() takes a book key as input, and does a binary search through
	/// the book file for the given key. Returns the index of the leftmost book
	/// entry with the same key as the input.

	size_t polyglotbook::find_first(uint64_t key) {

		seekg(0, std::ios::end); // Move pointer to end, so tellg() gets file's size

		size_t low = 0, mid, high = (size_t)tellg() / sizeof(Entry) - 1;
		Entry e;

		_ASSERT(low <= high);

		while (low < high && good())
		{
			mid = (low + high) / 2;

			_ASSERT(mid >= low && mid < high);

			seekg(mid * sizeof(Entry), ios_base::beg);
			*this >> e;

			if (key <= e.key)
				high = mid;
			else
				low = mid + 1;
		}

		_ASSERT(low == high);

		return low;
	}

}
