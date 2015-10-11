#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rodent.h"
#include "timer.h"

void ReadLine(char *str, int n)
{
  char *ptr;

  if (fgets(str, n, stdin) == NULL)
    exit(0);
  if ((ptr = strchr(str, '\n')) != NULL)
    *ptr = '\0';
}

char *ParseToken(char *string, char *token)
{
  while (*string == ' ')
    string++;
  while (*string != ' ' && *string != '\0')
    *token++ = *string++;
  *token = '\0';
  return string;
}

void UciLoop(void)
{
  char command[4096], token[80], *ptr;
  POS p[1];

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  SetPosition(p, START_POS);
  AllocTrans(16);
  for (;;) {
    ReadLine(command, sizeof(command));
    ptr = ParseToken(command, token);
    if (strcmp(token, "uci") == 0) {
      printf("id name Mini Rodent 0.1.10\n");
      printf("id author Pawel Koziol (based on Sungorus 1.4 by Pablo Vazquez)\n");
      printf("option name Hash type spin default 16 min 1 max 4096\n");
      printf("option name Clear Hash type button\n");
      printf("uciok\n");
    } else if (strcmp(token, "isready") == 0) {
      printf("readyok\n");
    } else if (strcmp(token, "setoption") == 0) {
      ParseSetoption(ptr);
    } else if (strcmp(token, "position") == 0) {
      ParsePosition(p, ptr);
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

void ParseSetoption(char *ptr)
{
  char token[80], name[80], value[80] = "";

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
  }
}

void ParsePosition(POS *p, char *ptr)
{
  char token[80], fen[80];
  UNDO u[1];

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
    for (;;) {
      ptr = ParseToken(ptr, token);
      if (*token == '\0')
        break;
      p->DoMove(StrToMove(p, token), u);
      if (p->rev_moves == 0)
        p->head = 0;
    }
}

void ParseGo(POS *p, char *ptr)
{
  char token[80], bestmove_str[6], ponder_str[6];
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

void Bench(int depth)
{
  POS p[1];
  int pv[MAX_PLY];
  char *test[] = {
    "r1bqkbnr/pp1ppppp/2n5/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
    "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
    "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
    "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
    "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
    "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
    "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
    "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
    "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
    "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
    "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
    "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
    "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
    "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
    NULL
  }; // test positions taken from DiscoCheck by Lucas Braesch

  if (depth == 0) depth = 8; // so that you can call bench without parameters

  printf("Bench test started (depth %d): \n", depth);

  ResetEngine();
  nodes = 0;
  Timer.SetData(MAX_DEPTH, depth);
  Timer.SetData(FLAG_INFINITE, 1);
  Timer.SetStartTime();

  for (int i = 0; test[i]; ++i) {
    printf(test[i]);
    SetPosition(p, test[i]);
    printf("\n");
    Iterate(p, pv);
  }

  int end_time = Timer.GetElapsedTime();
  int nps = (nodes * 1000) / (end_time + 1);

  printf("%llu nodes searched in %d, speed %u nps (Score: %.3f)\n", nodes, end_time, nps, (float)nps / 430914.0);
}

void ResetEngine(void) 
{
  ClearHist();
  ClearTrans();
  ClearEvalHash();
}