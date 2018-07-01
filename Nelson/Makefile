ifeq ($(OS),Windows_NT)
	EXE := nemorino_gcc.exe
else
	EXE := nemorino
endif

FILES = bbEndings.cpp board.cpp book.cpp evaluation.cpp hashtables.cpp Material.cpp \
    Nemorino.cpp position.cpp search.cpp settings.cpp test.cpp timemanager.cpp \
    uci.cpp utils.cpp xboard.cpp tbprobe.cpp

FLAGS = -O3 -Wmain -std=c++11 -pthread -DNDEBUG

FLAGS_VG = -O1 -g -Wmain -std=c++11 -pthread -DNDEBUG

make: $(FILES)
	g++ $(FLAGS) $(FILES) -o $(EXE)

vg: $(FILES)
	g++ $(FLAGS_VG) $(FILES) -o $(EXE)_vg

vc: $(FILES)
	g++ -g $(FLAGS) $(FILES) -o $(EXE)_vc