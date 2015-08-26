#include <stdio.h>
#include <string.h>
#include "rodent.h"

void Think(POS *p, int *pv)
{
  ClearHist();
  tt_date = (tt_date + 1) & 255;
  nodes = 0;
  abort_search = 0;
  start_time = GetMS();
  for (root_depth = 1; root_depth <= max_depth; root_depth++) {
    printf("info depth %d\n", root_depth);
    Search(p, 0, -INF, INF, root_depth, pv);
    if (abort_search)
      break;
  }
}

int Search(POS *p, int ply, int alpha, int beta, int depth, int *pv)
{
  int best, score, move, new_depth, new_pv[MAX_PLY];
  int reduction;
  int mv_type;
  int is_pv = (beta > alpha + 1);
  int mv_tried = 0;
  MOVES m[1];
  UNDO u[1];

  // QUIESCENCE SEARCH ENTRY POINT

  if (depth <= 0)
    return Quiesce(p, ply, alpha, beta, pv);

  nodes++;

  // EARLY EXIT

  Check();
  if (abort_search) return 0;
  if (ply) *pv = 0;
  if (IsDraw(p) && ply) return 0;

  // TRANSPOSITION TABLE READ

  move = 0;
  if (TransRetrieve(p->key, &move, &score, alpha, beta, depth, ply)) {
	  if (!is_pv || (score > alpha && score < beta)) // in pv nodes only exact scores are returned
		  return score;
  }

  // SAFEGUARD AGAINST EXCEEDING PLY LIMIT

  if (ply >= MAX_PLY - 1) return Evaluate(p);

  int fl_check = InCheck(p);

  // NULL MOVE

  if (depth > 1 && beta <= Evaluate(p) && !fl_check && MayNull(p)) {
    DoNull(p, u);
    score = -Search(p, ply + 1, -beta, -beta + 1, depth - 3, new_pv);
    UndoNull(p, u);
    if (abort_search) return 0;
    if (score >= beta) {
      TransStore(p->key, 0, score, LOWER, depth, ply);
      return score;
    }
  }

  best = -INF;
  InitMoves(p, m, move, ply);

  // MAIN LOOP

  while ((move = NextMove(m, &mv_type))) {
    DoMove(p, move, u);
    if (Illegal(p)) { UndoMove(p, move, u); continue; }
	mv_tried++;

	// SET NEW DEPTH

    new_depth = depth - 1 + InCheck(p);

	// LATE MOVE REDUCTION (LMR)

	reduction = 0;

	if (!fl_check
	&&  !InCheck(p)
	&&   mv_tried > 3
	&&   depth > 3
	&&  !is_pv
	&&   mv_type == MV_NORMAL) {
		 reduction = 1;
		 new_depth -= reduction;
	}

	// PVS

	re_search:

    if (best == -INF)
      score = -Search(p, ply + 1, -beta, -alpha, new_depth, new_pv);
    else {
      score = -Search(p, ply + 1, -alpha - 1, -alpha, new_depth, new_pv);
      if (!abort_search && score > alpha && score < beta)
        score = -Search(p, ply + 1, -beta, -alpha, new_depth, new_pv);
    }

	// LMR RE_SEARCH

	if (reduction && score > alpha) {
		new_depth += reduction;
		reduction = 0;
		goto re_search;
	}

    UndoMove(p, move, u);
    if (abort_search) return 0;

	// BETA CUTOFF

    if (score >= beta) {
      Hist(p, move, depth, ply);
      TransStore(p->key, move, score, LOWER, depth, ply);
      return score;
    }

	// SCORE CHANGE

    if (score > best) {
      best = score;
      if (score > alpha) {
        alpha = score;
        BuildPv(pv, new_pv, move);
        if (!ply) DisplayPv(score, pv);
      }
    }
  }

  // RETURN CORRECT CHECKMATE/STALEMATE SCORE

  if (best == -INF)
    return InCheck(p) ? -MATE + ply : 0;

  // TRANSPOSITION TABLE WRITE

  if (*pv) {
    Hist(p, *pv, depth, ply);
    TransStore(p->key, *pv, best, EXACT, depth, ply);
  } else
    TransStore(p->key, 0, best, UPPER, depth, ply);

  // EXIT

  return best;
}

int IsDraw(POS *p)
{
  // Draw by 50 move rule

  if (p->rev_moves > 100) return 1;

  // Draw by repetition

  for (int i = 4; i <= p->rev_moves; i += 2)
	  if (p->key == p->rep_list[p->head - i])
		  return 1;

  // Default: no draw

  return 0;
}

void DisplayPv(int score, int *pv)
{
  char *type, pv_str[512];

  type = "mate";
  if (score < -MAX_EVAL)
    score = (-MATE - score) / 2;
  else if (score > MAX_EVAL)
    score = (MATE - score + 1) / 2;
  else
    type = "cp";
  PvToStr(pv, pv_str);
  printf("info depth %d time %d nodes %d score %s %d pv %s\n",
      root_depth, GetMS() - start_time, nodes, type, score, pv_str);
}

void Check(void)
{
  char command[80];

  if (nodes & 4095 || root_depth == 1)
    return;
  if (InputAvailable()) {
    ReadLine(command, sizeof(command));
    if (strcmp(command, "stop") == 0)
      abort_search = 1;
    else if (strcmp(command, "ponderhit") == 0)
      pondering = 0;
  }
  if (!pondering && move_time >= 0 && GetMS() - start_time >= move_time)
    abort_search = 1;
}
