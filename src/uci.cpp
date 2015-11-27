#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rodent.h"
#include "timer.h"
#include "book.h"

void ReadLine(char *str, int n) {
  char *ptr;

  if (fgets(str, n, stdin) == NULL)
    exit(0);
  if ((ptr = strchr(str, '\n')) != NULL)
    *ptr = '\0';
}

char *ParseToken(char *string, char *token) {

  while (*string == ' ')
    string++;
  while (*string != ' ' && *string != '\0')
    *token++ = *string++;
  *token = '\0';
  return string;
}

void UciLoop(void) {

  char command[4096], token[120], *ptr;
  POS p[1];

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  SetPosition(p, START_POS);
  AllocTrans(16);
  for (;;) {
    ReadLine(command, sizeof(command));
    ptr = ParseToken(command, token);
    if (strcmp(token, "uci") == 0) {
      printf("id name Rodent II 0.4.8\n");
      printf("id author Pawel Koziol (based on Sungorus 1.4 by Pablo Vazquez)\n");
      printf("option name Hash type spin default 16 min 1 max 4096\n");
      printf("option name Clear Hash type button\n");
    printf("option name Material type spin default %d min 0 max 500\n", mat_perc);
    printf("option name Attack type spin default %d min 0 max 500\n", weights[F_ATT]);
    printf("option name Mobility type spin default %d min 0 max 500\n", weights[F_MOB]);
    printf("option name KingTropism type spin default %d min 0 max 500\n", weights[F_TROPISM]);
    printf("option name PassedPawns type spin default %d min 0 max 500\n", weights[F_PASSERS]);
    printf("option name PawnStructure type spin default %d min 0 max 500\n", weights[F_PAWNS]);
    printf("option name Lines type spin default %d min 0 max 500\n", weights[F_LINES]);
    printf("option name Outposts type spin default %d min 0 max 500\n", weights[F_OUTPOST]);
    printf("option name NpsLimit type spin default %d min 0 max 5000000\n", Timer.nps_limit);
    printf("option name EvalBlur type spin default %d min 0 max 5000000\n", eval_blur);
  printf("option name GuideBookFile type string default guide.bin\n");
  printf("option name MainBookFile type string default rodent.bin\n");
      printf("uciok\n");
    } else if (strcmp(token, "isready") == 0) {
      printf("readyok\n");
    } else if (strcmp(token, "setoption") == 0) {
      ParseSetoption(ptr);
    } else if (strcmp(token, "position") == 0) {
      ParsePosition(p, ptr);
    } else if (strcmp(token, "perft") == 0) {
      ptr = ParseToken(ptr, token);
    int depth = atoi(token);
    if (depth == 0) depth = 5;
    Timer.SetStartTime();
    nodes = Perft(p, 0, depth);
    printf (" perft %d : %d nodes in %d miliseconds\n", depth, nodes, Timer.GetElapsedTime() );
    } else if (strcmp(token, "print") == 0) {
      PrintBoard(p);
    } else if (strcmp(token, "eval") == 0) {
      PrintEval(p);
    } else if (strcmp(token, "step") == 0) {
      ParseMoves(p, ptr);
    } else if (strcmp(token, "go") == 0) {
      ParseGo(p, ptr);
    } else if (strcmp(token, "bench") == 0) {
      ptr = ParseToken(ptr, token);
      Bench(atoi(token));
    } else if (strcmp(token, "quit") == 0) {
      exit(0);
    }
  }
}

void ParseSetoption(char *ptr) {

  char token[120], name[120], value[120] = "";

  ptr = ParseToken(ptr, token);
  name[0] = '\0';
  for (;;) {
    ptr = ParseToken(ptr, token);
    if (*token == '\0' || strcmp(token, "value") == 0)
      break;
    strcat(name, token);
    strcat(name, " ");
  }
  name[strlen(name) - 1] = '\0';
  if (strcmp(token, "value") == 0) {
    value[0] = '\0';

    for (;;) {
      ptr = ParseToken(ptr, token);
      if (*token == '\0')
        break;
      strcat(value, token);
      strcat(value, " ");
    }
    value[strlen(value) - 1] = '\0';
  }

  if (strcmp(name, "Hash") == 0) {
    AllocTrans(atoi(value));
  } else if (strcmp(name, "Clear Hash") == 0) {
    ResetEngine();
  } else if (strcmp(name, "Material") == 0) {
    mat_perc = atoi(value);
    ResetEngine();
    InitEval();
  } else if (strcmp(name, "Attack") == 0) {
    SetWeight(F_ATT, atoi(value));
  } else if (strcmp(name, "Mobility") == 0) {
    SetWeight(F_MOB, atoi(value));
  } else if (strcmp(name, "KingTropism") == 0) {
    SetWeight(F_TROPISM, atoi(value));
  } else if (strcmp(name, "PassedPawns") == 0) {
    SetWeight(F_PASSERS, atoi(value));
  } else if (strcmp(name, "PawnStructure") == 0) {
    SetWeight(F_PAWNS, atoi(value));
  } else if (strcmp(name, "Lines") == 0) {
   SetWeight(F_LINES, atoi(value));
  } else if (strcmp(name, "Outposts") == 0) {
    SetWeight(F_OUTPOST, atoi(value));
  } else if (strcmp(name, "NpsLimit") == 0) {
    Timer.nps_limit = atoi(value);
   ResetEngine();
  } else if (strcmp(name, "EvalBlur") == 0) {
    eval_blur = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "GuideBookFile") == 0) {
    GuideBook.ClosePolyglot();
    GuideBook.bookName = value;
    GuideBook.OpenPolyglot();
  } else if (strcmp(name, "MainBookFile") == 0) {
    MainBook.ClosePolyglot();
    MainBook.bookName = value;
    MainBook.OpenPolyglot();
  }
}

void SetWeight(int weight_name, int value) {

  weights[weight_name] = value;
  ResetEngine();
}

void ParseMoves(POS *p, char *ptr) {
  
  char token[120];
  UNDO u[1];

  for (;;) {

    // Get next move to parse

    ptr = ParseToken(ptr, token);

  // No more moves!

    if (*token == '\0') break;

    p->DoMove(StrToMove(p, token), u);

  // We won't be taking back moves beyond this point:

    if (p->rev_moves == 0) p->head = 0;
  }
}

void ParsePosition(POS *p, char *ptr) {

  char token[120], fen[120];

  ptr = ParseToken(ptr, token);
  if (strcmp(token, "fen") == 0) {
    fen[0] = '\0';
    for (;;) {
      ptr = ParseToken(ptr, token);

      if (*token == '\0' || strcmp(token, "moves") == 0)
        break;

      strcat(fen, token);
      strcat(fen, " ");
    }
    SetPosition(p, fen);
  } else {
    ptr = ParseToken(ptr, token);
    SetPosition(p, START_POS);
  }

  if (strcmp(token, "moves") == 0)
    ParseMoves(p, ptr);
}

void ParseGo(POS *p, char *ptr) {

  char token[120], bestmove_str[6], ponder_str[6];
  int pv[MAX_PLY];

  Timer.Clear();
  pondering = 0;

  for (;;) {
    ptr = ParseToken(ptr, token);
    if (*token == '\0')
      break;
    if (strcmp(token, "ponder") == 0) {
      pondering = 1;
    } else if (strcmp(token, "wtime") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(W_TIME, atoi(token));
    } else if (strcmp(token, "btime") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(B_TIME, atoi(token));
    } else if (strcmp(token, "winc") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(W_INC, atoi(token));
    } else if (strcmp(token, "binc") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(B_INC, atoi(token));
    } else if (strcmp(token, "movestogo") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(MOVES_TO_GO, atoi(token));
    } else if (strcmp(token, "depth") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(FLAG_INFINITE, 1);
      Timer.SetData(MAX_DEPTH, atoi(token));
    } else if (strcmp(token, "infinite") == 0) {
      Timer.SetData(FLAG_INFINITE, 1);
    }
  }

  Timer.SetSideData(p->side);
  Timer.SetMoveTiming();
  Think(p, pv);
  MoveToStr(pv[0], bestmove_str);
  if (pv[1]) {
    MoveToStr(pv[1], ponder_str);
    printf("bestmove %s ponder %s\n", bestmove_str, ponder_str);
  } else
    printf("bestmove %s\n", bestmove_str);
}

void ResetEngine(void) {

  ClearHist();
  ClearTrans();
  ClearEvalHash();
  ClearPawnHash();
}