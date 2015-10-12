#include "rodent.h"

int Quiesce(POS *p, int ply, int alpha, int beta, int *pv)
{
  int best, score, move, new_pv[MAX_PLY];
  MOVES m[1];
  UNDO u[1];

  // Statistics and attempt at quick exit

  nodes++;
  Check();
  if (abort_search) return 0;
  *pv = 0;
  if (IsDraw(p)) return 0;
  if (ply >= MAX_PLY - 1) return Evaluate(p);

  // Get a stand-pat score and adjust bounds
  // (exiting if eval exceeds beta)

  best = Evaluate(p); 
  if (best >= beta) return best;
  if (best > alpha) alpha = best;

  InitCaptures(p, m);

  // Main loop

  while ((move = NextCapture(m))) {

  // Delta pruning

  if (best + tp_value[TpOnSq(p, Tsq(move))] + 300 < alpha) continue;

  // Pruning of bad captures

  if (BadCapture(p, move)) continue;

    p->DoMove(move, u);
    if (Illegal(p)) { p->UndoMove(move, u); continue; }
    score = -Quiesce(p, ply + 1, -beta, -alpha, new_pv);
    p->UndoMove(move, u);
    if (abort_search) return 0;

  // Beta cutoff

    if (score >= beta)
      return score;

  // Adjust alpha and score

    if (score > best) {
      best = score;
      if (score > alpha) {
        alpha = score;
        BuildPv(pv, new_pv, move);
      }
    }
  }
  return best;
}
