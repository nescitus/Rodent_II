#include "rodent.h"
#include "timer.h"

sTimer Timer;        // setting and observing time limits

int main()
{
  Init();
  InitEval();
  InitSearch();
  UciLoop();
  return 0;
}
