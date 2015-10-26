#include <assert.h>
#include <stdio.h>
#include "rodent.h"
#include "magicmoves.h"
#include "eval.h"

#define PAWN_HASH

sPawnHashEntry PawnTT[EVAL_HASH_SIZE];

void FullPawnEval(POS * p, int use_hash) {

  // Try to retrieve score from pawn hashtable

  int addr = p->pawn_key % PAWN_HASH_SIZE;

  if (PawnTT[addr].key == p->pawn_key && use_hash) {
    mg[WC][F_PAWNS]   = PawnTT[addr].mg_pawns;
    eg[WC][F_PAWNS]   = PawnTT[addr].eg_pawns;
    mg[WC][F_PASSERS] = PawnTT[addr].mg_passers;
    eg[WC][F_PASSERS] = PawnTT[addr].eg_passers;
    return;
  }
	
  EvaluatePawns(p, WC);
  EvaluatePawns(p, BC);
  EvaluateKing(p, WC);
  EvaluateKing(p, BC);

  PawnTT[addr].key = p->pawn_key;
  PawnTT[addr].mg_pawns = mg[WC][F_PAWNS] - mg[BC][F_PAWNS];
  PawnTT[addr].mg_passers = mg[WC][F_PASSERS] - mg[BC][F_PASSERS];
  PawnTT[addr].eg_pawns = eg[WC][F_PAWNS] - eg[BC][F_PAWNS];
  PawnTT[addr].eg_passers = eg[WC][F_PASSERS] - eg[BC][F_PASSERS];
}

void EvaluatePawns(POS *p, int sd) {

  U64 bbPieces, bbSpan;
  int sq, fl_unopposed;
  int op = Opp(sd);

  // Is color OK?

  assert(sd == WC || sd == BC);

  // Loop through the pawns, evaluating each one

  bbPieces = PcBb(p, sd, P);
  while (bbPieces) {
    sq = PopFirstBit(&bbPieces);

    // Get front span

    if (sd == WC) bbSpan = FillNorth(ShiftNorth(SqBb(sq)));
    else          bbSpan = FillSouth(ShiftSouth(SqBb(sq)));
    fl_unopposed = ((bbSpan & PcBb(p, op, P)) == 0);

    // Doubled pawn

    if (bbSpan & PcBb(p, sd, P))
      Add(sd, F_PAWNS, -10, -20);

    // Passed pawn

    if (!(passed_mask[sd][sq] & PcBb(p, Opp(sd), P)))
      Add(sd, F_PASSERS, passed_bonus_mg[sd][Rank(sq)], passed_bonus_eg[sd][Rank(sq)]);

    // Isolated pawn

    if (!(adjacent_mask[File(sq)] & PcBb(p, sd, P))) {
	  Add(sd, F_PAWNS, -10, -20);
	  if (fl_unopposed) Add(sd, F_PAWNS, -10, 0);
    }

    // Backward pawn

    else if ((support_mask[sd][sq] & PcBb(p, sd, P)) == 0) {
      Add(sd, F_PAWNS, -8, -8);
      if (fl_unopposed) Add(sd, F_PAWNS, -8, 0);
    }
  }
}
