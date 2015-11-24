#include "rodent.h"
#include "magicmoves.h"
#include "timer.h"
#include "book.h"

sTimer Timer; // class for setting and observing time limits
sBook       Book;         // opening book

int main() {
  
  eval_blur = 0;
  Timer.Init();
  initmagicmoves();
  Init();
  InitWeights();
  InitEval();
  InitSearch();
  Book.bookName = "handmade.bin";
  Book.OpenPolyglot();
  UciLoop();
  Book.ClosePolyglot();
  return 0;
}
