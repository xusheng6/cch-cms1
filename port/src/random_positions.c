#include "cch.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t state;

static uint32_t random_word(void) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return (uint32_t)((state * UINT64_C(2685821657736338717)) >> 32);
}

int main(int argc, char **argv) {
    unsigned count = argc > 1 ? (unsigned)strtoul(argv[1], NULL, 0) : 500;
    state = argc > 2 ? strtoull(argv[2], NULL, 0) : UINT64_C(0xc0dec0de);
    if (!state) state = 1;
    puts("[");
    for (unsigned item = 0; item < count; ++item) {
        CchPosition position;
        CchMove history[40];
        unsigned wanted_parity = item & 1;
        unsigned length = 2 + random_word() % 39;
        if ((length & 1) != wanted_parity) length += length == 40 ? -1 : 1;
        for (;;) {
            cch_position_start(&position);
            unsigned ply = 0;
            for (; ply < length; ++ply) {
                CchMoveList moves;
                cch_generate_legal(&position, &moves);
                size_t usable = 0;
                for (size_t i = 0; i < moves.count; ++i)
                    if (CCH_TYPE(moves.moves[i].captured) != CCH_KING)
                        moves.moves[usable++] = moves.moves[i];
                if (!usable) break;
                CchMove move = moves.moves[random_word() % usable];
                history[ply] = move;
                cch_make_move(&position, move);
            }
            if (ply == length) break;
            length = 2 + random_word() % 39;
            if ((length & 1) != wanted_parity) length += length == 40 ? -1 : 1;
        }
        unsigned band = item % 10;
        unsigned depth = band < 6 ? 2 : band < 9 ? 3 : 4;
        printf("  {\"case\":%u,\"depth\":%u,\"moves\":[", item + 1, depth);
        for (unsigned ply = 0; ply < length; ++ply) {
            char text[5];
            cch_format_move(history[ply], text);
            printf("%s\"%s\"", ply ? "," : "", text);
        }
        printf("],\"side_selector\":\"%s\"}%s\n",
               position.side == CCH_RED ? "0x40" : "0x20",
               item + 1 == count ? "" : ",");
    }
    puts("]");
    return 0;
}
