EXE = nemorino

FILES = bbEndings.cpp board.cpp book.cpp evaluation.cpp hashtables.cpp Material.cpp \
    Nemorino.cpp position.cpp search.cpp settings.cpp tablebase.cpp test.cpp timemanager.cpp \
    uci.cpp utils.cpp xboard.cpp syzygy/tbprobe.c

FLAGS = -O3 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -std=c++11 -pthread -DNDEBUG

make: $(FILES)
	g++ $(FLAGS) $(FILES) -o $(EXE)
