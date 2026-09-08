#ifndef CCH_H
#define CCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

enum {
    CCH_BOARD_SIZE = 256,
    CCH_MAX_MOVES = 256,
    CCH_MAX_HISTORY = 1024,
    /* CMS1 stops before allocating the 0x2700 PV frame. */
    CCH_CMS1_MAX_SEARCH_PLY = 38
};

typedef enum { CCH_RED = 0, CCH_BLACK = 1 } CchSide;
typedef enum {
    CCH_EMPTY = 0, CCH_KING = 1, CCH_ADVISOR = 2, CCH_ELEPHANT = 3,
    CCH_HORSE = 4, CCH_ROOK = 5, CCH_CANNON = 6, CCH_PAWN = 7,
    CCH_OFFBOARD = 0x7f
} CchPieceType;

/* Low nibble is type; bit 4 is side. This is the port's representation,
 * intentionally isolated from the still-being-recovered DOS encoding. */
#define CCH_PIECE(side, type) ((uint8_t)((type) | ((side) << 4)))
#define CCH_TYPE(piece) ((CchPieceType)((piece) & 0x0f))
#define CCH_SIDE(piece) ((CchSide)(((piece) >> 4) & 1))
#define CCH_SQUARE(file, rank) ((uint8_t)(((rank) + 3) * 16 + ((file) + 3)))
#define CCH_FILE(square) (((square) & 15) - 3)
#define CCH_RANK(square) (((square) >> 4) - 3)

typedef struct {
    uint8_t from, to, captured;
    uint8_t moving_identity, captured_identity;
    /* Red-minus-black score term from CMS1's exceptional generator word. */
    int16_t evaluation_adjustment;
} CchMove;
typedef struct { CchMove moves[CCH_MAX_MOVES]; size_t count; } CchMoveList;
typedef struct {
    uint8_t board[CCH_BOARD_SIZE];
    /* Persistent CMS1 identity byte (20..5e), independent of current square. */
    uint8_t identity[CCH_BOARD_SIZE];
    CchSide side;
    uint8_t king_square[2];
    unsigned ply;
    /* CMS1 retains eight-byte move records and scans backward to the most
     * recent capture when constructing reversible-cycle candidates. */
    CchMove history[CCH_MAX_HISTORY];
    /* True when CMS1's square fields were initialized from its normal start. */
    bool cms1_standard_fields;
    /* CMS1 rebuilds side-specific fields before thinking, then holds them
     * constant for every node in that search. */
    bool cms1_fields_frozen;
    int16_t cms1_fields[2][8][90];
    int16_t cms1_frozen_guard_balance;
    int evaluation_adjustment;
} CchPosition;

typedef struct {
    uint64_t nodes;
    int score;
    unsigned depth;
    CchMove best_move;
    CchMove pv[CCH_CMS1_MAX_SEARCH_PLY + 1];
    unsigned pv_length;
} CchSearchResult;
typedef void (*CchSearchInfoCallback)(const CchSearchResult *result, void *user);

typedef struct { uint16_t alternate, continuation; uint8_t from, to; } CchBookNode;
typedef struct { CchBookNode *nodes; size_t count; } CchBook;
typedef struct { const char *name; uint8_t display_power; uint16_t raw_control; } CchProfile;

void cch_position_clear(CchPosition *position);
void cch_position_start(CchPosition *position);
bool cch_position_from_fen(CchPosition *position, const char *fen);
void cch_position_to_fen(const CchPosition *position, char *out, size_t size);
void cch_generate_pseudo(const CchPosition *position, CchMoveList *list);
void cch_generate_legal(CchPosition *position, CchMoveList *list);
bool cch_is_attacked(const CchPosition *position, uint8_t square, CchSide by_side);
void cch_make_move(CchPosition *position, CchMove move);
void cch_unmake_move(CchPosition *position, CchMove move);
uint64_t cch_perft(CchPosition *position, unsigned depth);
int cch_evaluate(const CchPosition *position);
void cch_set_modern_mode(bool enabled);
bool cch_modern_mode(void);
CchSearchResult cch_search(CchPosition *position, unsigned depth);
CchSearchResult cch_search_timed(CchPosition *position, unsigned max_depth, uint64_t milliseconds);
CchSearchResult cch_search_stream(CchPosition *position, unsigned max_depth,
                                  const atomic_bool *stop,
                                  CchSearchInfoCallback callback, void *user);
CchSearchResult cch_search_cms1_limit(CchPosition *position,
                                      unsigned maximum_original_ply,
                                      uint64_t milliseconds);
uint16_t cch_cms1_time_budget_ticks(uint16_t configured_minutes,
                                    uint32_t elapsed_seconds, unsigned ply);
bool cch_parse_move(const CchPosition *position, const char *text, CchMove *move);
void cch_format_move(CchMove move, char out[5]);
int cch_protocol_loop(void);
bool cch_book_load(CchBook *book, const char *path);
void cch_book_free(CchBook *book);
const CchBookNode *cch_book_node(const CchBook *book, uint16_t index);
bool cch_book_get_move(const CchBook *book, uint16_t cursor, const CchPosition *position, CchMove *move);
size_t cch_book_choice_count(const CchBook *book, uint16_t cursor);
bool cch_book_get_move_choice(const CchBook *book, uint16_t cursor, size_t choice,
                              const CchPosition *position, CchMove *move);
bool cch_book_advance(const CchBook *book, uint16_t *cursor, CchMove move);
size_t cch_profile_count(void);
const CchProfile *cch_profile(size_t index);
int cch_profile_find(const char *name);
uint16_t cch_profile_clock_argument(const CchProfile *profile);

#endif
