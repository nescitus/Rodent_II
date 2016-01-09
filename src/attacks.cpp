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
#include "magicmoves.h"

U64 AttacksFrom(POS *p, int sq) {

  switch (TpOnSq(p, sq)) {
  case P:
    return p_attacks[Cl(p->pc[sq])][sq];
  case N:
    return n_attacks[sq];
  case B:
    return BAttacks(OccBb(p), sq);
  case R:
    return RAttacks(OccBb(p), sq);
  case Q:
    return QAttacks(OccBb(p), sq);
  case K:
    return k_attacks[sq];
  }
  return 0;
}

U64 AttacksTo(POS *p, int sq) {

  return (PcBb(p, WC, P) & p_attacks[BC][sq]) |
         (PcBb(p, BC, P) & p_attacks[WC][sq]) |
         (p->tp_bb[N] & n_attacks[sq]) |
         ((p->tp_bb[B] | p->tp_bb[Q]) & BAttacks(OccBb(p), sq)) |
         ((p->tp_bb[R] | p->tp_bb[Q]) & RAttacks(OccBb(p), sq)) |
         (p->tp_bb[K] & k_attacks[sq]);
}

int Attacked(POS *p, int sq, int side) {

  return (PcBb(p, side, P) & p_attacks[Opp(side)][sq]) ||
         (PcBb(p, side, N) & n_attacks[sq]) ||
         ((PcBb(p, side, B) | PcBb(p, side, Q)) & BAttacks(OccBb(p), sq)) ||
         ((PcBb(p, side, R) | PcBb(p, side, Q)) & RAttacks(OccBb(p), sq)) ||
         (PcBb(p, side, K) & k_attacks[sq]);
}
