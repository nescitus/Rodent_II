#include "rodent.h"
#include "magicmoves.h"
#include "timer.h"

sTimer Timer; // class for setting and observing time limits

int main() {
  initmagicmoves();
  Init();
  InitWeights();
  InitEval();
  InitSearch();
  UciLoop();
  return 0;
}
