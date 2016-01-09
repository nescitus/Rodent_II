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

#define USE_MM_POPCNT

#if defined(__GNUC__)

int PopCnt(U64 bb) {
  return __builtin_popcountll(bb);
}

#elif defined(USE_MM_POPCNT) && defined(_M_AMD64)  // 64 bit windows
#include <nmmintrin.h>

int PopCnt(U64 bb) {
  return (int)_mm_popcnt_u64(bb);
}

#else

int PopCnt(U64 bb) // general purpose population count
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

int PopFirstBit(U64 * bb) {

  U64 bbLocal = *bb;
  *bb &= (*bb - 1);
  return FirstOne(bbLocal);
}

U64 FillNorth(U64 bb) {
  bb |= bb << 8;
  bb |= bb << 16;
  bb |= bb << 32;
  return bb;
}

U64 FillSouth(U64 bb) {
  bb |= bb >> 8;
  bb |= bb >> 16;
  bb |= bb >> 32;
  return bb;
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

  if (sd == WC) return FillNorth(ShiftNorth(bb));
  else          return FillSouth(ShiftSouth(bb));
}