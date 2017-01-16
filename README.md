# README #

Nemorino is a UCI chess engine.

### Features ###

* Syzygy Tablebase support
* Pondering
* Standard Chess and Chess960
* Multi Core support (Very Lazy SMP)
* Multi PV analysis mode
* CECP support (experimental - all testing is done using UCI)
##### UCI parameters: #####
- **UCI_Chess960:**     If set to true engine will play according to Chess960 rules (default false)
- **Hash:**             Size in MBytes used for the transposition table (default is 32. Nemorino will use additional memory for other fixed-size hash tables)
- **Clear Hash:**       Will clear the transposition table
- **MultiPV:**          Number of principal variations shown (default 1)
- **Threads:**          Maximum number of threads (cores) used (default 1)
- **Ponder:**           When set to true, engine will continue analysis while waiting for opponent's move (default false)
- **Contempt:**         Score (in centipawns) for accepting draws. If Contempt > 0 then the engine will avoid draws (default 0)
- **BookFile:**         Polyglot book file the engine shall use (default "book.bin"). Nemorino doesn't have an own book so far
- **OwnBook:**          Use Polyglot book, specified by book file (default false)
- **SyzygyPath:**       Path(es) to directories Syzygy tablebase files can be found. Seperate multiple directories by ';' on windows and by ':' on linux
- **SyzygyProbeDepth:** Minimum search depth for probing Syzygy Bases. The lower the number, more tablebase lookups will be done, but search speed will decrease (default 8)
- **MoveOverhead:**     Safety Time(in ms) needed to avoid time-losses (default 100)
- **UCI_Opponent:**     With this command the GUI can send the name, title, elo and if the engine is playing a human or computer to the engine (see [UCI Specification](http://wbec-ridderkerk.nl/html/UCIProtocol.html))


### Installation ###
Released executables can be downloaded in the **[Download section](https://bitbucket.org/christian_g_nther/nemorino/downloads)**

The current development versions are also available:

* [Main Nemorino dev version](https://s3.eu-central-1.amazonaws.com/nemorinotest/appveyor/nemorino_Release.zip)
* [Nemorino dev version for old PC without POPCNT support](https://s3.eu-central-1.amazonaws.com/nemorinotest/appveyor/nemorino_ReleaseNoPopcount.zip)

Nemorino doesn't bring it's own UI. So for running it, you need a GUI supporting UCI (like [Arena](http://www.playwitharena.com/)).  
The engine requires 64-bit Windows.

### Internals ###

* Bitboard based (using magic bitboards for sliding move generation)
* Principal Variation Search (with Aspiration Windows, Null-Move Pruning, Killermoves, Futility Pruning, Razoring, ...)
* Tapered Eval based on Material, Mobility, Threats, King Safety, and Pawn Structure
* Special evaluation functions for some endgames
* Copy/Make
* Syzygy Tablebases accessed via [Fathom](https://github.com/basil00/Fathom)

### Version History ###

**1.01:**

*          Bugfix: rare crashes caused by hash collisions in transposition table
*          Code changes for non-popcount compiles

**1.02:**

*          New Parameters: SyzygyProbeDepth and MoveOverhead
*          Bugfix: Handling of EP-Squares in TB probing code


### License ###

[GNU GENERAL PUBLIC LICENSE Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html)

### Strength ###

Nemorino is listed in several rating lists:

* [CCRL 40/40 **Elo 2909 #46**](http://www.computerchess.org.uk/ccrl/4040/cgi/engine_details.cgi?print=Details&each_game=1&eng=Nemorino%201.02%2064-bit#Nemorino_1_02_64-bit)
* [CCRL 40/4 ` `**Elo 2928 #43**](http://www.computerchess.org.uk/ccrl/404/cgi/engine_details.cgi?match_length=30&each_game=1&print=Details&each_game=1&eng=Nemorino%201.02%2064-bit#Nemorino_1_02_64-bit)
* [CEGT 40/4 ` `**Elo 2763 #40**](http://www.husvankempen.de/nunn/40_4_Ratinglist/40_4_single/484.html)
* [CEGT 40/20 **Elo 2782 #32**](http://www.husvankempen.de/nunn/40_40%20Rating%20List/40_40%20SingleVersion/337.html)
* [FCP 40/10 ` `**Elo 2796 #29**](http://www.amateurschach.de/fcp-rating-list.txt)

### Remarks ###

I wrote this engine because I wanted to understand, how a chess engine is working. And the best way to learn is to write an engine from scratch by yourself. I first started with C#, but after some time I got the ambition to learn C++.
Therefore don't expect a lot of new ideas within my code, and neither expect clean and well-structured code. Instead you will find a unique combination of all those ideas explained in the [Chess Programming Wiki](https://chessprogramming.wikispaces.com).

### Acknowledgements ###

These are my sources of information I used for my engine:

* My main source of information: [Chess Programming Wiki](https://chessprogramming.wikispaces.com)
* [Computer Chess Club Forums](http://talkchess.com/forum/index.php)
* The code of several Open Source Chess Engines, mainly:
    * [Stockfish](http://stockfishchess.org/) - I copied board and move representation and some lines of code from Stockfish (marked by source code comments)
    * [Hakkapeliitta](https://github.com/mAarnos/Hakkapeliitta) 
    * [Lozza](http://op12no2.me/toys/lozza/)
    * [Senpai](http://www.chessprogramming.net/senpai/)
* The blogs of [Steve Maughan](http://www.chessprogramming.net/), [Thomas Petzke](http://macechess.blogspot.de/) and [Jonatan Pettersson](http://mediocrechess.blogspot.de/)