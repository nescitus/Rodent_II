#include <assert.h>
#include "rodent.h"
#include "eval.h"

static const U64 bbQSCastle[2] = { SqBb(A1) | SqBb(B1) | SqBb(C1) | SqBb(A2) | SqBb(B2) | SqBb(C2),
                                   SqBb(A8) | SqBb(B8) | SqBb(C8) | SqBb(A7) | SqBb(B7) | SqBb(C7)
};
static const U64 bbKSCastle[2] = { SqBb(F1) | SqBb(G1) | SqBb(H1) | SqBb(F2) | SqBb(G2) | SqBb(H2),
                                   SqBb(F8) | SqBb(G8) | SqBb(H8) | SqBb(F7) | SqBb(G7) | SqBb(H7)
};

static const U64 bbCentralFile = FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB;

sPawnHashEntry PawnTT[EVAL_HASH_SIZE];

void ClearPawnHash(void) {

  for (int e = 0; e < PAWN_HASH_SIZE; e++) {
    PawnTT[e].key = 0;
    PawnTT[e].mg_pawns = 0;
    PawnTT[e].eg_pawns = 0;
    PawnTT[e].mg_passers = 0;
    PawnTT[e].eg_passers = 0;
  }
}

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

  // Pawn eval

  EvaluatePawns(p, WC);
  EvaluatePawns(p, BC);

  // King's pawn shield and pawn storm on enemy king

  EvaluateKing(p, WC);
  EvaluateKing(p, BC);

  // Save stuff in pawn hashtable

  PawnTT[addr].key = p->pawn_key;
  PawnTT[addr].mg_pawns = mg[WC][F_PAWNS] - mg[BC][F_PAWNS];
  PawnTT[addr].mg_passers = mg[WC][F_PASSERS] - mg[BC][F_PASSERS];
  PawnTT[addr].eg_pawns = eg[WC][F_PAWNS] - eg[BC][F_PAWNS];
  PawnTT[addr].eg_passers = eg[WC][F_PASSERS] - eg[BC][F_PASSERS];
}

void EvaluatePawns(POS *p, int sd) {

  U64 bbPieces, bbSpan, fl_phalanx1, fl_phalanx2;
  int sq, fl_unopposed;
  int op = Opp(sd);
  U64 bbOwnPawns = PcBb(p, sd, P);

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
	fl_phalanx1 = ShiftEast(SqBb(sq)) & bbOwnPawns;
	fl_phalanx2 = ShiftWest(SqBb(sq)) & bbOwnPawns;

    // Doubled pawn

    if (bbSpan & PcBb(p, sd, P))
      Add(sd, F_PAWNS, -10, -20);

    // Passed pawn

    if (!(passed_mask[sd][sq] & PcBb(p, Opp(sd), P)))
      Add(sd, F_PASSERS, passed_bonus_mg[sd][Rank(sq)], passed_bonus_eg[sd][Rank(sq)]);

    // Isolated pawn

    if (!(adjacent_mask[File(sq)] & PcBb(p, sd, P)))
	  Add(sd, F_PAWNS, -10 - 10*fl_unopposed, -20);

    // Backward pawn

    else if ((support_mask[sd][sq] & PcBb(p, sd, P)) == 0)
      Add(sd, F_PAWNS, -8 - 8*fl_unopposed, -8);
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

	mg[sd][F_PAWNS] += result;
}

int EvalKingFile(POS * p, int sd, U64 bbFile) {

	int shelter = EvalFileShelter(bbFile & PcBb(p, sd, P), sd);
	int storm = EvalFileStorm(bbFile & PcBb(p, Opp(sd), P), sd);
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