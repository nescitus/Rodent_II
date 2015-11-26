#include <stdio.h>
#include <string.h>
#include "math.h"
#include "rodent.h"
#include "timer.h"
#include "book.h"

double lmrSize[2][MAX_PLY][MAX_MOVES];
int lmp_limit[6] = { 0, 4, 8, 12, 24, 48 };

void InitSearch(void) {

  // Set depth of late move reduction using modified Stockfish formula

  for (int depth = 0; depth < MAX_PLY; depth++)
    for (int moves = 0; moves < MAX_MOVES; moves++) {
      lmrSize[0][depth][moves] = (0.33 + log((double)(depth)) * log((double)(moves)) / 2.25); // zw node
      lmrSize[1][depth][moves] = (0.00 + log((double)(depth)) * log((double)(moves)) / 3.50); // pv node

      for (int node = 0; node <= 1; node++) {
        if (lmrSize[node][depth][moves] < 1) lmrSize[node][depth][moves] = 0; // ultra-small reductions make no sense
        else lmrSize[node][depth][moves] += 0.5;

        if (lmrSize[node][depth][moves] > depth - 1) // reduction cannot exceed actual depth
          lmrSize[node][depth][moves] = depth - 1;
      }
    }
}

void Think(POS *p, int *pv) {

  pv[0] = GuideBook.GetPolyglotMove(p, 1);
  if (pv[0]) return;

  pv[0] = MainBook.GetPolyglotMove(p, 1);
  if (pv[0]) return;

  ClearHist();
  tt_date = (tt_date + 1) & 255;
  nodes = 0;
  abort_search = 0;
  Timer.SetStartTime();
  Iterate(p, pv);
}

void Iterate(POS *p, int *pv) {

  int val = 0, cur_val = 0;
  U64 nps = 0;
  Timer.SetIterationTiming();

  // Are we operating in slowdown mode?

  Timer.slow_play = 0;
  if (Timer.nps_limit) Timer.slow_play = 1;

  // TODO: only single move available

  for (root_depth = 1; root_depth <= Timer.GetData(MAX_DEPTH); root_depth++) {
    int elapsed = Timer.GetElapsedTime();
    if (elapsed) nps = nodes * 1000 / elapsed;
    printf("info depth %d time %d nodes %I64d nps %I64d\n", root_depth, elapsed, nodes, nps);
    cur_val = Widen(p, root_depth, pv, cur_val);
    if (abort_search || Timer.FinishIteration()) break;
    val = cur_val;
  }
}

int Widen(POS *p, int depth, int * pv, int lastScore) {
  
  // Function performs aspiration search, progressively widening the window.
  // Code structere modelled after Senpai 1.0.

  int cur_val = lastScore, alpha, beta;

  if (depth > 6 && lastScore < MAX_EVAL) {
    for (int margin = 10; margin < 500; margin *= 2) {
      alpha = lastScore - margin;
      beta  = lastScore + margin;
      cur_val = Search(p, 0, alpha, beta, depth, 0, -1, pv);
      if (abort_search) break;
      if (cur_val > alpha && cur_val < beta) 
      return cur_val;                // we have finished within the window
      if (cur_val > MAX_EVAL) break; // verify mate searching with infinite bounds
    }
  }

  cur_val = Search(p, 0, -INF, INF, root_depth, 0, -1, pv);      // full window search
  return cur_val;
}

int Search(POS *p, int ply, int alpha, int beta, int depth, int was_null, int last_move, int *pv) {

  int best, score, null_score, move, new_depth, new_pv[MAX_PLY];
  int fl_check, fl_prunable_node, fl_prunable_move, mv_type, reduction;
  int is_pv = (beta > alpha + 1);
  int mv_tried = 0, quiet_tried = 0, fl_futility = 0;

  MOVES m[1];
  UNDO u[1];

  // Quiescence search entry point

  if (depth <= 0)
    return Quiesce(p, ply, alpha, beta, pv);

  // Periodically check for timeout, ponderhit or stop command

  nodes++;
  Check();

  // Quick exit on a timeout or on a statically detected draw
  
  if (abort_search) return 0;
  if (ply) *pv = 0;
  if (IsDraw(p) && ply) return 0;

  // Retrieving data from transposition table. We hope for a cutoff
  // or at least for a move to improve move ordering.

  move = 0;
  if (TransRetrieve(p->hash_key, &move, &score, alpha, beta, depth, ply)) {
    
    // For move ordering purposes, a cutoff from hash is treated
    // exactly like a cutoff from search

    if (score >= beta) UpdateHistory(p, last_move, move, depth, ply);

    // In pv nodes only exact scores are returned. This is done because
    // there is much more pruning and reductions in zero-window nodes,
    // so retrieving such scores in pv nodes works like retrieving scores
    // from slightly lower depth.

    if (!is_pv || (score > alpha && score < beta))
      return score;
  }
  
  // Safeguard against exceeding ply limit
  
  if (ply >= MAX_PLY - 1)
    return Evaluate(p, 1);

  // Are we in check? Knowing that is useful when it comes 
  // to pruning/reduction decisions

  fl_check = InCheck(p);
  fl_prunable_node = !fl_check & !is_pv && alpha > -MAX_EVAL && beta < MAX_EVAL;

  if (ply && depth <= 3
  && fl_prunable_node
  && !was_null) {
    int sc = Evaluate(p, 1) - 120 * depth; // TODO: Tune me!
    if (sc > beta) return sc;
  }

  // Null move

  if (depth > 1
  && fl_prunable_node
  && !was_null
  && MayNull(p)) {
    int eval = Evaluate(p, 1);
    if (eval > beta) {

      reduction = 4;
      if (depth > 8) reduction += depth / 4;

    // omit null move search if normal search to the same depth wouldn't exceed beta
    // (sometimes we can check it for free via hash table)

      if (TransRetrieve(p->hash_key, &move, &null_score, alpha, beta, depth-reduction, ply)) {
        if (null_score < beta) goto avoid_null;
      }

      p->DoNull(u);
      score = -Search(p, ply + 1, -beta, -beta + 1, depth - reduction, 1, 0, new_pv);
      p->UndoNull(u);

      if (abort_search ) return 0;
      if (score >= beta) return score;
    }
  } 
  
  avoid_null:

  // end of null move code

  // Razoring based on Toga II 3.0

  if (fl_prunable_node
  && !move
  && !was_null
  && !(PcBb(p, p->side, P) & bbRelRank[p->side][RANK_7]) // no pawns to promote in one move
  &&  depth <= 3) {
    int threshold = beta - 300 - (depth - 1) * 60;
    int eval = Evaluate(p, 1);

    if (eval < threshold) {
      score = Quiesce(p, ply, alpha, beta, pv);
      if (score < threshold) return score;
    }
  } 
  
  // end of razoring code

  // Set futility pruning flag

  if (depth <= 6
  && fl_prunable_node) {
    if (Evaluate(p, 1) + 50 + 50 * depth < beta) fl_futility = 1;
  }

  // Init moves and variables before entering main loop
  
  best = -INF;
  InitMoves(p, m, move, refutation[Fsq(last_move)][Tsq(last_move)], ply);
  
  // Main loop
  
  while ((move = NextMove(m, &mv_type))) {
    p->DoMove(move, u);
    if (Illegal(p)) { p->UndoMove(move, u); continue; }

  // Update move statistics (needed for reduction/pruning decisions)

  mv_tried++;
  if (mv_type == MV_NORMAL) quiet_tried++;
  fl_prunable_move = !InCheck(p) & (mv_type == MV_NORMAL);

  // Set new search depth

  new_depth = depth - 1 + InCheck(p);

  // Futility pruning

  if (fl_futility
  &&  fl_prunable_move
  &&  mv_tried > 1) {
    p->UndoMove(move, u); continue;
  }

  // Late move pruning

  if (fl_prunable_node
  && quiet_tried > lmp_limit[depth]
  && fl_prunable_move
  && depth <= 3
  && MoveType(move) != CASTLE ) {
    p->UndoMove(move, u); continue;
  }

  // Late move reduction

  reduction = 0;

  if (depth >= 2 
  && mv_tried > 3
  && alpha > -MAX_EVAL && beta < MAX_EVAL
  && !fl_check 
  &&  fl_prunable_move
  && lmrSize[is_pv][depth][mv_tried] > 0
  && MoveType(move) != CASTLE ) {
    reduction = lmrSize[is_pv][depth][mv_tried];
    new_depth -= reduction;
  }

  re_search:
   
  // PVS

  if (best == -INF)
    score = -Search(p, ply + 1, -beta, -alpha, new_depth, 0, move, new_pv);
  else {
    score = -Search(p, ply + 1, -alpha - 1, -alpha, new_depth, 0, move, new_pv);
    if (!abort_search && score > alpha && score < beta)
      score = -Search(p, ply + 1, -beta, -alpha, new_depth, 0, move, new_pv);
  }

  // Reduced move scored above alpha - we need to re-search it

  if (reduction && score > alpha) {
    new_depth += reduction;
    reduction = 0;
    goto re_search;
  }

    p->UndoMove(move, u);
    if (abort_search) return 0;

  // Beta cutoff

    if (score >= beta) {
      UpdateHistory(p, last_move, move, depth, ply);
      TransStore(p->hash_key, move, score, LOWER, depth, ply);

    // If beta cutoff occurs at the root, change the best move

    if (!ply) {
      BuildPv(pv, new_pv, move);
      DisplayPv(score, pv);
    }

      return score;
    }

  // Updating score and alpha

    if (score > best) {
      best = score;
      if (score > alpha) {
        alpha = score;
        BuildPv(pv, new_pv, move);
        if (!ply) DisplayPv(score, pv);
      }
    }

  } // end of the main loop

  // Return correct checkmate/stalemate score

  if (best == -INF)
    return InCheck(p) ? -MATE + ply : 0;

  // Save score in the transposition table

  if (*pv) {
    UpdateHistory(p, last_move, *pv, depth, ply);
    TransStore(p->hash_key, *pv, best, EXACT, depth, ply);
  } else
    TransStore(p->hash_key, 0, best, UPPER, depth, ply);

  return best;
}

int IsDraw(POS *p) {

  // Draw by 50 move rule

  if (p->rev_moves > 100) return 1;

  // Draw by repetition

  for (int i = 4; i <= p->rev_moves; i += 2)
    if (p->hash_key == p->rep_list[p->head - i])
      return 1;

  // Draw by insufficient material (bare kings or Km vs K)

  if (!Illegal(p)) {
    if (p->cnt[WC][P] + p->cnt[BC][P] + p->cnt[WC][Q] + p->cnt[BC][Q] + p->cnt[WC][R] + p->cnt[BC][R] == 0
    &&  p->cnt[WC][N] + p->cnt[BC][N] + p->cnt[WC][B] + p->cnt[BC][B] <= 1) return 0;
  }

  return 0; // default: no draw
}

void DisplayPv(int score, int *pv) {

  char *type, pv_str[512];
  int elapsed = Timer.GetElapsedTime();
  U64 nps = GetNps(elapsed);

  type = "mate";
  if (score < -MAX_EVAL)
    score = (-MATE - score) / 2;
  else if (score > MAX_EVAL)
    score = (MATE - score + 1) / 2;
  else
    type = "cp";

  PvToStr(pv, pv_str);
  printf("info depth %d time %d nodes %I64d nps %I64d score %s %d pv %s\n",
      root_depth, elapsed, nodes, nps, type, score, pv_str);
}

void Check(void) {

  char command[80];
  int time;
  U64 nps;

  if (!Timer.slow_play) {
    if (nodes & 4095 || root_depth == 1)
      return;
  }

  if (Timer.nps_limit && root_depth > 1) {
    time = Timer.GetElapsedTime() + 1;
    nps = GetNps(time);
    while ((int)nps > Timer.nps_limit) {
      Timer.WasteTime(10);
      time = Timer.GetElapsedTime() + 1;
      nps = GetNps(time);
      if (Timeout()) {
        abort_search = 1;
        return;
      }
    }
  }

  if (InputAvailable()) {
    ReadLine(command, sizeof(command));

    if (strcmp(command, "stop") == 0)
      abort_search = 1;
    else if (strcmp(command, "ponderhit") == 0)
      pondering = 0;
  }

  if (Timeout()) abort_search = 1;
}

int Timeout() {

  return (!pondering && !Timer.IsInfiniteMode() && Timer.TimeHasElapsed());
}

U64 GetNps(int elapsed)
{
  U64 nps = 0;
  if (elapsed) nps = (nodes * 1000) / elapsed;
  return nps;
}