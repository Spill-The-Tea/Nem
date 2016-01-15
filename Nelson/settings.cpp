#include "settings.h"

int HashSizeMB = 32;

std::string BOOK_FILE = "book.bin";
bool USE_BOOK = false;
int HelperThreads = 0;
Value Contempt = VALUE_ZERO;
Color EngineSide = WHITE;
Protocol protocol = NO_PROTOCOL;

#ifdef TB
  std::string SYZYGY_PATH = "";
  int SYZYGY_PROBE_DEPTH = 1;
#endif

//Value PASSED_PAWN_BONUS[4] = { Value(10), Value(30), Value(60), Value(100) };
//Value BETA_PRUNING_FACTOR = Value(200);