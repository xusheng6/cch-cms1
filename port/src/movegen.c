#include "cch.h"

static const int orth[4] = {-16, 16, -1, 1};
/* CMS1's palace-step loop at 337a visits right before left. */
static const int king_step[4] = {1, -1, -16, 16};
static const int horse_to[8] = {-33,-31,-18,-14,14,18,31,33};
static const int horse_leg[8] = {-16,-16,-1,1,-1,1,16,16};
static const int elephant_to[4] = {-34,-30,30,34};
static const int elephant_eye[4] = {-17,-15,15,17};
/* 3370's recursive diagonal table visits the positive-file member first
 * within each rank. Root records use a separate table, applied by order_moves. */
static const int advisor_to[4] = {-15,-17,17,15};

static bool friend_at(const CchPosition *p, int sq, CchSide side) {
    uint8_t pc = p->board[sq];
    return pc != CCH_EMPTY && pc != CCH_OFFBOARD && CCH_SIDE(pc) == side;
}
static int line_step(int from,int to){
    if(CCH_FILE(from)==CCH_FILE(to))return to>from?16:-16;
    if(CCH_RANK(from)==CCH_RANK(to))return to>from?1:-1;
    return 0;
}
static int cannon_check_screen(const CchPosition *p,CchSide victim,int *checking_cannon){
    if(checking_cannon)*checking_cannon=-1;
    int king=p->king_square[victim];CchSide attacker=(CchSide)!victim;
    for(int direction_index=0;direction_index<4;++direction_index){int step=orth[direction_index];
        int square=king+step,screen=-1;
        while(p->board[square]!=CCH_OFFBOARD){uint8_t piece=p->board[square];
            if(piece!=CCH_EMPTY){
                if(screen<0)screen=square;
                else{if(CCH_SIDE(piece)==attacker&&CCH_TYPE(piece)==CCH_CANNON){
                    if(checking_cannon)*checking_cannon=square;return screen;}break;}
            }square+=step;
        }
    }return -1;
}
static void add(const CchPosition *p, CchMoveList *l, int from, int to, bool adjustments) {
    if (l->count < CCH_MAX_MOVES) {
        int adjustment=0;
        if(adjustments){
        uint8_t pc=p->board[from];
        int checking_cannon=-1;
        int check_screen=cannon_check_screen(p,CCH_SIDE(pc),&checking_cannon);
        if(check_screen==from){
            /* 41e4..4259 tests the moving identity's one-hot type byte.
             * Rook/horse/pawn screens receive zero; elephant/advisor screens
             * receive -20. A cannon receives -60 while its own general is
             * still on the home-centre square. */
            CchPieceType type=CCH_TYPE(pc);
            if(type==CCH_ELEPHANT||type==CCH_ADVISOR)
                adjustment=CCH_SIDE(pc)==CCH_RED?-20:20;
            else if(type==CCH_CANNON){
                int home=CCH_SQUARE(4,CCH_SIDE(pc)==CCH_RED?9:0);
                if(p->king_square[CCH_SIDE(pc)]==home)
                    adjustment=CCH_SIDE(pc)==CCH_RED?-60:60;
            }
        }else if(CCH_TYPE(pc)==CCH_CANNON&&checking_cannon>=0){
            /* 4235..424d advances beyond the related cannon to the next
             * occupied ray square. A different friendly cannon capturing
             * that piece receives the fifteen-point response. */
            int king=p->king_square[CCH_SIDE(pc)];
            int step=line_step(king,checking_cannon);
            bool interposes=false;
            for(int probe=king+step;step&&probe!=check_screen;probe+=step)
                if(probe==to){interposes=true;break;}
            if(interposes)
                adjustment=CCH_SIDE(pc)==CCH_RED?-15:15;
            int beyond=checking_cannon+step;
            while(step&&p->board[beyond]==CCH_EMPTY)beyond+=step;
            if(!adjustment&&step&&beyond==to)
                adjustment=CCH_SIDE(pc)==CCH_RED?-15:15;
        }
        int response_adjustment=adjustment;
        if(CCH_TYPE(pc)==CCH_ROOK&&p->board[to]==CCH_EMPTY){
            CchSide side=CCH_SIDE(pc),other=(CchSide)!side;
            for(int ordinal=1;ordinal<=2;++ordinal){
                int square=-1;
                for(int probe=0;probe<CCH_BOARD_SIZE;++probe){
                    uint8_t opposing=p->board[probe];
                    if(opposing!=CCH_EMPTY&&opposing!=CCH_OFFBOARD&&
                       CCH_SIDE(opposing)==other&&CCH_TYPE(opposing)==CCH_ROOK&&
                       ((p->identity[probe]&0x1f)/2)==ordinal){square=probe;break;}
                }
                if(square<0)continue;
                int step=line_step(square,to);
                /* The cached 0830 relation tested here is geometric; blockers
                 * are deliberately handled by the subsequent outward scan. */
                if(!step)continue;
                /* Once the first aligned opposing identity is selected,
                 * DOS does not fall through to try the other rook. */
                if(line_step(square,from))break;
                /* DOS 4340 forms opposing_rook - destination. 4371 scans
                 * from the destination TOWARD that rook, not away from it. */
                int beyond=to-step;
                while(p->board[beyond]==CCH_EMPTY)beyond-=step;
                uint8_t tail=p->board[beyond];
                int relation;
                /* 437a..438d tests the first occupied square's horse/cannon
                 * type bits, then selects -8 for a friendly screen or +3
                 * for an opposing screen (before the colour conversion). */
                if(tail==CCH_OFFBOARD)break;
                else if(CCH_TYPE(tail)==CCH_HORSE||CCH_TYPE(tail)==CCH_CANNON)
                    relation=CCH_SIDE(tail)==side?
                        (side==CCH_RED?-8:8):(side==CCH_RED?3:-3);
                else break;
                /* 4399..43ee suppresses this relation on a home horse
                 * square when its friendly horse occupies c2/g2 (red)
                 * or c7/g7 (black). The tested type mask is 0x1000. */
                int home_rank=side==CCH_RED?9:0;
                int file=CCH_FILE(to);
                if(CCH_RANK(to)==home_rank&&(file==1||file==7)){
                    int horse_square=CCH_SQUARE(file==1?2:6,
                                               side==CCH_RED?7:2);
                    uint8_t horse=p->board[horse_square];
                    if(horse!=CCH_EMPTY&&horse!=CCH_OFFBOARD&&
                       CCH_SIDE(horse)==side&&CCH_TYPE(horse)==CCH_HORSE)
                        relation=0;
                }
                if(!adjustment)adjustment=relation;break;
            }
        }
        if(CCH_TYPE(pc)==CCH_CANNON){
            CchSide side=CCH_SIDE(pc),other=(CchSide)!side;
            int source=CCH_SQUARE(4,side==CCH_RED?7:2);
            int target=CCH_SQUARE(4,side==CCH_RED?3:6);
            int enemy_cannon=CCH_SQUARE(4,side==CCH_RED?2:7);
            int enemy_home_rank=other==CCH_RED?9:0;
            uint8_t middle=p->board[enemy_cannon];
            uint8_t left=p->board[CCH_SQUARE(1,enemy_home_rank)];
            uint8_t right=p->board[CCH_SQUARE(7,enemy_home_rank)];
            /* CMS1 425c..42d2: the central cannon-for-pawn capture receives
             * a 30-point sacrifice penalty while the opposing central cannon
             * and both home horses remain. DOS squares are 1c8->c8 and its
             * 088->188 colour mirror. */
            if(from==source&&to==target&&p->board[to]!=CCH_EMPTY&&
               CCH_SIDE(p->board[to])==other&&CCH_TYPE(p->board[to])==CCH_PAWN&&
               middle!=CCH_EMPTY&&middle!=CCH_OFFBOARD&&CCH_SIDE(middle)==other&&CCH_TYPE(middle)==CCH_CANNON&&
               left!=CCH_EMPTY&&left!=CCH_OFFBOARD&&CCH_SIDE(left)==other&&CCH_TYPE(left)==CCH_HORSE&&
               right!=CCH_EMPTY&&right!=CCH_OFFBOARD&&CCH_SIDE(right)==other&&CCH_TYPE(right)==CCH_HORSE)
                if(!adjustment)adjustment=side==CCH_RED?-30:30;
        }
        if(CCH_TYPE(pc)==CCH_HORSE&&checking_cannon<0){
            int home_rank=CCH_SIDE(pc)==CCH_RED?9-CCH_RANK(from):CCH_RANK(from);
            int destination_home=CCH_SIDE(pc)==CCH_RED?9-CCH_RANK(to):CCH_RANK(to);
            int file=CCH_FILE(from);
            /* CMS1 42d8..4326: the two home horses receive signed four for
             * either unobstructed initial development while the corner rook
             * remains. In red-minus-black form this is +4/-4. The cannon
             * check-response path at 41e4 returns before this recognizer. */
            if(home_rank==0&&(destination_home==1||destination_home==2)&&
               p->board[to]==CCH_EMPTY&&
               (file==1||file==7)){
                int rook_file=file==1?0:8;
                int rook_rank=CCH_SIDE(pc)==CCH_RED?9:0;
                uint8_t corner=p->board[CCH_SQUARE(rook_file,rook_rank)];
                if(corner!=CCH_EMPTY&&corner!=CCH_OFFBOARD&&
                   CCH_SIDE(corner)==CCH_SIDE(pc)&&CCH_TYPE(corner)==CCH_ROOK)
                    if(!adjustment)adjustment=CCH_SIDE(pc)==CCH_RED?4:-4;
            }
        }
        /* 41e4 returns before ordinary root recognizers for ANY check.
         * A non-cannon check gets zero; a cannon check keeps only its
         * dedicated response adjustment, not development/relation bonuses. */
        if(adjustment!=response_adjustment&&
           cch_is_attacked(p,p->king_square[CCH_SIDE(pc)],(CchSide)!CCH_SIDE(pc)))
            adjustment=response_adjustment;
        }
        l->moves[l->count++] = (CchMove){(uint8_t)from,(uint8_t)to,p->board[to],
                                         p->identity[from],p->identity[to],
                                         (int16_t)adjustment};
    }
}
static bool palace(CchSide side, int sq) {
    int f = CCH_FILE(sq), r = CCH_RANK(sq);
    return f >= 3 && f <= 5 && (side == CCH_BLACK ? r >= 0 && r <= 2 : r >= 7 && r <= 9);
}
static bool own_half(CchSide side, int sq) {
    int r = CCH_RANK(sq); return side == CCH_BLACK ? r <= 4 : r >= 5;
}

static void generate_pseudo(const CchPosition *p, CchMoveList *l, bool adjustments) {
    l->count = 0; CchSide side = p->side;
    for (int sq = 0; sq < CCH_BOARD_SIZE; ++sq) {
        uint8_t pc = p->board[sq];
        if (pc == CCH_EMPTY || pc == CCH_OFFBOARD || CCH_SIDE(pc) != side) continue;
        switch (CCH_TYPE(pc)) {
        case CCH_KING:
            for (int i=0;i<4;++i) { int to=sq+king_step[i]; if (palace(side,to)&&!friend_at(p,to,side)) add(p,l,sq,to,adjustments); }
            /* Flying general capture. */
            for (int to=sq+(side==CCH_RED?-16:16); p->board[to]!=CCH_OFFBOARD; to+=(side==CCH_RED?-16:16)) {
                if (p->board[to]==CCH_EMPTY) continue;
                if (CCH_TYPE(p->board[to])==CCH_KING && CCH_SIDE(p->board[to])!=side) add(p,l,sq,to,adjustments);
                break;
            }
            break;
        case CCH_ADVISOR:
            for(int i=0;i<4;++i){int to=sq+advisor_to[i];if(palace(side,to)&&!friend_at(p,to,side))add(p,l,sq,to,adjustments);} break;
        case CCH_ELEPHANT:
            for(int i=0;i<4;++i){int to=sq+elephant_to[i];if(p->board[to]!=CCH_OFFBOARD&&own_half(side,to)&&p->board[sq+elephant_eye[i]]==CCH_EMPTY&&!friend_at(p,to,side))add(p,l,sq,to,adjustments);} break;
        case CCH_HORSE:
            for(int i=0;i<8;++i){int to=sq+horse_to[i];if(p->board[to]!=CCH_OFFBOARD&&p->board[sq+horse_leg[i]]==CCH_EMPTY&&!friend_at(p,to,side))add(p,l,sq,to,adjustments);} break;
        case CCH_ROOK:
            for(int i=0;i<4;++i)for(int to=sq+orth[i];p->board[to]!=CCH_OFFBOARD;to+=orth[i]){if(p->board[to]==CCH_EMPTY){add(p,l,sq,to,adjustments);continue;}if(!friend_at(p,to,side))add(p,l,sq,to,adjustments);break;} break;
        case CCH_CANNON:
            for(int i=0;i<4;++i){int to=sq+orth[i];for(;p->board[to]!=CCH_OFFBOARD&&p->board[to]==CCH_EMPTY;to+=orth[i])add(p,l,sq,to,adjustments);if(p->board[to]==CCH_OFFBOARD)continue;for(to+=orth[i];p->board[to]!=CCH_OFFBOARD&&p->board[to]==CCH_EMPTY;to+=orth[i]){}if(p->board[to]!=CCH_OFFBOARD&&!friend_at(p,to,side))add(p,l,sq,to,adjustments);} break;
        case CCH_PAWN: {
            int forward=side==CCH_RED?-16:16,to=sq+forward;if(p->board[to]!=CCH_OFFBOARD&&!friend_at(p,to,side))add(p,l,sq,to,adjustments);
            bool crossed=side==CCH_RED?CCH_RANK(sq)<=4:CCH_RANK(sq)>=5;
            if(crossed)for(int d=-1;d<=1;d+=2){to=sq+d;if(p->board[to]!=CCH_OFFBOARD&&!friend_at(p,to,side))add(p,l,sq,to,adjustments);} break;
        }
        default: break;
        }
    }
}

void cch_generate_pseudo(const CchPosition *p, CchMoveList *l) {
    generate_pseudo(p,l,true);
}

bool cch_is_attacked(const CchPosition *p, uint8_t square, CchSide by) {
    CchPosition copy=*p; copy.side=by; CchMoveList l; generate_pseudo(&copy,&l,false);
    for(size_t i=0;i<l.count;++i)if(l.moves[i].to==square)return true;
    return false;
}

void cch_generate_legal(CchPosition *p, CchMoveList *out) {
    CchMoveList pseudo; cch_generate_pseudo(p,&pseudo); out->count=0; CchSide us=p->side;
    for(size_t i=0;i<pseudo.count;++i){CchMove m=pseudo.moves[i];cch_make_move(p,m);bool legal=!cch_is_attacked(p,p->king_square[us],p->side);cch_unmake_move(p,m);if(legal&&out->count<CCH_MAX_MOVES)out->moves[out->count++]=m;}
}

uint64_t cch_perft(CchPosition *p, unsigned depth) {
    if(!depth)return 1; CchMoveList l;cch_generate_legal(p,&l);if(depth==1)return l.count;
    uint64_t nodes=0;for(size_t i=0;i<l.count;++i){cch_make_move(p,l.moves[i]);nodes+=cch_perft(p,depth-1);cch_unmake_move(p,l.moves[i]);}return nodes;
}
