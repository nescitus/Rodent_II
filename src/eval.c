#include "rodent.h"
#include "eval.h"

#define REL_SQ(sq,cl)   ( sq ^ (cl * 56) )
#define ShiftNorth(x)   (x<<8)
#define ShiftSouth(x)   (x>>8)

const int phase_value[7] = { 0, 1, 1, 2, 4, 0, 0 };
int mg_pst_data[2][6][64];
int eg_pst_data[2][6][64];
int mg[2];
int eg[2];
int phase;

void InitEval(void)
{
	// Init piece/square tables (including material values)

	for (int sq = 0; sq < 64; sq++) {
		for (int sd = 0; sd < 2; sd++) {
			mg_pst_data[sd][P][REL_SQ(sq, sd)] = pstPawnMg[sq] + 100; // TODO: lower material value
			eg_pst_data[sd][P][REL_SQ(sq, sd)] = pstPawnEg[sq] + 100; // TODO: lower material value

			mg_pst_data[sd][N][REL_SQ(sq, sd)] = pstKnightMg[sq] + 325;
			eg_pst_data[sd][N][REL_SQ(sq, sd)] = pstKnightEg[sq] + 325;

			mg_pst_data[sd][B][REL_SQ(sq, sd)] = pstBishopMg[sq] + 325;
			eg_pst_data[sd][B][REL_SQ(sq, sd)] = pstBishopEg[sq] + 325;

			mg_pst_data[sd][R][REL_SQ(sq, sd)] = pstRookMg[sq] + 500;
			eg_pst_data[sd][R][REL_SQ(sq, sd)] = pstRookEg[sq] + 500;

			mg_pst_data[sd][Q][REL_SQ(sq, sd)] = pstQueenMg[sq] + 975;
			eg_pst_data[sd][Q][REL_SQ(sq, sd)] = pstQueenEg[sq] + 975;

			mg_pst_data[sd][K][REL_SQ(sq, sd)] = pstKingMg[sq];
			eg_pst_data[sd][K][REL_SQ(sq, sd)] = pstKingEg[sq];
		}
	}
}

int EvaluatePieces(POS *p, int sd)
{
  U64 bbPieces, bbZone, bbMob, bbAtt;
  int op = Opp(sd);
  int sq, mob, cnt, att;

  // Init

  mob = att = 0;
  bbZone = k_attacks[KingSq(p, op)];
  if (sd == WC) bbZone |= ShiftSouth(bbZone);
  if (sd == BC) bbZone |= ShiftNorth(bbZone);

  // Knight

  bbPieces = PcBb(p, sd, N);
  while (bbPieces) {
	  sq = FirstOne(bbPieces);

	  mg[sd] += mg_pst_data[sd][N][sq];
	  eg[sd] += eg_pst_data[sd][N][sq];
	  phase += 1;

	  bbMob = bbAtt = n_attacks[sq] & ~p->cl_bb[sd];
	  cnt = PopCnt(bbMob);
	  mg[sd] += n_mob_mg[cnt];
	  eg[sd] += n_mob_eg[cnt];

	  if (bbAtt & bbZone)
		  att += 8 * PopCnt(bbAtt & bbZone);

	  bbPieces &= bbPieces - 1;
  }

  // Bishop

  bbPieces = PcBb(p, sd, B);
  while (bbPieces) {
    sq = FirstOne(bbPieces);

	mg[sd] += mg_pst_data[sd][B][sq];
	eg[sd] += eg_pst_data[sd][B][sq];
	phase += 1;

    cnt = PopCnt(BAttacks(OccBb(p), sq));
	mg[sd] += b_mob_mg[cnt];
	eg[sd] += b_mob_eg[cnt];

	bbAtt = BAttacks(OccBb(p) ^ PcBb(p, sd, Q), sq);
	if (bbAtt & bbZone)
		att += 2 * PopCnt(bbAtt & bbZone);

    bbPieces &= bbPieces - 1;
  }

  // Rook

  bbPieces = PcBb(p, sd, R);
  while (bbPieces) {
    sq = FirstOne(bbPieces);

	mg[sd] += mg_pst_data[sd][R][sq];
	eg[sd] += eg_pst_data[sd][R][sq];
	phase += 2;

    cnt = PopCnt(RAttacks(OccBb(p), sq));
	mg[sd] += r_mob_mg[cnt];
	eg[sd] += r_mob_eg[cnt];

	bbAtt = RAttacks(OccBb(p) ^ PcBb(p, sd, Q) ^ PcBb(p, sd, R), sq);
	if (bbAtt & bbZone)
		att += 3 * PopCnt(bbAtt & bbZone);
    
	bbPieces &= bbPieces - 1;
  }

  // Queen

  bbPieces = PcBb(p, sd, Q);
  while (bbPieces) {
    sq = FirstOne(bbPieces);

	mg[sd] += mg_pst_data[sd][Q][sq];
	eg[sd] += eg_pst_data[sd][Q][sq];
	phase += 4;

    cnt = PopCnt(QAttacks(OccBb(p), sq));
	mg[sd] += q_mob_mg[cnt];
	eg[sd] += q_mob_eg[cnt];

	bbAtt  = RAttacks(OccBb(p) ^ PcBb(p, sd, Q) ^ PcBb(p, sd, R), sq);
	bbAtt |= BAttacks(OccBb(p) ^ PcBb(p, sd, Q) ^ PcBb(p, sd, B), sq);
	if (bbAtt & bbZone)
		att += 5 * PopCnt(bbAtt & bbZone);

    bbPieces &= bbPieces - 1;
  }

  if (PcBb(p, sd, Q)) {
	  mob += safety[att];
  }

  return mob;
}

int EvaluatePawns(POS *p, int sd)
{
  U64 bbPieces;
  int sq, score;

  score = 0;
  bbPieces = PcBb(p, sd, P);
  while (bbPieces) {
    sq = FirstOne(bbPieces);

	mg[sd] += mg_pst_data[sd][P][sq];
	eg[sd] += eg_pst_data[sd][P][sq];
    
	// Passed pawns

	if (!(passed_mask[sd][sq] & PcBb(p, Opp(sd), P))) {
		score += passed_bonus[sd][Rank(sq)];
	}
    
	// Isolated pawns

	if (!(adjacent_mask[File(sq)] & PcBb(p, sd, P))) {
		score -= 20;
	}

    bbPieces &= bbPieces - 1;
  }
  return score;
}

void EvaluateKing(POS *p, int sd)
{
	int sq = KingSq(p, sd);

	mg[sd] += mg_pst_data[sd][K][sq];
	eg[sd] += eg_pst_data[sd][K][sq];
}

int Evaluate(POS *p)
{
  int score = 0;
  mg[WC] = mg[BC] = 0;
  eg[WC] = eg[BC] = 0;
  phase = 0;

  score += EvaluatePieces(p, WC) - EvaluatePieces(p, BC);
  //score += p->pst[WC] - p->pst[BC];
  score += EvaluatePawns(p, WC) - EvaluatePawns(p, BC);
  EvaluateKing(p, WC); 
  EvaluateKing(p, BC);

  // Interpolate mg/eg scores

  int total_mg = mg[WC] - mg[BC];
  int total_eg = eg[WC] - eg[BC];
  int phase_mg = phase;
  if (phase_mg > 24) phase_mg = 24;
  int phase_eg = 24 - phase_mg;

  score = (((total_mg * phase_mg) + (total_eg * phase_eg)) / 24);

  // Keep score below checkmate values
  
  if (score < -MAX_EVAL)
    score = -MAX_EVAL;
  else if (score > MAX_EVAL)
    score = MAX_EVAL;

  // Return score relative to side to move

  return p->side == WC ? score : -score;
}
