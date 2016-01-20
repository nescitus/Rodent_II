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
#include "eval.h"

void EvalPatterns(POS * p) {

  U64 king_mask, rook_mask;

  // Blockage of a central pawn on its initial square

  if (IsOnSq(p, WC, P, D2) && IsOnSq(p, WC, B, C1)
  && OccBb(p) & SqBb(D3)) Add(WC, F_OTHERS, -50, 0);

  if (IsOnSq(p, WC, P, E2) && IsOnSq(p, WC, B, F1)
  && OccBb(p) & SqBb(E3)) Add(WC, F_OTHERS, -50, 0);

  if (IsOnSq(p, BC, P, D7) && IsOnSq(p, BC, B, C8)
  && OccBb(p) & SqBb(D6)) Add(BC, F_OTHERS, -50, 0);

  if (IsOnSq(p, BC, P, E7) && IsOnSq(p, BC, B, F8)
  && OccBb(p) & SqBb(E6)) Add(BC, F_OTHERS, -50, 0);

  // Trapped bishop
  
  if (IsOnSq(p, WC, B, A7) && IsOnSq(p, BC, P, B6)) Add(WC, F_OTHERS, -150, -150);
  if (IsOnSq(p, WC, B, B8) && IsOnSq(p, BC, P, C7)) Add(WC, F_OTHERS, -150, -150);
  if (IsOnSq(p, WC, B, H7) && IsOnSq(p, BC, P, G6)) Add(WC, F_OTHERS, -150, -150);
  if (IsOnSq(p, WC, B, G8) && IsOnSq(p, BC, P, F7)) Add(WC, F_OTHERS, -150, -150);
  if (IsOnSq(p, WC, B, A6) && IsOnSq(p, BC, P, B5)) Add(WC, F_OTHERS, -50, -50);
  if (IsOnSq(p, WC, B, H6) && IsOnSq(p, BC, P, G5)) Add(WC, F_OTHERS, -50, -50);

  if (IsOnSq(p, BC, B, A2) && IsOnSq(p, WC, P, B3)) Add(BC, F_OTHERS, -150, -150);
  if (IsOnSq(p, BC, B, B1) && IsOnSq(p, WC, P, C2)) Add(BC, F_OTHERS, -150, -150);
  if (IsOnSq(p, BC, B, H2) && IsOnSq(p, WC, P, G3)) Add(BC, F_OTHERS, -150, -150);
  if (IsOnSq(p, BC, B, G1) && IsOnSq(p, WC, P, F2)) Add(BC, F_OTHERS, -150, -150);
  if (IsOnSq(p, BC, B, A3) && IsOnSq(p, WC, P, B4)) Add(BC, F_OTHERS, -50, -50);
  if (IsOnSq(p, BC, B, H3) && IsOnSq(p, WC, P, G4)) Add(BC, F_OTHERS, -50, -50);
  
  // Rook blocked by uncastled king

  king_mask = SqBb(F1) | SqBb(G1);
  rook_mask = SqBb(G1) | SqBb(H1) | SqBb(H2);

  if ((PcBb(p, WC, K) & king_mask)
  && (PcBb(p, WC, R) & rook_mask)) Add(WC, F_OTHERS, -50, 0);

  king_mask = SqBb(A1) | SqBb(B1);
  rook_mask = SqBb(A1) | SqBb(B1) | SqBb(A2);

  if ((PcBb(p, WC, K) & king_mask)
  && (PcBb(p, WC, R) & rook_mask)) Add(WC, F_OTHERS, -50, 0);

  king_mask = SqBb(F8) | SqBb(G8);
  rook_mask = SqBb(G8) | SqBb(H8) | SqBb(H7);

  if ((PcBb(p, BC, K) & king_mask)
  && (PcBb(p, BC, R) & rook_mask)) Add(BC, F_OTHERS, -50, 0);

  king_mask = SqBb(C8) | SqBb(B8);
  rook_mask = SqBb(C8) | SqBb(B8) | SqBb(B7);

  if ((PcBb(p, BC, K) & king_mask)
  && (PcBb(p, BC, R) & rook_mask)) Add(BC, F_OTHERS, -50, 0);

  // TODO "luft" eval
}