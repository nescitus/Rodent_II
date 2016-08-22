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


typedef class {
public:
  int elo = 2850;
  int fl_weakening = 0;
  int pc_value[7] = { 100, 325, 335, 500, 1000, 0, 0 }; // these values might be changed via UCI options
  int keep_pc[7] =  {   0,   0,   0,   0,   0,  0, 0 };
  int mg_pst[2][6][64];       // midgame piece-square tables (initialized depending on pst_style, pst_perc and mat_perc)
  int eg_pst[2][6][64];       // endgame piece-square tables (initialized depending on pst_style, pst_perc and mat_perc)
  int sp_pst_data[2][6][64];  // special piece/square tables (outposts etc.)
  int danger[512];            // table for evaluating king safety
  int dist[64][64];           // table for evaluating king tropism
  int chebyshev_dist[64][64]; // table for unstoppable passer detection
  int phalanx[2][64];
  int defended[2][64];
  int pst_style = 0;
  int mob_style = 0;
  int mat_perc = 100;
  int pst_perc = 80;
  int shield_perc = 120;
  int storm_perc  = 100;
  int bish_pair = 50;
  int knight_pair = -10;
  int exchange_imbalance = 25;
  int np_bonus = 6;
  int rp_malus = 3;
  int rook_pair_malus = -5;
  int doubled_malus_mg = -12;
  int doubled_malus_eg = -24;
  int isolated_malus_mg = -10;
  int isolated_malus_eg = -20;
  int isolated_open_malus = -10;
  int backward_malus_base = -8;
  int backward_malus_mg[8] = { -5,  -7,  -9, -11, -11,  -9,  -7,  -5 };
  int backward_malus_eg = -8;
  int backward_open_malus = -8;
  int minorBehindPawn = 5;
  int minorVsQueen = 5;
  int bishConfined = -5;
  int rookOn7thMg = 16;
  int rookOn7thEg = 32;
  int twoRooksOn7thMg = 8;
  int twoRooksOn7thEg = 16;
  int rookOnQueen = 5;
  int rookOnOpenMg = 14;
  int rookOnOpenEg = 10;
  int rookOnBadHalfOpenMg = 6;
  int rookOnBadHalfOpenEg = 4;
  int rookOnGoodHalfOpenMg = 8;
  int rookOnGoodHalfOpenEg = 6;
  int queenOn7thMg = 4;
  int queenOn7thEg = 8;
  int imbalance[9][9];
  int np_table[9];
  int rp_table[9];
  int draw_score = 0;
  int book_filter = 20;
  int eval_blur = 0;
  int n_mob_mg[9] =  { -16, -12,  -8,  -4,  +0,  +4,  +8, +12, +16 };
  int n_mob_eg[9] =  { -16, -12,  -8,  -4,  +0,  +4,  +8, +12, +16 };
  int b_mob_mg[14] = { -35, -30, -25, -20, -15, -10,  -5,  +0,  +5, +10, +15, +20, +25, +30 };
  int b_mob_eg[14] = { -35, -30, -25, -20, -15, -10,  -5,  +0,  +5, +10, +15, +20, +25, +30 };
  int r_mob_mg[15] = { -14, -12, -10,  -8,  -6,  -4,  -2,  +0,  +2,  +4,  +6,  +8, +10, +12, +14 };
  int r_mob_eg[15] = { -28, -24, -20, -16, -12,  -8,  -4,  +0,  +4,  +8, +12, +16, +20, +24, +28 };
  int q_mob_mg[28] = { -14, -13, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,  +0,
	                    +1,  +2,  +3,  +4,  +5,  +6,  +7,  +8,  +9, +10, +11, +12, +13 };
  int q_mob_eg[28] = { -28, -26, -24, -22, -20, -18, -16, -14, -12, -10,  -8,  -6,  -4,  -2,  +0,
	                    +2,  +4,  +6,  +8, +10, +12, +14, +16, +18, +20, +22, +24, +26 };
  void DynamicInit(void);
  void Default(void);
} cParam; 

extern cParam Param;
