#include <assert.h>
#include <stdio.h>
#include "rodent.h"
#include "magicmoves.h"
#include "eval.h"

static const int max_phase = 24;
const int phase_value[7] = { 0, 1, 1, 2, 4, 0, 0 };

static const U64 bbQSCastle[2] = { SqBb(A1) | SqBb(B1) | SqBb(C1) | SqBb(A2) | SqBb(B2) | SqBb(C2),
                                   SqBb(A8) | SqBb(B8) | SqBb(C8) | SqBb(A7) | SqBb(B7) | SqBb(C7)
                                 };
static const U64 bbKSCastle[2] = { SqBb(F1) | SqBb(G1) | SqBb(H1) | SqBb(F2) | SqBb(G2) | SqBb(H2),
                                   SqBb(F8) | SqBb(G8) | SqBb(H8) | SqBb(F7) | SqBb(G7) | SqBb(H7)
                                 };

static const U64 bbCentralFile = FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB;

U64 bbPawnTakes[2];
U64 bbPawnCanTake[2];
U64 support_mask[2][64];
int mg_pst_data[2][6][64];
int eg_pst_data[2][6][64];
int mg[2][N_OF_FACTORS];
int eg[2][N_OF_FACTORS];

char *factor_name[] = { "Pst       ", "Pawns     ", "Passers   ", "Attack    ", "Mobility  ", "Outposts  ", "Lines     ", "Others    "};


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
}

void InitEval(void) {

  // Init piece/square values together with material value of the pieces.
  // Middgame queen table (-5 on the first rank, othewise 0) and endgame
  // rook table (all zeroes) are initialized by a formula rather than
  // by reading a set of constants.

  for (int sq = 0; sq < 64; sq++) {
    for (int sd = 0; sd < 2; sd++) {
 
	  mg_pst_data[sd][P][REL_SQ(sq, sd)] = tp_value[P] + 5 * file_bonus[File(sq)];
	  if (sq == D4 || sq == E4) mg_pst_data[sd][P][REL_SQ(sq, sd)] = tp_value[P] + 25;
	  if (sq == C4 || sq == F4) mg_pst_data[sd][P][REL_SQ(sq, sd)] = tp_value[P] + 10;
	  if (sq == C2 || sq == F2) mg_pst_data[sd][P][REL_SQ(sq, sd)] = tp_value[P] + 0;
	  if (sq == D2 || sq == E2) mg_pst_data[sd][P][REL_SQ(sq, sd)] = tp_value[P] + 5;

	  eg_pst_data[sd][P][REL_SQ(sq, sd)] = tp_value[P] - file_bonus[File(sq)];

	  mg_pst_data[sd][N][REL_SQ(sq, sd)] = tp_value[N] + (5 * (knightLine[File(sq)] + knightRank[Rank(sq)]));
	  if (sq == D2 || sq == E2) mg_pst_data[sd][N][REL_SQ(sq, sd)] = tp_value[N] +(5 * (knightLine[File(sq)] + knightRank[Rank(sq)])) + 5;
	  if (sq == A8 || sq == H8) mg_pst_data[sd][N][REL_SQ(sq, sd)] = tp_value[N] +(5 * (knightLine[File(sq)] + knightRank[Rank(sq)])) -100;

	  eg_pst_data[sd][N][REL_SQ(sq, sd)] = tp_value[N] + 5 * (knightLine[Rank(sq)] + knightLine[File(sq)]);
	  mg_pst_data[sd][B][REL_SQ(sq, sd)] = tp_value[B] + pstBishopMg[sq];
	  eg_pst_data[sd][B][REL_SQ(sq, sd)] = tp_value[B] + pstBishopEg[sq];
	  mg_pst_data[sd][R][REL_SQ(sq, sd)] = tp_value[R] + file_bonus[File(sq)];
      eg_pst_data[sd][R][REL_SQ(sq, sd)] = tp_value[R];
      mg_pst_data[sd][Q][REL_SQ(sq, sd)] = tp_value[Q] - (5 * (Rank(sq) == RANK_1));
	  eg_pst_data[sd][Q][REL_SQ(sq, sd)] = tp_value[Q] + (4 * (biased[Rank(sq)] + biased[File(sq)]));
	  mg_pst_data[sd][K][REL_SQ(sq, sd)] = 10 * (kingRank[Rank(sq)] + kingFile[File(sq)]);
	  eg_pst_data[sd][K][REL_SQ(sq, sd)] = 12 * (biased[Rank(sq)] + biased[File(sq)]);
    }
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
}

void EvaluatePieces(POS *p, int sd) {

  U64 bbPieces, bbMob, bbAtt, bbFile;
  int op, sq, cnt, tmp, ksq, att = 0, wood = 0;

  // Is color OK?

  assert(sd == WC || sd == BC);

  // Init variables

  op = Opp(sd);
  ksq = KingSq(p, op);

  // Init enemy king zone for attack evaluation. We mark squares where the king
  // can move plus two or three more squares facing enemy position.

  U64 bbZone = k_attacks[ksq];
  (sd == WC) ? bbZone |= ShiftSouth(bbZone) : bbZone |= ShiftNorth(bbZone);
  
  // Knight

  bbPieces = PcBb(p, sd, N);
  while (bbPieces) {
    sq = PopFirstBit(&bbPieces);
    
    // Knight mobility

    bbMob = n_attacks[sq] & ~p->cl_bb[sd];
    cnt = PopCnt(bbMob &~bbPawnTakes[op]) - 4;
    Add(sd, F_MOB, 4*cnt, 4*cnt);

    // Knight attacks on enemy king zone

    bbAtt = n_attacks[sq];
    if (bbAtt & bbZone) {
      wood++;
      att += 5 * PopCnt(bbAtt & bbZone);
    }

    // Knight outpost

    tmp = pstKnightOutpost[REL_SQ(sq, sd)];
	if (SqBb(sq) & ~bbPawnCanTake[op]) 
      Add(sd, F_OUTPOST, tmp, tmp);
  }

  // Bishop

  bbPieces = PcBb(p, sd, B);
  while (bbPieces) {
    sq = PopFirstBit(&bbPieces);
  
    // Bishop mobility

    bbMob = BAttacks(OccBb(p), sq);
    cnt = PopCnt(bbMob &~bbPawnTakes[op]) - 7;
    Add(sd, F_MOB, 5 * cnt, 5 * cnt);

    // Bishop attacks on enemy king zone

    bbAtt = BAttacks(OccBb(p) ^ PcBb(p,sd, Q) , sq);
    if (bbAtt & bbZone) {
      wood++;
      att += 4 * PopCnt(bbAtt & bbZone);
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
  
    // Rook mobility

    bbMob = RAttacks(OccBb(p), sq);
    cnt = PopCnt(bbMob) - 7;
    Add(sd, F_MOB, 2 * cnt, 4 * cnt);

    // Rook attacks on enemy king zone

    bbAtt = RAttacks(OccBb(p) ^ PcBb(p, sd, Q) ^ PcBb(p, sd, R), sq);
    if (bbAtt & bbZone) {
      wood++;
      att += 8 * PopCnt(bbAtt & bbZone);
    }

    // Rook on (half) open file

    bbFile = FillNorth(SqBb(sq)) | FillSouth(SqBb(sq));
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

    // Queen mobility

    bbMob = QAttacks(OccBb(p), sq);
    cnt = PopCnt(bbMob) - 14;
    Add(sd, F_MOB, 1 * cnt, 2 * cnt);

    // Queen attacks on enemy king zone
   
    bbAtt  = BAttacks(OccBb(p) ^ PcBb(p, sd, B) ^ PcBb(p, sd, Q), sq);
    bbAtt |= RAttacks(OccBb(p) ^ PcBb(p, sd, B) ^ PcBb(p, sd, Q), sq);
    if (bbAtt & bbZone) {
      wood++;
      att += 16 * PopCnt(bbAtt & bbZone);
    }
  }

  // Score king attacks if own queen is present

  if (wood > 1 && p->cnt[sd][Q]) {
    tmp = att * (wood - 1);
    Add(sd, F_ATT, tmp, tmp);
  }
}

void EvaluateKing(POS *p, int sd) {

  const int startSq[2] = { E1, E8 };
  const int qCastle[2] = { B1, B8 };
  const int kCastle[2] = { G1, G8 };

  U64 bbKingFile, bbNextFile;
  int result = 0;
  int sq = KingSq(p, sd);

  // Normalize king square for pawn shield evaluation,
  // to discourage shuffling the king between g1 and h1.

  if (SqBb(sq) & bbKSCastle[sd]) sq = kCastle[sd];
  if (SqBb(sq) & bbQSCastle[sd]) sq = qCastle[sd];

  // Evaluate shielding and storming pawns on each file.

  bbKingFile = FillNorth(SqBb(sq)) | FillSouth(SqBb(sq));
  result += EvalKingFile(p, sd, bbKingFile);

  bbNextFile = ShiftEast(bbKingFile);
  if (bbNextFile) result += EvalKingFile(p, sd, bbNextFile);

  bbNextFile = ShiftWest(bbKingFile);
  if (bbNextFile) result += EvalKingFile(p, sd, bbNextFile);

  mg[sd][F_OTHERS] += result;
}

int EvalKingFile(POS * p, int sd, U64 bbFile) {

  int shelter = EvalFileShelter(bbFile & PcBb(p, sd, P), sd);
  int storm   = EvalFileStorm  (bbFile & PcBb(p, Opp(sd), P), sd);
  if (bbFile & bbCentralFile) return (shelter / 2) + storm;
  else return shelter + storm;
}

int EvalFileShelter(U64 bbOwnPawns, int sd) {

  if (!bbOwnPawns) return -36;
  if (bbOwnPawns & bbRelRank[sd][RANK_2]) return    2;
  if (bbOwnPawns & bbRelRank[sd][RANK_3]) return  -11;
  if (bbOwnPawns & bbRelRank[sd][RANK_4]) return  -20;
  if (bbOwnPawns & bbRelRank[sd][RANK_5]) return  -27;
  if (bbOwnPawns & bbRelRank[sd][RANK_6]) return  -32;
  if (bbOwnPawns & bbRelRank[sd][RANK_7]) return  -35;
  return 0;
}

int EvalFileStorm(U64 bbOppPawns, int sd) {

  if (!bbOppPawns) return -16;
  if (bbOppPawns & bbRelRank[sd][RANK_3]) return -32;
  if (bbOppPawns & bbRelRank[sd][RANK_4]) return -16;
  if (bbOppPawns & bbRelRank[sd][RANK_5]) return -8;
  return 0;
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
  EvaluatePawns(p, WC); 
  EvaluatePawns(p, BC);
  EvaluateKing(p, WC);
  EvaluateKing(p, BC);
  
  // Sum all the eval factors

  for (int fc = 0; fc < N_OF_FACTORS; fc++) {
      mg_score += (mg[WC][fc] - mg[BC][fc]) * weights[fc] / 100;
      eg_score += (eg[WC][fc] - eg[BC][fc]) * weights[fc] / 100;
  }

  // Merge mg/eg scores

  int mg_phase = Min(max_phase, p->phase);
  int eg_phase = max_phase - mg_phase;

  score += (((mg_score * mg_phase) + (eg_score * eg_phase)) / max_phase);

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
	mg_score = mg[WC][fc] - mg[BC][fc];
	eg_score = eg[WC][fc] - eg[BC][fc];
	total = (((mg_score * mg_phase) + (eg_score * eg_phase)) / max_phase);

    printf(factor_name[fc]);
    printf(" | %4d (%3d) | %4d (%4d, %4d) | %4d (%4d, %4d) |\n", total, weights[fc], mg_score, mg[WC][fc], mg[BC][fc], eg_score, eg[WC][fc], eg[BC][fc]);
  }
  printf("-----------------------------------------------------------------\n");
}