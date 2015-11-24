#include "rodent.h"
#include "magicmoves.h"
#include "timer.h"
#include "book.h"

sTimer Timer; // class for setting and observing time limits
sBook       MainBook;         // opening book
sBook       GuideBook;

int main() {
  
  eval_blur = 0;
  Timer.Init();
  initmagicmoves();
  Init();
  InitWeights();
  InitEval();
  InitSearch();
  MainBook.bookName = "handmade.bin";
  MainBook.OpenPolyglot();
  GuideBook.bookName = "handmade.bin";
  GuideBook.OpenPolyglot();
  UciLoop();
  MainBook.ClosePolyglot();
  GuideBook.ClosePolyglot();
  return 0;
}
