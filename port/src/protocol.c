#include "cch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    CchPosition position;
    atomic_bool stop;
    CchSearchResult result;
    pthread_t thread;
    bool running;
} InfiniteSearch;

static void print_search_info(const CchSearchResult *r,void *unused){
    (void)unused;
    printf("info depth %u score cp %d nodes %llu pv",r->depth,r->score,
           (unsigned long long)r->nodes);
    for(unsigned i=0;i<r->pv_length;++i){char move[5];cch_format_move(r->pv[i],move);printf(" %s",move);}
    putchar('\n');fflush(stdout);
}
static void *run_infinite_search(void *opaque){
    InfiniteSearch *search=opaque;
    search->result=cch_search_stream(&search->position,CCH_CMS1_MAX_SEARCH_PLY,
                                     &search->stop,print_search_info,NULL);
    char move[5];cch_format_move(search->result.best_move,move);
    printf("bestmove %s\n",search->result.depth?move:"0000");fflush(stdout);
    return NULL;
}

static void apply_moves(CchPosition *p,char *moves,const CchBook *book,uint16_t *cursor,bool *in_book){
    for(char *tok=strtok(moves," \t\r\n");tok;tok=strtok(NULL," \t\r\n")){CchMove m;if(!cch_parse_move(p,tok,&m))break;
        if(*in_book&&!cch_book_advance(book,cursor,m))*in_book=false;cch_make_move(p,m);}
}
static void set_position(CchPosition *p,char *line,const CchBook *book,uint16_t *cursor,bool *in_book){
    char *moves=strstr(line," moves ");if(moves){*moves=0;moves+=7;}
    *cursor=0;*in_book=!strncmp(line,"startpos",8)&&book->count;
    if(!strncmp(line,"startpos",8))cch_position_start(p);else if(!strncmp(line,"fen ",4))cch_position_from_fen(p,line+4);
    if(moves)apply_moves(p,moves,book,cursor,in_book);
}
typedef struct { unsigned depth; uint64_t remaining,increment; unsigned moves_to_go; bool has_time,has_depth,infinite; } GoLimits;
static GoLimits parse_go(const char *line,CchSide side,bool use_millisec){
    GoLimits g={.depth=8};char copy[2048];snprintf(copy,sizeof copy,"%s",line);
    char marked[2052];snprintf(marked,sizeof marked," %s ",line);
    g.infinite=strstr(marked," infinite ")!=NULL;
    uint64_t wtime=0,btime=0,winc=0,binc=0,generic_time=0,generic_inc=0;
    bool have_wtime=false,have_btime=false,have_generic=false;
    char *tok=strtok(copy," \t");
    while(tok){
        char *name=tok;tok=strtok(NULL," \t");if(!tok)break;
        if(!strcmp(name,"depth")){g.depth=(unsigned)strtoul(tok,NULL,10);g.has_depth=true;}
        else if(!strcmp(name,"wtime")){wtime=strtoull(tok,NULL,10);have_wtime=true;}
        else if(!strcmp(name,"btime")){btime=strtoull(tok,NULL,10);have_btime=true;}
        else if(!strcmp(name,"winc"))winc=strtoull(tok,NULL,10);
        else if(!strcmp(name,"binc"))binc=strtoull(tok,NULL,10);
        else if(!strcmp(name,"time")){generic_time=strtoull(tok,NULL,10);have_generic=true;}
        else if(!strcmp(name,"increment"))generic_inc=strtoull(tok,NULL,10);
        else if(!strcmp(name,"movestogo"))g.moves_to_go=(unsigned)strtoul(tok,NULL,10);
        else continue;
        tok=strtok(NULL," \t");
    }
    if(side==CCH_RED&&have_wtime){g.remaining=wtime;g.increment=winc;g.has_time=true;}
    else if(side==CCH_BLACK&&have_btime){g.remaining=btime;g.increment=binc;g.has_time=true;}
    else if(have_generic){g.remaining=generic_time;g.increment=generic_inc;g.has_time=true;}
    if(g.has_time&&!use_millisec){g.remaining*=1000;g.increment*=1000;}
    /* CMS1 reserves 0x2700 bytes of 0x100-byte PV frames, so its hard
     * recursive ceiling is 38 plies beyond the root frame.  Keep explicit
     * UCI/UCCI depth requests within that recovered limit rather than the
     * port's former arbitrary depth-10 cap. */
    if(g.depth>CCH_CMS1_MAX_SEARCH_PLY)g.depth=CCH_CMS1_MAX_SEARCH_PLY;
    return g;
}
int cch_protocol_loop(void){
    CchPosition p;cch_position_start(&p);char line[2048];CchBook book={0};uint16_t cursor=0;bool in_book=false;
    InfiniteSearch infinite={0};atomic_init(&infinite.stop,false);
    bool use_millisec=true;
    int profile_index=-1;
    const char *book_path=getenv("CCH_BOOK");if(book_path)cch_book_load(&book,book_path);
    else if(cch_book_load(&book,"OPENING.LIB")||cch_book_load(&book,"CCH/OPENING.LIB"))in_book=true;
    while(fgets(line,sizeof line,stdin)){
        line[strcspn(line,"\r\n")]=0;
        if(infinite.running){
            atomic_store(&infinite.stop,true);pthread_join(infinite.thread,NULL);
            infinite.running=false;
            if(!strcmp(line,"stop"))continue;
        }
        if(!strcmp(line,"ucci")||!strcmp(line,"uci")){
            bool ucci=!strcmp(line,"ucci");
            printf("id name CCH %s Port%s\n",cch_modern_mode()?"Modern":"Preservation",ucci?"":" (Xiangqi)");
            puts("id author Yu Hsi-Shun (original); preservation rewrite contributors");
            if(ucci)puts("option usemillisec type check default true");
            printf("option name Profile type combo default Unlimited var Unlimited");
            for(size_t i=0;i<cch_profile_count();++i)printf(" var %s",cch_profile(i)->name);
            putchar('\n');puts(ucci?"ucciok":"uciok");
        }
        else if(!strcmp(line,"isready"))puts("readyok");
        else if(!strncmp(line,"field ",6)){
            char colour[16],square[8];int type;
            if(sscanf(line+6,"%15s %d %7s",colour,&type,square)==3&&
               type>=CCH_KING&&type<=CCH_PAWN&&strlen(square)>=2){
                CchSide side=!strcmp(colour,"black")?CCH_BLACK:CCH_RED;
                int file=square[0]-'a',rank=9-(square[1]-'0');
                if(file>=0&&file<9&&rank>=0&&rank<10)
                    printf("field %s type=%d square=%s value=%d\n",colour,type,square,
                           p.cms1_fields[side][type][rank*9+file]);
            }
        }
        else if(!strcmp(line,"profiles")){
            puts("profile Unlimited mode=protocol-controlled");
            for(size_t i=0;i<cch_profile_count();++i){const CchProfile *profile=cch_profile(i);
                unsigned clock=cch_profile_clock_argument(profile);
                if(!clock)printf("profile %s mode=fixed original_ply=2 uci_depth=2 quiescence=captures max_recursive_plies=38\n",profile->name);
                else printf("profile %s mode=clock whole_game_minutes=%u initial_ticks=%u initial_seconds=%.1f max_iteration=30 max_recursive_plies=38\n",
                            profile->name,clock,clock*27u,clock*1.5);}
        }
        else if(!strncmp(line,"setoption name usemillisec value ",33)){
            use_millisec=strcmp(line+33,"false")&&strcmp(line+33,"0");
        }
        /* UCCI uses `setoption <name> <value>` rather than UCI's
         * `setoption name <name> value <value>`. */
        else if(!strncmp(line,"setoption usemillisec ",23)){
            use_millisec=strcmp(line+23,"false")&&strcmp(line+23,"0");
        }
        else if(!strncmp(line,"setoption name Profile value ",29)){
            profile_index=!strcmp(line+29,"Unlimited")?-1:cch_profile_find(line+29);
        }
        else if(!strcmp(line,"ucinewgame")||!strcmp(line,"newgame")){cch_position_start(&p);cursor=0;in_book=book.count!=0;}
        else if(!strncmp(line,"position ",9))set_position(&p,line+9,&book,&cursor,&in_book);
        else if(!strncmp(line,"go",2)){
            GoLimits limits=parse_go(line+2,p.side,use_millisec);uint64_t ms=0;
            if(limits.infinite){
                in_book=false;infinite.position=p;atomic_store(&infinite.stop,false);
                if(!pthread_create(&infinite.thread,NULL,run_infinite_search,&infinite)){
                    infinite.running=true;continue;
                }
                /* A thread failure is exceptional; retain a bounded
                 * synchronous fallback so the protocol still answers. */
                limits.depth=CCH_CMS1_MAX_SEARCH_PLY;ms=1500;
            }
            if(profile_index>=0&&!limits.has_time&&!limits.has_depth){
                uint16_t clock=cch_profile_clock_argument(cch_profile((size_t)profile_index));
                /* The root controller makes the candidate and passes PLY-1
                 * to 15d3. Its pre-decrement leaf test therefore gives the
                 * displayed PLY its conventional root-inclusive meaning. */
                if(!clock)limits.depth=2;
                else{
                    uint16_t ticks=cch_cms1_time_budget_ticks(clock,0,p.ply);
                    ms=(uint64_t)ticks*1000u/18u;
                    limits.depth=CCH_CMS1_MAX_SEARCH_PLY;
                }
            }
            if(limits.has_time){
                unsigned fullmoves=p.ply/2u;
                unsigned moves_remaining=limits.moves_to_go?limits.moves_to_go:(fullmoves<60u?60u-fullmoves:1u);
                ms=limits.remaining/moves_remaining;
                if(moves_remaining>=40u)ms=ms*3u/2u;
                ms+=limits.increment/2;if(ms<10)ms=10;}
            CchSearchResult r={0};CchMove bm;size_t choices=in_book?cch_book_choice_count(&book,cursor):0;
            /* CMS1 5578..559b chooses (entropy % eligible_siblings)+1. */
            size_t choice=choices?(size_t)time(NULL)%choices:0;
            if(in_book&&choices&&cch_book_get_move_choice(&book,cursor,choice,&p,&bm)){
                r.best_move=bm;r.pv[0]=bm;r.pv_length=1;r.depth=1;
            }
            else{in_book=false;
                if(profile_index>=0&&!limits.has_time&&!limits.has_depth)
                    r=cch_search_cms1_limit(&p,cch_profile_clock_argument(cch_profile((size_t)profile_index))?30:2,ms);
                else r=ms?cch_search_timed(&p,limits.depth,ms):cch_search(&p,limits.depth);
            }char mv[5];cch_format_move(r.best_move,mv);
            printf("info depth %u score cp %d nodes %llu",r.depth,r.score,(unsigned long long)r.nodes);
            if(r.depth){fputs(" pv",stdout);for(unsigned i=0;i<r.pv_length;++i){char pv_move[5];cch_format_move(r.pv[i],pv_move);printf(" %s",pv_move);}}
            putchar('\n');printf("bestmove %s\n",r.depth?mv:"0000");
        } else if(!strncmp(line,"perft ",6)){unsigned d=(unsigned)strtoul(line+6,NULL,10);printf("nodes %llu\n",(unsigned long long)cch_perft(&p,d));}
        else if(!strcmp(line,"legalmoves")){CchMoveList moves;cch_generate_legal(&p,&moves);
            fputs("legalmoves",stdout);for(size_t i=0;i<moves.count;++i){char move[5];cch_format_move(moves.moves[i],move);printf(" %s",move);}putchar('\n');}
        else if(!strcmp(line,"terms")){for(int rank=0;rank<10;++rank)for(int file=0;file<9;++file){
            int square=CCH_SQUARE(file,rank);uint8_t piece=p.board[square];
            if(piece==CCH_EMPTY||piece==CCH_OFFBOARD)continue;
            int term=p.cms1_fields_frozen?p.cms1_fields[CCH_SIDE(piece)][CCH_TYPE(piece)][rank*9+file]:0;
            if(CCH_TYPE(piece)==CCH_KING)term+=5000;
            printf("term %c%c side=%d type=%d value=%d\n",'a'+file,'9'-rank,CCH_SIDE(piece),CCH_TYPE(piece),term);}}
        else if(!strcmp(line,"eval"))printf("eval %d\n",cch_evaluate(&p));
        else if(!strcmp(line,"d")){char fen[128];cch_position_to_fen(&p,fen,sizeof fen);puts(fen);}
        else if(!strcmp(line,"stop")){}
        else if(!strcmp(line,"quit"))break;
        fflush(stdout);
    }if(infinite.running){atomic_store(&infinite.stop,true);pthread_join(infinite.thread,NULL);}
    cch_book_free(&book);return 0;
}
