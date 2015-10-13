#include "rodent.h"
#include "timer.h"

sTimer Timer; // class for setting and observing time limits

int main() {
  Init();
  InitEval();
  InitSearch();
  UciLoop();
  return 0;
}
