#include "rodent.h"
#include "timer.h"

sTimer Timer;        // setting and observing time limits

int main()
{
  Init();
  InitEval();
  UciLoop();
  return 0;
}
