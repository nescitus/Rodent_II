#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "rodent.h"
#include "magicmoves.h"
#include "eval.h"

#define SCALE(x,y) ((x*y)/100)

static const int maxAttUnit = 399;
static const double maxAttStep = 7.5;
static const double maxAttScore = 1280;
static const double attCurveMult = 0.025;
int danger[512];

static const int max_phase = 24;
const int phase_value[7] = { 0, 1, 1, 2, 4, 0, 0 };
int dist[64][64];

U64 bbAllAttacks[2];
U64 bbPawnTakes[2];
U64 bbTwoPawnsTake[2];
U64 bbPawnCanTake[2];
U64 support_mask[2][64];
int mg_pst_data[2][6][64];
int eg_pst_data[2][6][64];
int mg[2][N_OF_FACTORS];
int eg[2][N_OF_FACTORS];

char *factor_name[] = { "Pst       ", "Pawns     ", "Passers   ", "Attack    ", "Mobility  ", "Tropism   ", "Outposts  ", "Lines     ", "Others    "};


sEvalHashEntry EvalTT[EVAL_HASH_SIZE];

void ClearEvalHash(void) {

  for (int e = 0; e < EVAL_HASH_SIZE; e++) {
    EvalTT[e].key = 0;
    EvalTT[e].score = 0;
  }
}

void InitWeights(void) {

  for (int fc = 0; fc < N_OF_FACTORS; fc++)
    weights[fc] = 100;

  weights[F_TROPISM] = 20;
  mat_perc = 100;
}

void InitEval(void) {

  // Init piece/square values together with material value of the pieces.

  for (int sq = 0; sq < 64; sq++) {
    for (int sd = 0; sd < 2; sd++) {
 
	  mg_pst_data[sd][P][REL_SQ(sq, sd)] = SCALE(tp_value[P], mat_perc) + 5 * file_bonus[File(sq)];
	  if (sq == D4 || sq == E4) mg_pst_data[sd][P][REL_SQ(sq, sd)] = SCALE(tp_value[P], mat_perc) + 25;
	  if (sq == C4 || sq == F4) mg_pst_data[sd][P][REL_SQ(sq, sd)] = SCALE(tp_value[P], mat_perc) + 10;
	  if (sq == C2 || sq == F2) mg_pst_data[sd][P][REL_SQ(sq, sd)] = SCALE(tp_value[P], mat_perc) + 0;
	  if (sq == D2 || sq == E2) mg_pst_data[sd][P][REL_SQ(sq, sd)] = SCALE(tp_value[P], mat_perc) + 5;

	  eg_pst_data[sd][P][REL_SQ(sq, sd)] = SCALE(tp_value[P], mat_perc) - file_bonus[File(sq)];

	  mg_pst_data[sd][N][REL_SQ(sq, sd)] = SCALE(tp_value[N], mat_perc) + (5 * (knightLine[File(sq)] + knightRank[Rank(sq)]));
	  if (sq == D2 || sq == E2) mg_pst_data[sd][N][REL_SQ(sq, sd)] = SCALE(tp_value[N], mat_perc) + (5 * (knightLine[File(sq)] + knightRank[Rank(sq)])) + 5;
	  if (sq == A8 || sq == H8) mg_pst_data[sd][N][REL_SQ(sq, sd)] = SCALE(tp_value[N], mat_perc) + (5 * (knightLine[File(sq)] + knightRank[Rank(sq)])) - 100;

	  eg_pst_data[sd][N][REL_SQ(sq, sd)] = SCALE(tp_value[N], mat_perc) + 5 * (knightLine[Rank(sq)] + knightLine[File(sq)]);
	  mg_pst_data[sd][B][REL_SQ(sq, sd)] = SCALE(tp_value[B], mat_perc) + pstBishopMg[sq];
	  eg_pst_data[sd][B][REL_SQ(sq, sd)] = SCALE(tp_value[B], mat_perc) + pstBishopEg[sq];
	  mg_pst_data[sd][R][REL_SQ(sq, sd)] = SCALE(tp_value[R], mat_perc) + file_bonus[File(sq)];
	  eg_pst_data[sd][R][REL_SQ(sq, sd)] = SCALE(tp_value[R], mat_perc);
	  mg_pst_data[sd][Q][REL_SQ(sq, sd)] = SCALE(tp_value[Q], mat_perc) - (5 * (Rank(sq) == RANK_1));
	  eg_pst_data[sd][Q][REL_SQ(sq, sd)] = SCALE(tp_value[Q], mat_perc) + (4 * (biased[Rank(sq)] + biased[File(sq)]));
	  mg_pst_data[sd][K][REL_SQ(sq, sd)] = 10 * (kingRank[Rank(sq)] + kingFile[File(sq)]);
	  eg_pst_data[sd][K][REL_SQ(sq, sd)] = 12 * (biased[Rank(sq)] + biased[File(sq)]);
    }
  }

  // Init king attack table

  for (int t = 0, i = 1; i < 512; ++i) {
	  t = Min(maxAttScore, Min(int(attCurveMult * i * i), t + maxAttStep));
	  danger[i] = (t * 100) / 256; // rescale to centipawns
  }

  // Init mask for passed pawn detection

  for (int sq = 0; sq < 64; sq++) {
    passed_mask[WC][sq] = FillNorth(ShiftNorth(SqBb(sq)));
    passed_mask[WC][sq] |= ShiftWest(passed_mask[WC][sq]);
    passed_mask[WC][sq] |= ShiftEast(passed_mask[WC][sq]);
    passed_mask[BC][sq] = FillSouth(ShiftSouth(SqBb(sq)));
    passed_mask[BC][sq] |= ShiftWest(passed_mask[BC][sq]);
    passed_mask[BC][sq] |= ShiftEast(passed_mask[BC][sq]);
  }

  // Init adjacent mask (for detecting isolated pawns)

  for (int i = 0; i < 8; i++) {
	  adjacent_mask[i] = 0;
	  if (i > 0) adjacent_mask[i] |= FILE_A_BB << (i - 1);
	  if (i < 7) adjacent_mask[i] |= FILE_A_BB << (i + 1);
  }

  // Init support mask (for detecting weak pawns)

  for (int sq = 0; sq < 64; sq++) {
    support_mask[WC][sq] = ShiftWest(SqBb(sq)) | ShiftEast(SqBb(sq));
    support_mask[WC][sq] |= FillSouth(support_mask[WC][sq]);

    support_mask[BC][sq] = ShiftWest(SqBb(sq)) | ShiftEast(SqBb(sq));
    support_mask[BC][sq] |= FillNorth(support_mask[BC][sq]);
  }

  // Init distance table (for evaluating king tropism)

  for (int i = 0; i < 64; ++i) {
	  for (int j = 0; j < 64; ++j) {
		  dist[i][j] = 14 - (Abs(Rank(i) - Rank(j)) + Abs(File(i) - File(j)));
	  }
  }

}

void EvaluatePieces(POS *p, int sd) {

  U64 bbPieces, bbMob, bbAtt, bbStop, bbFile;
  int op, sq, cnt, tmp, mul, ksq, att = 0, wood = 0;

  // Is color OK?

  assert(sd == WC || sd == BC);

  // Init variables

  op = Opp(sd);
  ksq = KingSq(p, op);

  // Init enemy king zone for attack evaluation. We mark squares where the king
  // can move plus two or three more squares facing enemy position.

  U64 bbZone = k_attacks[ksq];
  (sd == WC) ? bbZone |= ShiftSouth(bbZone) : bbZone |= ShiftNorth(bbZone);

  // Init bitboards to detect check threats

  U64 bbKnightChk = n_attacks[ksq];
  U64 bbStr8Chk = RAttacks(OccBb(p), ksq);
  U64 bbDiagChk = BAttacks(OccBb(p), ksq);
  U64 bbQueenChk = bbStr8Chk | bbDiagChk;
  
  // Knight

  bbPieces = PcBb(p, sd, N);
  while (bbPieces) {
    sq = PopFirstBit(&bbPieces);

	// Knight tropism to enemy king

	Add(sd, F_TROPISM, 3 * dist[sq][ksq], 3 * dist[sq][ksq]);
    
    // Knight mobility

    bbMob = n_attacks[sq] & ~p->cl_bb[sd];
    cnt = PopCnt(bbMob &~bbPawnTakes[op]);
	Add(sd, F_MOB, n_mob_mg[cnt], n_mob_eg[cnt]);          // mobility bonus
	if ((bbMob &~bbPawnTakes[op]) & bbKnightChk) att += 3; // check threat bonus
	bbAllAttacks[sd] |= bbMob;

    // Knight attacks on enemy king zone

    bbAtt = n_attacks[sq];
    if (bbAtt & bbZone) {
      wood++;
      att += 6 * PopCnt(bbAtt & bbZone);
    }

    // Knight outpost

	if (sd == WC) bbStop = ShiftNorth(SqBb(sq));
	if (sd == BC) bbStop = ShiftSouth(SqBb(sq));

	mul = 0;
    tmp = pstKnightOutpost[REL_SQ(sq, sd)];
	if (SqBb(sq) & ~bbPawnCanTake[op]) mul += 2;  // in the hole of enemy pawn structure
	if (SqBb(sq) & bbPawnTakes[sd]) mul += 1;     // defended by own pawn
	if (SqBb(sq) & bbTwoPawnsTake[sd]) mul += 1;  // defended by two pawns
	tmp *= mul;
	tmp /= 2;

    Add(sd, F_OUTPOST, tmp, tmp);
  }

  // Bishop

  bbPieces = PcBb(p, sd, B);
  while (bbPieces) {
    sq = PopFirstBit(&bbPieces);

	// Bishop tropism to enemy king

	Add(sd, F_TROPISM, 2 * dist[sq][ksq], 1 * dist[sq][ksq]);

    // Bishop mobility

    bbMob = BAttacks(OccBb(p), sq);
    cnt = PopCnt(bbMob &~bbPawnTakes[op]);
	Add(sd, F_MOB, b_mob_mg[cnt], b_mob_eg[cnt]);        // mobility bonus
	if ((bbMob &~bbPawnTakes[op]) & bbDiagChk) att += 3; // check threat bonus
	bbAllAttacks[sd] |= bbMob;

    // Bishop attacks on enemy king zone

    bbAtt = BAttacks(OccBb(p) ^ PcBb(p,sd, Q) , sq);
    if (bbAtt & bbZone) {
      wood++;
      att += 6 * PopCnt(bbAtt & bbZone);
    }

    // Bishop outpost

    tmp = pstBishopOutpost[REL_SQ(sq, sd)];
    if (SqBb(sq) & ~bbPawnCanTake[op])
      Add(sd, F_OUTPOST, tmp, tmp);
  }

  // Rook

  bbPieces = PcBb(p, sd, R);
  while (bbPieces) {
    sq = PopFirstBit(&bbPieces);

	// Rook tropism to enemy king

	Add(sd, F_TROPISM, 2 * dist[sq][ksq], 1 * dist[sq][ksq]);
  
    // Rook mobility

    bbMob = RAttacks(OccBb(p), sq);
    cnt = PopCnt(bbMob);
    Add(sd, F_MOB, r_mob_mg[cnt], r_mob_eg[cnt]);        // mobility bonus
	if ((bbMob &~bbPawnTakes[op]) & bbStr8Chk) att += 9; // check threat bonus
	bbAllAttacks[sd] |= bbMob;

    // Rook attacks on enemy king zone

    bbAtt = RAttacks(OccBb(p) ^ PcBb(p, sd, Q) ^ PcBb(p, sd, R), sq);
    if (bbAtt & bbZone) {
      wood++;
      att += 9 * PopCnt(bbAtt & bbZone);
    }

    // Rook on (half) open file

	bbFile = FillNorth(SqBb(sq)) | FillSouth(SqBb(sq)); // better this way than using front span
    if (!(bbFile & PcBb(p, sd, P))) {
      if (!(bbFile & PcBb(p, op, P))) Add(sd, F_LINES, 12, 12);  // beats 10, 10
	  else                            Add(sd, F_LINES,  6,  6);  // beats  5,  5
    }

    // Rook on 7th rank attacking pawns or cutting off enemy king

    if (SqBb(sq) & bbRelRank[sd][RANK_7]) {
      if (PcBb(p, op, P) & bbRelRank[sd][RANK_7]
      ||  PcBb(p, op, K) & bbRelRank[sd][RANK_8]) {
		  Add(sd, F_LINES, 16, 32);
      }
    }
  }

  // Queen

  bbPieces = PcBb(p, sd, Q);
  while (bbPieces) {
    sq = PopFirstBit(&bbPieces);

	// Queen tropism to enemy king

	Add(sd, F_TROPISM, 2 * dist[sq][ksq], 4 * dist[sq][ksq]);

    // Queen mobility

    bbMob = QAttacks(OccBb(p), sq);
    cnt = PopCnt(bbMob);
    Add(sd, F_MOB, q_mob_mg[cnt], q_mob_eg[cnt]);  // mobility bonus
	if ((bbMob &~bbPawnTakes[op]) & bbQueenChk) {  // check threat bonus and contact checks
	  att += 12; 
	  U64 bbContact = bbMob & k_attacks[ksq];
	  while (bbContact) {
		  int contactSq = PopFirstBit(&bbContact);

		  // possible bug: queen exchanges are accepted as contact checks
		  if (Swap(p, sq, contactSq) >= 0) {
			  att += 10;
			  break;
		  }
	  }
	}

	bbAllAttacks[sd] |= bbMob;

    // Queen attacks on enemy king zone
   
    bbAtt  = BAttacks(OccBb(p) ^ PcBb(p, sd, B) ^ PcBb(p, sd, Q), sq);
    bbAtt |= RAttacks(OccBb(p) ^ PcBb(p, sd, B) ^ PcBb(p, sd, Q), sq);
    if (bbAtt & bbZone) {
      wood++;
	  att += 15 * PopCnt(bbAtt & bbZone);
    }
  }

  // Score king attacks if own queen is present and there are at least 2 attackers

  if (wood > 1 && p->cnt[sd][Q]) {
    if (att > 399) att = 399;
	tmp = danger[att];

    Add(sd, F_ATT, tmp, tmp);
  }
}

void EvalHanging(POS *p, int sd) {

	int op = Opp(sd);
	U64 bbHanging = p->cl_bb[op]    & ~bbPawnTakes[op];
	U64 bbThreatened = p->cl_bb[op] & bbPawnTakes[sd];
	bbHanging |= bbThreatened;     // piece attacked by our pawn isn't well defended
	bbHanging &= bbAllAttacks[sd]; // obviously, hanging piece has to be attacked
	bbHanging &= ~PcBb(p, op, P);  // currently we don't evaluate threats against pawns

	int pc, sq, val;

	while (bbHanging) {
		sq = PopFirstBit(&bbHanging);
		pc = TpOnSq(p, sq);
		val = tp_value[pc] / 64;
		Add(sd, F_OTHERS, 10 + val, 18 + val);
	}
}


int Evaluate(POS *p, int use_hash) {

  // Try to retrieve score from eval hashtable

  int addr = p->hash_key % EVAL_HASH_SIZE;

  if (EvalTT[addr].key == p->hash_key && use_hash) {
    int hashScore = EvalTT[addr].score;
    return p->side == WC ? hashScore : -hashScore;
  }

  // Clear eval

  int score = 0;
  int mg_score = 0;
  int eg_score = 0;

  for (int sd = 0; sd < 2; sd++) {
    for (int fc = 0; fc < N_OF_FACTORS; fc++) {
      mg[sd][fc] = 0;
      eg[sd][fc] = 0;
    }
  }

  // Init eval with incrementally updated stuff

  mg[WC][F_PST] = p->mg_pst[WC];
  mg[BC][F_PST] = p->mg_pst[BC];
  eg[WC][F_PST] = p->eg_pst[WC];
  eg[BC][F_PST] = p->eg_pst[BC];

  // Calculate variables used during evaluation

  bbPawnTakes[WC] = GetWPControl(PcBb(p, WC, P));
  bbPawnTakes[BC] = GetBPControl(PcBb(p, BC, P));
  bbTwoPawnsTake[WC] = GetDoubleWPControl(PcBb(p, BC, P));
  bbTwoPawnsTake[BC] = GetDoubleBPControl(PcBb(p, BC, P));
  bbAllAttacks[WC] = bbPawnTakes[WC] | k_attacks[p->king_sq[WC]];
  bbAllAttacks[BC] = bbPawnTakes[BC] | k_attacks[p->king_sq[BC]];
  bbPawnCanTake[WC] = FillNorth(bbPawnTakes[WC]);
  bbPawnCanTake[BC] = FillSouth(bbPawnTakes[BC]);

  // Tempo bonus

  mg[p->side][F_OTHERS] += 10;
  eg[p->side][F_OTHERS] += 5;

  // Bishop pair

  if (PopCnt(PcBb(p, WC, B)) > 1) score += 50;
  if (PopCnt(PcBb(p, BC, B)) > 1) score -= 50;

  // Evaluate pieces and pawns

  EvaluatePieces(p, WC);
  EvaluatePieces(p, BC);
  FullPawnEval(p, use_hash);
  EvalHanging(p, WC);
  EvalHanging(p, BC);
  EvalPatterns(p);

  // Sum all the eval factors

  for (int fc = 0; fc < N_OF_FACTORS; fc++) {
      mg_score += (mg[WC][fc] - mg[BC][fc]) * weights[fc] / 100;
      eg_score += (eg[WC][fc] - eg[BC][fc]) * weights[fc] / 100;
  }

  // Merge mg/eg scores

  int mg_phase = Min(max_phase, p->phase);
  int eg_phase = max_phase - mg_phase;

  score += (((mg_score * mg_phase) + (eg_score * eg_phase)) / max_phase);

  // Material imbalance table

  int minorBalance = p->cnt[WC][N] - p->cnt[BC][N] + p->cnt[WC][B] - p->cnt[BC][B];
  int majorBalance = p->cnt[WC][R] - p->cnt[BC][R] + 2 * p->cnt[WC][Q] - 2 * p->cnt[BC][Q];

  int x = Max(majorBalance + 4, 0);
  if (x > 8) x = 8;

  int y = Max(minorBalance + 4, 0);
  if (y > 8) y = 8;

  score += imbalance[x][y];

  // Scale down drawish endgames

  int draw_factor = 64;
  if (score > 0) draw_factor = GetDrawFactor(p, WC);
  else           draw_factor = GetDrawFactor(p, BC);
  score *= draw_factor;
  score /= 64;

  // Make sure eval doesn't exceed mate score

  if (score < -MAX_EVAL)
    score = -MAX_EVAL;
  else if (score > MAX_EVAL)
    score = MAX_EVAL;

  // Weakening: add pseudo-random value to eval score

  if (eval_blur) {
	  int randomMod = (eval_blur / 2) - (p->hash_key % eval_blur);
	  score += randomMod;
  }

  // Save eval score in the evaluation hash table

  EvalTT[addr].key = p->hash_key;
  EvalTT[addr].score = score;

  // Return score relative to the side to move

  return p->side == WC ? score : -score;
}

void Add(int sd, int factor, int mg_bonus, int eg_bonus) {

  mg[sd][factor] += mg_bonus;
  eg[sd][factor] += eg_bonus;
}

void PrintEval(POS * p) {

  int mg_score, eg_score, total;
  int mg_phase = Min(max_phase, p->phase);
  int eg_phase = max_phase - mg_phase;

  printf("Total score: %d\n", Evaluate(p, 0));
  printf("-----------------------------------------------------------------\n");
  printf("Factor     | Val (perc) |   Mg (  WC,   BC) |   Eg (  WC,   BC) |\n");
  printf("-----------------------------------------------------------------\n");
  for (int fc = 0; fc < N_OF_FACTORS; fc++) {
	mg_score = ((mg[WC][fc] - mg[BC][fc]) * weights[fc]) / 100;
	eg_score = ((eg[WC][fc] - eg[BC][fc]) * weights[fc]) / 100;
	total = (((mg_score * mg_phase) + (eg_score * eg_phase)) / max_phase);

    printf(factor_name[fc]);
    printf(" | %4d (%3d) | %4d (%4d, %4d) | %4d (%4d, %4d) |\n", total, weights[fc], mg_score, mg[WC][fc], mg[BC][fc], eg_score, eg[WC][fc], eg[BC][fc]);
  }
  printf("-----------------------------------------------------------------\n");
}