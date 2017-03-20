EXE = nemorino

FILES = bbEndings.cpp board.cpp book.cpp evaluation.cpp hashtables.cpp Material.cpp \
    Nemorino.cpp position.cpp search.cpp settings.cpp tablebase.cpp test.cpp timemanager.cpp \
    uci.cpp utils.cpp xboard.cpp syzygy/tbprobe.c

FLAGS = -O3 -Wmain -std=c++11 -pthread -march=core2 -DNDEBUG -DTB

make: $(FILES)
	g++ $(FLAGS) $(FILES) -o $(EXE)