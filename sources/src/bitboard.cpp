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

#define USE_MM_POPCNT

static const int n_moves[8] = { -33, -31, -18, -14, 14, 18, 31, 33 };
static const int k_moves[8] = { -17, -16, -15, -1, 1, 15, 16, 17 };

void cBitBoard::Init() {

  int x;

  initmagicmoves();

  // init knight attacks

  for (int i = 0; i < 64; i++) {
    n_attacks[i] = 0;
    for (int j = 0; j < 8; j++) {
      x = Map0x88(i) + n_moves[j];
      if (!Sq0x88Off(x))
        n_attacks[i] |= SqBb(Unmap0x88(x));
    }
  }

  // init king attacks

  for (int i = 0; i < 64; i++) {
    k_attacks[i] = 0;
    for (int j = 0; j < 8; j++) {
      x = Map0x88(i) + k_moves[j];
      if (!Sq0x88Off(x))
        k_attacks[i] |= SqBb(Unmap0x88(x));
    }
  }

}

#if defined(__GNUC__)

int cBitBoard::PopCnt(U64 bb) {
  return __builtin_popcountll(bb);
}

#elif defined(USE_MM_POPCNT) && defined(_M_AMD64)  // 64 bit windows
#include <nmmintrin.h>

int cBitBoard::PopCnt(U64 bb) {
  return (int)_mm_popcnt_u64(bb);
}

#else

int cBitBoard::PopCnt(U64 bb) // general purpose population count
{
  U64 k1 = (U64)0x5555555555555555;
  U64 k2 = (U64)0x3333333333333333;
  U64 k3 = (U64)0x0F0F0F0F0F0F0F0F;
  U64 k4 = (U64)0x0101010101010101;

  bb -= (bb >> 1) & k1;
  bb = (bb & k2) + ((bb >> 2) & k2);
  bb = (bb + (bb >> 4)) & k3;
  return (bb * k4) >> 56;
}

#endif

int cBitBoard::PopFirstBit(U64 * bb) {

  U64 bbLocal = *bb;
  *bb &= (*bb - 1);
  return FirstOne(bbLocal);
}

U64 cBitBoard::FillNorth(U64 bb) {
  bb |= bb << 8;
  bb |= bb << 16;
  bb |= bb << 32;
  return bb;
}

U64 cBitBoard::FillSouth(U64 bb) {
  bb |= bb >> 8;
  bb |= bb >> 16;
  bb |= bb >> 32;
  return bb;
}

U64 cBitBoard::FillNorthExcl(U64 bb) {
  return FillNorth(ShiftNorth(bb));
}

U64 cBitBoard::FillSouthExcl(U64 bb) {
  return FillSouth(ShiftSouth(bb));
}

U64 cBitBoard::FillNorthSq(int sq) {
  return FillNorth(SqBb(sq));
}

U64 cBitBoard::FillSouthSq(int sq) {
  return FillSouth(SqBb(sq));
}

U64 GetWPControl(U64 bb) {
  return (ShiftNE(bb) | ShiftNW(bb));
}

U64 GetBPControl(U64 bb) {
  return (ShiftSE(bb) | ShiftSW(bb));
}

U64 GetDoubleWPControl(U64 bb) {
  return (ShiftNE(bb) & ShiftNW(bb));
}

U64 GetDoubleBPControl(U64 bb) {
  return (ShiftSE(bb) & ShiftSW(bb));
}

U64 GetFrontSpan(U64 bb, int sd) {

  if (sd == WC) return BB.FillNorthExcl(bb);
  else          return BB.FillSouthExcl(bb);
}

U64 ShiftFwd(U64 bb, int side) {

  if (side == WC) return ShiftNorth(bb);
  return ShiftSouth(bb);
}

U64 cBitBoard::KnightAttacks(int sq) {
  return n_attacks[sq];
}

U64 cBitBoard::RookAttacks(U64 occ, int sq) {
  return Rmagic(sq, occ);
}

U64 cBitBoard::BishAttacks(U64 occ, int sq) {
  return Bmagic(sq, occ);
}

U64 cBitBoard::QueenAttacks(U64 occ, int sq) {
  return Rmagic(sq, occ) | Bmagic(sq, occ);
}

U64 cBitBoard::KingAttacks(int sq) {
	return k_attacks[sq];
}