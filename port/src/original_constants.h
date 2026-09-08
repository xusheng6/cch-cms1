#ifndef CCH_ORIGINAL_CONSTANTS_H
#define CCH_ORIGINAL_CONSTANTS_H

/*
 * Constants recovered from CMS1.EXE (SHA-1
 * 04f50af2063a487fce6f905e52f6edd1ef20ecf3).
 *
 * The MZ image sets DS=ES to load-segment+0x056c.  Consequently a DOS data
 * address A is stored at file offset 0x200 + 0x56c0 + A.  Keep that mapping
 * here so these numbers are distinguishable from modern tuning choices.
 */

/* Immutable mailbox destination flags at DS:1031+square. Contrary to the
 * earlier attack-cache hypothesis, runtime write tracing shows no writes to
 * this bank during move application, evaluation rebuilds, or search. Bits
 * correspond directly to CMS1's piece flags: 80 king-palace, 40 rook,
 * 20 cannon, 10 horse, 08 elephant point, 04 advisor point, 02 Red soldier,
 * and 01 Black soldier. Displacement tables provide the other half of each
 * geometry test. */
static const unsigned char cch_cms1_destination_flags[10][9] = {
 {0x72,0x72,0x7a,0xf6,0xf2,0xf6,0x7a,0x72,0x72},
 {0x72,0x72,0x72,0xf2,0xf6,0xf2,0x72,0x72,0x72},
 {0x7a,0x72,0x72,0xf6,0xfa,0xf6,0x72,0x72,0x7a},
 {0x73,0x72,0x73,0x72,0x73,0x72,0x73,0x72,0x73},
 {0x73,0x72,0x7b,0x72,0x73,0x72,0x7b,0x72,0x73},
 {0x73,0x71,0x7b,0x71,0x73,0x71,0x7b,0x71,0x73},
 {0x73,0x71,0x73,0x71,0x73,0x71,0x73,0x71,0x73},
 {0x79,0x71,0x71,0xf5,0xf9,0xf5,0x71,0x71,0x79},
 {0x71,0x71,0x71,0xf1,0xf5,0xf1,0x71,0x71,0x71},
 {0x71,0x71,0x79,0xf5,0xf1,0xf5,0x79,0x71,0x71}
};

/* DS:3b6e, consumed as ten rows of nine signed words by 0000:0dcc. */
static const short cch_cms1_horse_square[10][9] = {
    {300, 323, 325, 300, 300, 300, 325, 323, 300},
    {328, 335, 338, 341, 260, 341, 338, 335, 328},
    {347, 345, 352, 349, 345, 349, 352, 345, 347},
    {345, 358, 360, 362, 364, 362, 360, 358, 345},
    {360, 366, 368, 373, 374, 373, 368, 366, 360},
    {355, 375, 381, 378, 386, 378, 381, 375, 355},
    {360, 385, 379, 390, 381, 390, 379, 385, 360},
    {370, 380, 382, 390, 388, 390, 382, 380, 370},
    {365, 378, 390, 381, 375, 381, 390, 378, 365},
    {355, 366, 368, 368, 365, 368, 368, 366, 355}
};
/* CMS1 DS:3c22, consumed by 0fce..100c. Odd entries select the inward
 * vertical neighbour; even entries select the inward horizontal neighbour. */
static const signed char cch_cms1_horse_inward_orientation[10][9] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1},
    { 2, 1, 1, 1, 1, 1, 1, 1,-2},
    { 1, 1, 1, 1, 1, 1, 1, 1, 1},
    { 2, 1, 1, 1, 1, 1, 1, 1,-2},
    { 2, 1, 1, 1, 1, 1, 1, 1,-2},
    { 2, 1, 1, 1, 1, 1, 1, 1,-2},
    { 2, 1, 1, 1, 1, 1, 1, 1,-2},
    { 2, 2, 2, 1, 1, 1,-2,-2,-2},
    { 2, 2, 2, 2,-1,-2,-2,-2,-2},
    { 2, 2,-1,-1,-1,-1,-1,-2,-2}
};

/* DS:3cfe and DS:3db2, copied by 0000:0fc1 and sign-mirrored at 1153. */
static const short cch_cms1_rook_square[10][9] = {
    {745,765,753,770,751,770,753,765,745},
    {764,772,768,778,759,778,768,772,764},
    {761,770,765,776,762,776,765,770,761},
    {760,769,768,775,770,775,768,769,760},
    {772,779,778,785,783,785,778,779,772},
    {769,775,774,781,781,781,774,775,769},
    {770,777,775,782,782,782,775,777,770},
    {773,775,774,781,782,781,774,775,773},
    {775,779,776,783,790,783,776,779,775},
    {773,776,774,781,781,781,774,776,773}
};

static const short cch_cms1_cannon_square[10][9] = {
    {365,366,366,367,370,367,366,366,365},
    {366,367,368,367,365,367,368,367,366},
    {366,368,369,372,373,372,369,368,366},
    {362,364,366,367,371,367,366,364,362},
    {357,365,365,365,370,365,365,365,357},
    {360,365,365,365,369,365,365,365,360},
    {360,365,365,365,368,365,365,365,360},
    {359,359,358,357,359,357,358,359,359},
    {360,359,358,357,355,357,358,359,360},
    {360,359,358,357,355,357,358,359,360}
};

/* Effective positive-side fields after CMS1's normal-game initializer. These
 * were read from DS:29f2/2cb2/2f72 after executing 0a28..139f in the original
 * x86. CMS1 freezes these fields for the game and updates DS:1350 by deltas. */
static const short cch_cms1_initial_king_field[10][9] = {
 {0,0,0,5009,5022,5009,0,0,0},{0,0,0,5006,4987,5006,0,0,0},
 {0,0,0,5003,5005,5003,0,0,0},{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},
 {0,0,0,5015,5015,5015,0,0,0},{0,0,0,5015,5015,5015,0,0,0},
 {0,0,0,5015,5015,5015,0,0,0}
};
static const short cch_cms1_initial_rook_field[10][9] = {
 {745,765,753,770,751,770,753,765,745},{764,772,768,778,759,778,768,772,764},
 {761,770,765,776,762,776,765,770,761},{760,769,768,775,770,775,768,769,760},
 {772,779,778,785,783,785,778,779,772},{769,775,774,781,781,781,774,775,769},
 {770,777,775,782,802,782,775,777,770},{773,775,774,781,802,781,774,775,773},
 {785,869,786,793,834,793,786,869,785},{773,776,777,820,835,820,777,776,773}
};
static const short cch_cms1_initial_cannon_field[10][9] = {
 {365,366,366,367,370,367,366,366,365},{366,367,368,367,365,367,368,367,366},
 {366,368,369,372,373,372,369,368,366},{362,364,366,367,371,367,366,364,362},
 {357,365,365,365,370,365,365,365,357},{360,365,365,365,444,365,365,365,360},
 {360,365,365,365,468,365,365,365,360},{359,359,358,357,459,357,358,359,359},
 {360,359,358,357,405,357,358,359,360},{360,364,373,337,325,337,373,364,360}
};
static const short cch_cms1_initial_horse_field[10][9] = {
 {306,329,331,306,306,306,331,329,306},{334,335,344,347,266,347,344,335,334},
 {347,351,352,355,345,355,352,351,347},{351,364,366,368,370,368,366,364,351},
 {366,372,374,379,380,379,374,372,366},{361,381,381,384,386,384,381,381,361},
 {366,385,385,404,387,404,385,385,366},{370,386,391,408,404,408,391,386,370},
 {371,389,408,392,381,392,408,389,371},{355,366,379,374,381,374,379,366,355}
};
static const short cch_cms1_initial_elephant_field[10][9] = {
 {0,0,126,0,0,0,126,0,0},{0,0,0,0,0,0,0,0,0},{124,0,0,0,130,0,0,0,124},
 {0,0,0,0,0,0,0,0,0},{0,0,126,0,0,0,126,0,0},{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0}
};
static const short cch_cms1_initial_advisor_field[10][9] = {
 {0,0,0,125,0,125,0,0,0},{0,0,0,0,128,0,0,0,0},{0,0,0,125,0,125,0,0,0},
 {0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0}
};
static const short cch_cms1_initial_pawn_field[10][9] = {
 {0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0},
 {15,0,17,0,20,0,17,0,15},{17,0,21,0,23,0,21,0,17},
 {65,75,80,83,85,83,80,75,65},{70,79,90,93,93,93,90,79,70},
 {67,78,115,132,132,132,115,78,67},{65,75,130,170,172,170,130,75,65},
 {50,65,85,118,120,118,85,65,50}
};

/* DS:3ac0 and DS:3ad4. The latter deliberately has 77 entries: palace-file
 * and palace-rank selection can address its final two overlap values. */
static const short cch_cms1_soldier_uncrossed[2][5] = {
    {15,17,20,17,15},
    {17,21,23,21,17}
};

static const short cch_cms1_soldier_king_relative[77] = {
    60,65,75,80,83,85,83,80,75,65,60,
    60,70,79,90,93,93,93,90,79,70,60,
    55,67,78,115,132,132,132,115,78,67,55,
    45,65,75,130,170,172,170,130,75,65,45,
    30,50,65,85,118,120,118,85,65,50,30,
    20,30,45,75,90,90,90,75,45,30,20,
    10,15,30,55,60,60,60,55,30,15,10
};

/* DS:3aae, addressed by the even weighted-material count as a byte offset. */
static const short cch_cms1_material_adjustment[9] = {
    80,75,70,50,32,18,9,2,0
};

/* DS:3a9a: nine general-square values copied into a 3x3 palace. */
static const short cch_cms1_general_palace[3][3] = {
    {5009, 5012, 5009},
    {5006, 5007, 5006},
    {5003, 5005, 5003}
};

enum {
    CCH_CMS1_GENERAL_OTHER = 5015, /* DS:3aac */
    CCH_CMS1_ELEPHANT_BASE = 130,
    CCH_CMS1_ADVISOR_BASE = 125,
    CCH_CMS1_HANDICAP_DELTA = 90,
    CCH_CMS1_SEARCH_NEG_INFINITY = -32767, /* 0x8001 */
    CCH_CMS1_SEARCH_POS_INFINITY = 32767,
    CCH_CMS1_ASPIRATION_UPPER_MARGIN = 50,
    CCH_CMS1_ASPIRATION_LOWER_MARGIN = 56,
    CCH_CMS1_INITIAL_LOWER_MARGIN = 80,
    CCH_CMS1_ROOT_DEPTH_BONUS = 2,
    CCH_CMS1_TERMINAL_BASE = 10000 /* signed 0xd8f0; stack offset is added */
};

#endif
