#include "rodent.h"

static const U64 bbHomeZone[2] = { RANK_1_BB | RANK_2_BB | RANK_3_BB | RANK_4_BB,
                                   RANK_8_BB | RANK_7_BB | RANK_6_BB | RANK_5_BB };

int GetDrawFactor(POS *p, int sd) 
{
	int op = Opp(sd);

	// Bishops of opposite color

	if (PcMatB(p, sd) && PcMatB(p, op) && DifferentBishops(p)) {
	   if (p->cnt[sd][P] == 0) return 0; // single bishop cannot win without pawns

	   if (p->cnt[sd][P] == 1
	   &&  p->cnt[op][P] == 0) {
		   if (bbHomeZone[sd] & PcBb(p, sd, P)) return 0; // a pawn on the own half of the board will not queen
		   // TODO: distant bishop controls a square on pawn's path
	   }

		return 32; // halve the score for pure BOC endings
	}

	if (p->cnt[sd][P] == 0) {
	   
	   // K(m) vs K(m) or Km vs Kp(p)
	   if (p->cnt[sd][Q] + p->cnt[sd][R] == 0 && p->cnt[sd][B] + p->cnt[sd][N] < 2) return 0;

	   // KR vs Km(p)
	   if (PcMatR(p, sd) && PcMat1Minor(p, op) ) return 32;

	   // KRm vs KR(p)
	   if (p->cnt[sd][R] == 1 && p->cnt[sd][Q] == 0 &&  p->cnt[sd][B] + p->cnt[sd][N] == 1
	   &&  PcMatR(p, op) ) return 32;

	   // KQm vs KQ(p)
	   if (p->cnt[sd][Q] == 1 && p->cnt[sd][R] == 0 && p->cnt[sd][B] + p->cnt[sd][N] == 1
	   &&  PcMatQ(p, op) ) return 32;

	   // Kmm vs KB(p)
	   if (PcMat2Minors(p,sd) &&  PcMatB(p, op) ) return 16;

	   // KBN vs Km(p)
	   if (PcMatBN(p, sd) &&  PcMat1Minor(p, op) ) return 16;
	}

	return 64;
}

int DifferentBishops(POS * p) {
	if ((bbWhiteSq & PcBb(p, WC, B)) && (bbBlackSq & PcBb(p, BC, B))) return 1;
	if ((bbBlackSq & PcBb(p, WC, B)) && (bbWhiteSq & PcBb(p, BC, B))) return 1;
	return 0;
}

int PcMat1Minor(POS *p, int sd) {
	return (p->cnt[sd][B] + p->cnt[sd][N] == 1 && p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMat2Minors(POS *p, int sd) {
	return (p->cnt[sd][B] + p->cnt[sd][N] == 2 && p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMatBN(POS *p, int sd) {
	return (p->cnt[sd][B] == 1 && p->cnt[sd][N] == 1 && p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMatB(POS *p, int sd) {
	return (p->cnt[sd][B] == 1 && p->cnt[sd][N] + p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMatQ(POS *p, int sd) {
	return (p->cnt[sd][Q] == 1 && p->cnt[sd][N] + p->cnt[sd][B] + p->cnt[sd][R] == 0);
}

int PcMatR(POS *p, int sd) {
	return (p->cnt[sd][R] == 1 && p->cnt[sd][N] + p->cnt[sd][B] + p->cnt[sd][Q] == 0);
}