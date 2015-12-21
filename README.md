# README #

Nemorino is a UCI chess engine.

### Features ###

* Syzygy Tablebase support
* Pondering
* Standard Chess and Chess960
* Multi Core support (Very Lazy SMP)
* Multi PV analysis mode
* CECP support (not yet 100% stable)
##### UCI parameters: #####
- UCI_Chess960:     If set to true engine will play according to Chess960 rules (default false)
- Hash:             Size in MBytes used for the transposition table (default is 32. Nemorino will use additional memory for other fixed-size hash tables)
- Clear Hash:       Will clear the transposition table
- MultiPV:          Number of principal variations shown (default 1)
- Threads:          Maximum number of threads (cores) used (default 1)
- Ponder:           When set to true, engine will continue analysis while waiting for opponent's move (default false)
- Contempt:         Score (in centipawns) for accepting draws. If Contempt > 0 then the engine will avoid draws (default 0)
- BookFile:         Polyglot book file the engine shall use (default "book.bin"). Nemorino doesn't has an own book so far
- OwnBook:          Use Polyglot book, specified by book file (default false)
- SyzygyPath:       Path(es) to directories Syzygy tablebase files can be found. Seperate multiple directories by ';' on windows and by ':' on linux
- SyzygyProbeDepth: Limits the depth where Syzygy tables are probed during the search (default 0).


### How do I get set up? ###
Nemorino doesn't bring it's own UI. So for running it, you need a GUI supporting UCI (like [Arena](http://www.playwitharena.com/)).  
Nemorino requires either 64-bit windows or linux.
It has been developed and tested on windows. The linux compile has only been smoke-tested.

### Internals ###

* Bitboard based (using magic bitboards for sliding move generation)
* Principal Variation Search (with Aspiration Windows, Null-Move Pruning, Killermoves, Futility Pruning, Razoring, ...)
* 

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact