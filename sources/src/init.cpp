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

void Init(void) {

  int i, j, k, x;
  static const int dirs[4][2] = {{1, -1}, {16, -16}, {17, -17}, {15, -15}};
  static const int p_moves[2][2] = {{15, 17}, {-17, -15}};
  static const int line[8] = {0, 2, 4, 5, 5, 4, 2, 0};

  for (i = 0; i < 2; i++)
    for (j = 0; j < 64; j++) {
      p_attacks[i][j] = 0;
      for (k = 0; k < 2; k++) {
        x = Map0x88(j) + p_moves[i][k];
        if (!Sq0x88Off(x))
          p_attacks[i][j] |= SqBb(Unmap0x88(x));
      }
    }

  for (i = 0; i < 64; i++)
    castle_mask[i] = 15;

  castle_mask[A1] = 13;
  castle_mask[E1] = 12;
  castle_mask[H1] = 14;
  castle_mask[A8] = 7;
  castle_mask[E8] = 3;
  castle_mask[H8] = 11;

  for (i = 0; i < 12; i++)
    for (j = 0; j < 64; j++)
      zob_piece[i][j] = Random64();

  for (i = 0; i < 16; i++)
    zob_castle[i] = Random64();

  for (i = 0; i < 8; i++)
    zob_ep[i] = Random64();
}
