/*
Rodent, a UCI chess playing engine derived from Sungorus 1.4
Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
Copyright (C) 2011-2016 Pawel Koziol

Rodent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Rodent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rodent.h"

static const U64 bbHomeZone[2] = { RANK_1_BB | RANK_2_BB | RANK_3_BB | RANK_4_BB,
                                   RANK_8_BB | RANK_7_BB | RANK_6_BB | RANK_5_BB };

int GetDrawFactor(POS *p, int sd) 
{
  int op = Opp(sd);

  // Case 1: KBP vs Km
  // drawn when defending king stands on pawn's path and can't be driven out by a bishop
  // (must be dealt with before opposite bishops ending)
  if (PcMatB(p, sd)
  && PcMat1Minor(p, op)
  && p->cnt[sd][P] == 1
  && p->cnt[op][P] == 0
  && (SqBb(p->king_sq[op]) & GetFrontSpan(PcBb(p, sd, P), sd))
  && NotOnBishColor(p, sd, p->king_sq[op]) ) 
     return 0;

  // Case 2: Bishops of opposite color

  if (PcMatB(p, sd) && PcMatB(p, op) && DifferentBishops(p)) {

     // 2a: single bishop cannot win without pawns
     if (p->cnt[sd][P] == 0) return 0;

     // 2b: different bishops with a single pawn on the own half of the board
     if (p->cnt[sd][P] == 1
     &&  p->cnt[op][P] == 0) {
       if (bbHomeZone[sd] & PcBb(p, sd, P)) return 0;

     // TODO: 2c: distant bishop controls a square on pawn's path
     }

     // 2d: halve the score for pure BOC endings
     return 32;
  }

  if (p->cnt[sd][P] == 0) {

    // low and almost equal material with no pawns

    if (p->cnt[op][P] == 0) {
      if (PcMatRm(p, sd) && PcMatRm(p, op)) return 8;
      if (PcMatR (p, sd) && PcMatR (p, op)) return 8;
      if (PcMatQ (p, sd) && PcMatQ (p, op)) return 8;
      if (PcMat2Minors(p, sd) && PcMatR(p, op)) return 8;
    }
     
    // K(m) vs K(m) or Km vs Kp(p)
    if (p->cnt[sd][Q] + p->cnt[sd][R] == 0 && p->cnt[sd][B] + p->cnt[sd][N] < 2 ) 
    return 0;

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

  // KRP vs KR

  if (PcMatR(p, sd) && PcMatR(p, op) && p->cnt[sd][P] == 1 && p->cnt[op][P] == 0) {

	  // good defensive position with a king on pawn's path increases drawing chances

	  if ((SqBb(p->king_sq[op]) & GetFrontSpan(PcBb(p, sd, P), sd)))
		  return 32; // 1/2

	  // draw code for rook endgame with edge pawn

	  if ((RelSqBb(A7, sd) & PcBb(p, sd, P))
	  && ( RelSqBb(A8, sd) & PcBb(p, sd, R))
	  && ( FILE_A_BB & PcBb(p, op, R))
	  && ((RelSqBb(H7, sd) & PcBb(p, op, K)) || (RelSqBb(G7, sd) & PcBb(p, op, K)))
	  ) return 0; // dead draw

	  if ((RelSqBb(H7, sd) & PcBb(p, sd, P))
	  && ( RelSqBb(H8, sd) & PcBb(p, sd, R))
	  && ( FILE_H_BB & PcBb(p, op, R))
	  && ((RelSqBb(A7, sd) & PcBb(p, op, K)) || (RelSqBb(B7, sd) & PcBb(p, op, K)))
	  ) return 0; // dead draw

  }

  return 64;
}

int DifferentBishops(POS * p) {

  if ((bbWhiteSq & PcBb(p, WC, B)) && (bbBlackSq & PcBb(p, BC, B))) return 1;
  if ((bbBlackSq & PcBb(p, WC, B)) && (bbWhiteSq & PcBb(p, BC, B))) return 1;
  return 0;
}

int NotOnBishColor(POS * p, int bishSide, int sq) {

  if (((bbWhiteSq & PcBb(p, bishSide, B)) == 0)
  && (SqBb(sq) & bbWhiteSq)) return 1;

  if (((bbBlackSq & PcBb(p, bishSide, B)) == 0)
  && (SqBb(sq) & bbBlackSq)) return 1;

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

int PcMatRm(POS *p, int sd) {
  return (p->cnt[sd][R] == 1 && p->cnt[sd][N] + p->cnt[sd][B] == 1 && p->cnt[sd][Q] == 0);
}