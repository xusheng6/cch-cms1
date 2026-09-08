#include "cch.h"
#include <stdio.h>
#include <string.h>

static bool playable(int file, int rank) {
    return file >= 0 && file < 9 && rank >= 0 && rank < 10;
}

void cch_position_clear(CchPosition *p) {
    memset(p, 0, sizeof(*p));
    for (int i = 0; i < CCH_BOARD_SIZE; ++i) {
        p->board[i] = CCH_OFFBOARD;p->identity[i]=0xff;
    }
    for (int r = 0; r < 10; ++r)
        for (int f = 0; f < 9; ++f) {
            p->board[CCH_SQUARE(f, r)] = CCH_EMPTY;
            p->identity[CCH_SQUARE(f,r)]=0xff;
        }
    p->king_square[0] = p->king_square[1] = 0xff;
    p->side = CCH_RED;
}

static CchPieceType fen_type(char c) {
    switch (c >= 'a' && c <= 'z' ? c - 32 : c) {
        case 'K': return CCH_KING; case 'A': return CCH_ADVISOR;
        case 'B': case 'E': return CCH_ELEPHANT; case 'N': case 'H': return CCH_HORSE;
        case 'R': return CCH_ROOK; case 'C': return CCH_CANNON; case 'P': return CCH_PAWN;
        default: return CCH_EMPTY;
    }
}

bool cch_position_from_fen(CchPosition *p, const char *fen) {
    cch_position_clear(p);
    static const uint8_t pools[2][8][5]={
        [CCH_BLACK]={{0},{0x20},{0x32,0x34},{0x2e,0x30},{0x2a,0x2c},
                     {0x22,0x24},{0x26,0x28},{0x3c,0x38,0x36,0x3a,0x3e}},
        [CCH_RED]={{0},{0x40},{0x54,0x52},{0x50,0x4e},{0x4c,0x4a},
                   {0x44,0x42},{0x48,0x46},{0x5c,0x58,0x56,0x5a,0x5e}}
    };
    uint8_t used[2][8]={{0}};
    int rank = 0, file = 0;
    for (; *fen && *fen != ' '; ++fen) {
        if (*fen == '/') { if (file != 9) return false; ++rank; file = 0; continue; }
        if (*fen >= '1' && *fen <= '9') { file += *fen - '0'; continue; }
        CchPieceType type = fen_type(*fen);
        if (!type || !playable(file, rank)) return false;
        CchSide side = (*fen >= 'a' && *fen <= 'z') ? CCH_BLACK : CCH_RED;
        uint8_t sq = CCH_SQUARE(file++, rank);
        p->board[sq] = CCH_PIECE(side, type);
        unsigned index=used[side][type]++;
        if(index>=5||pools[side][type][index]==0)return false;
        p->identity[sq]=pools[side][type][index];
        if (type == CCH_KING) p->king_square[side] = sq;
    }
    if (rank != 9 || file != 9 || p->king_square[0] == 0xff || p->king_square[1] == 0xff) return false;
    while (*fen == ' ') ++fen;
    p->side = (*fen == 'b') ? CCH_BLACK : CCH_RED;
    /* A GUI may express the untouched start position as a FEN (notably to
     * put Black to move). CMS1 would still be using the square fields frozen
     * by its normal initializer, so recognize that layout independent of the
     * side token instead of falling back to the custom-position evaluator. */
    char normalized[128];cch_position_to_fen(p,normalized,sizeof normalized);
    p->cms1_standard_fields=
        !strcmp(normalized,"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w")||
        !strcmp(normalized,"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR b");
    return true;
}

void cch_position_start(CchPosition *p) {
    (void)cch_position_from_fen(p, "rheakaehr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RHEAKAEHR w");
}

void cch_position_to_fen(const CchPosition *p, char *out, size_t size) {
    static const char red[] = " KABNRCP", black[] = " kabnrcp";
    size_t n = 0;
#define PUT(ch) do { if (n + 1 < size) out[n] = (ch); ++n; } while (0)
    for (int r = 0; r < 10; ++r) {
        int empty = 0;
        for (int f = 0; f < 9; ++f) {
            uint8_t pc = p->board[CCH_SQUARE(f, r)];
            if (pc == CCH_EMPTY) { ++empty; continue; }
            if (empty) { PUT((char)('0' + empty)); empty = 0; }
            PUT((CCH_SIDE(pc) == CCH_RED ? red : black)[CCH_TYPE(pc)]);
        }
        if (empty) PUT((char)('0' + empty));
        if (r != 9) PUT('/');
    }
    PUT(' '); PUT(p->side == CCH_RED ? 'w' : 'b');
    if (size) out[n < size ? n : size - 1] = '\0';
#undef PUT
}

void cch_make_move(CchPosition *p, CchMove m) {
    if(p->ply<CCH_MAX_HISTORY)p->history[p->ply]=m;
    uint8_t pc = p->board[m.from];
    p->board[m.to] = pc; p->board[m.from] = CCH_EMPTY;
    p->identity[m.to]=p->identity[m.from];p->identity[m.from]=0xff;
    if (CCH_TYPE(pc) == CCH_KING) p->king_square[CCH_SIDE(pc)] = m.to;
    p->side = (CchSide)!p->side; ++p->ply;
}

void cch_unmake_move(CchPosition *p, CchMove m) {
    p->side = (CchSide)!p->side; --p->ply;
    uint8_t pc = p->board[m.to];
    p->board[m.from] = pc; p->board[m.to] = m.captured;
    p->identity[m.from]=m.moving_identity;
    p->identity[m.to]=m.captured_identity;
    if (CCH_TYPE(pc) == CCH_KING) p->king_square[CCH_SIDE(pc)] = m.from;
}
