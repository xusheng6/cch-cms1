#include "cch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void play(CchPosition *p,const char *notation){
    CchMove move;assert(cch_parse_move(p,notation,&move));cch_make_move(p,move);
}

int main(int argc,char **argv) {
    CchPosition p; char fen[128]; cch_position_start(&p); cch_position_to_fen(&p,fen,sizeof fen);
    assert(!strcmp(fen,"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w"));
    assert(p.identity[CCH_SQUARE(0,0)]==0x22);
    assert(p.identity[CCH_SQUARE(8,0)]==0x24);
    assert(p.identity[CCH_SQUARE(0,9)]==0x44);
    assert(p.identity[CCH_SQUARE(8,9)]==0x42);
    assert(cch_perft(&p,1)==44);
    assert(cch_perft(&p,2)==1920);
    assert(cch_perft(&p,3)==79666);
    assert(cch_perft(&p,4)==3290240);
    assert(cch_evaluate(&p)==0);
    CchPosition guard_test;
    assert(cch_position_from_fen(&guard_test,"r3k4/9/9/9/9/9/9/9/9/2B1K4 w"));
    /* Original DOS initializer reports black-minus-red 684 and 672 for
     * these synthetic evaluation-only FENs (the generals face each other). */
    assert(cch_evaluate(&guard_test)==-684);
    assert(cch_position_from_fen(&guard_test,"r3k4/9/9/9/9/9/9/9/9/3AK4 w"));
    assert(cch_evaluate(&guard_test)==-672);
    assert(cch_position_from_fen(&guard_test,"4k4/9/9/9/4P4/9/9/9/9/3K5 w"));
    assert(cch_evaluate(&guard_test)==162);
    assert(cch_position_from_fen(&guard_test,"9/9/4k4/9/4P4/9/9/9/9/3K5 w"));
    assert(cch_evaluate(&guard_test)==216);
    CchPosition invalid;assert(!cch_position_from_fen(&invalid,"9/9 w"));
    CchMove m; assert(cch_parse_move(&p,"h2e2",&m));
    assert(m.moving_identity==0x46&&m.captured_identity==0xff);
    char text[5]; cch_format_move(m,text); assert(!strcmp(text,"h2e2"));
    cch_make_move(&p,m);
    /* CMS1's initialized cannon field changes 368 -> 373; black is now the
     * side to move, so the side-relative incremental score is -5. */
    assert(cch_evaluate(&p)==-5);
    cch_unmake_move(&p,m);
    assert(p.identity[CCH_SQUARE(7,7)]==0x46&&p.identity[CCH_SQUARE(4,7)]==0xff);
    CchSearchResult result=cch_search(&p,2); assert(result.depth==2 && result.nodes>0);
    result=cch_search_timed(&p,4,1000);assert(result.depth>=2&&result.nodes>0);
    result=cch_search_cms1_limit(&p,2,0);assert(result.depth==2);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"h0g2"));
    CchPosition cannon_sacrifice;cch_position_start(&cannon_sacrifice);
    play(&cannon_sacrifice,"h2e2");play(&cannon_sacrifice,"h7e7");play(&cannon_sacrifice,"a3a4");
    result=cch_search_cms1_limit(&cannon_sacrifice,2,0);
    assert(result.depth==2&&result.score==2);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"b9c7"));
    CchPosition reversible;cch_position_start(&reversible);
    play(&reversible,"h0g2");play(&reversible,"b9c7");play(&reversible,"g2h0");play(&reversible,"c7b9");
    result=cch_search_cms1_limit(&reversible,2,0);
    assert(result.depth==2&&result.score==4);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"b0c2"));
    assert(result.score==4);
    CchPosition perpetual;
    assert(cch_position_from_fen(&perpetual,"4k4/3R5/9/9/9/9/9/9/9/5K3 w"));
    play(&perpetual,"d8e8");play(&perpetual,"e9d9");
    play(&perpetual,"e8d8");play(&perpetual,"d9e9");
    result=cch_search_cms1_limit(&perpetual,2,0);
    cch_format_move(result.best_move,text);
    /* CMS1 suppresses d8e8 here: replay proves the same rook continuously
     * attacks the opposing general throughout the reversible cycle. Equal
     * terminal returns resolve by original piece-identity order. */
    assert(result.score==9944&&!strcmp(text,"f0f1"));
    CchPosition response_terms;CchMoveList response_moves;
    assert(cch_position_from_fen(&response_terms,
        "3k5/9/9/9/9/4c4/9/4C4/9/4K4 w"));
    cch_generate_pseudo(&response_terms,&response_moves);
    bool found_sixty=false;
    for(size_t i=0;i<response_moves.count;++i){cch_format_move(response_moves.moves[i],text);
        if(!strcmp(text,"e2d2")){assert(response_moves.moves[i].evaluation_adjustment==-60);found_sixty=true;}}
    assert(found_sixty);
    assert(cch_position_from_fen(&response_terms,
        "3k5/9/9/C1P1r4/9/4c4/9/4B4/9/4K4 w"));
    cch_generate_pseudo(&response_terms,&response_moves);
    bool found_fifteen=false;
    for(size_t i=0;i<response_moves.count;++i){cch_format_move(response_moves.moves[i],text);
        if(!strcmp(text,"a6e6")){assert(response_moves.moves[i].evaluation_adjustment==-15);found_fifteen=true;}}
    assert(found_fifteen);
    result=cch_search_cms1_limit(&p,3,0);assert(result.depth==3&&result.score==29);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"h0g2"));
    result=cch_search_cms1_limit(&p,4,0);assert(result.depth==4&&result.score==8);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"h0g2"));
    CchPosition black_start;
    assert(cch_position_from_fen(&black_start,
        "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR b"));
    result=cch_search_cms1_limit(&black_start,4,0);
    assert(result.depth==4&&result.score==8);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"b9c7"));
    CchPosition changed_root;cch_position_start(&changed_root);
    const char *changed_root_line[]={"g0i2","f9e8","c0e2","b7b0","e0e1",
        "h7d7","e1d1","g9i7","h2h8","d7d0","h0f1","b9c7","h8h4",
        "i7g9","b2b9","e8f9","f1h0","i6i5","b9b6","d0e0"};
    for(size_t i=0;i<sizeof changed_root_line/sizeof changed_root_line[0];++i)
        play(&changed_root,changed_root_line[i]);
    result=cch_search_cms1_limit(&changed_root,3,0);
    cch_format_move(result.best_move,text);
    assert(result.score==-163&&!strcmp(text,"a0b0"));
    CchPosition retained_fail_high;cch_position_start(&retained_fail_high);
    const char *retained_line[]={"i0i1","b7b5","b2a2","h7d7","g3g4","d7c7",
        "i1c1","b5a5","a2a5","c7e7","g4g5","e7f7","h2h1","g6g5",
        "h1d1","c6c5","h0i2","f7f3","c1c2","c9e7","f0e1","f3f8",
        "c2b2","f8f6"};
    for(size_t i=0;i<sizeof retained_line/sizeof retained_line[0];++i)
        play(&retained_fail_high,retained_line[i]);
    result=cch_search_cms1_limit(&retained_fail_high,3,0);
    cch_format_move(result.best_move,text);
    assert(result.score==1369&&!strcmp(text,"a5a9"));
    CchPosition check_line;cch_position_start(&check_line);
    play(&check_line,"h2e2");play(&check_line,"b9c7");play(&check_line,"e3e4");
    result=cch_search_cms1_limit(&check_line,2,0);
    assert(result.depth==2&&result.score==15);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"h7e7"));
    play(&check_line,"h7e7");
    result=cch_search_cms1_limit(&check_line,2,0);
    assert(result.depth==2&&result.score==-37);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"h0g2"));
    CchPosition after_horse;cch_position_start(&after_horse);
    assert(cch_parse_move(&after_horse,"h0g2",&m));cch_make_move(&after_horse,m);
    result=cch_search_cms1_limit(&after_horse,2,0);
    assert(result.depth==2&&result.score==-11&&cch_evaluate(&after_horse)==-15);
    cch_format_move(result.best_move,text);assert(!strcmp(text,"b9c7"));
    /* Original DOS oracles: saved-root PVS and continued pawn-root search. */
    const char *pvs_line[]={"c0e2","b7b5","b0c2","c9a7","g3g4","h7d7",
        "f0e1","i9i7","h0g2","h9g7","i0h0","i7i9","h2h1","d7d1",
        "h1h6","c6c5","a0a1","b5a5","e1f0","g7e8","e3e4","d1d4",
        "e0e1","a5b5","e2c0","d4d3"};
    CchPosition pvs_position;cch_position_start(&pvs_position);
    for(size_t i=0;i<sizeof pvs_line/sizeof pvs_line[0];++i)play(&pvs_position,pvs_line[i]);
    result=cch_search(&pvs_position,3);cch_format_move(result.best_move,text);
    assert(result.score==156&&!strcmp(text,"a1d1"));
    const char *pawn_root_line[]={"h2h5","a9a8","a3a4","h7d7","a0a3","d7d1",
        "b0a2","b7d7","i0i1","e9e8","i3i4","i9i8","i1i3","i8i7",
        "h5c5","a8c8","b2b7","i7i9","b7b0","i6i5","c5e5","e8d8",
        "c3c4","c9a7","e5e4","d7h7","b0b8","c8c9","a2b0"};
    CchPosition pawn_root;cch_position_start(&pawn_root);
    for(size_t i=0;i<sizeof pawn_root_line/sizeof pawn_root_line[0];++i)play(&pawn_root,pawn_root_line[i]);
    result=cch_search(&pawn_root,4);cch_format_move(result.best_move,text);
    assert(result.score==-115&&!strcmp(text,"d1d7"));
    { /* Fresh DOS oracle case 25; fixed generalized behavior. */
        const char *line[]={"b0a2","h7h5","a0b0","b7i7","c0e2","h5e5","b2b6","c6c5","b0b2","e5i5","h2h1","i5i4","c3c4","i7d7","h1h3","f9e8","b6c6","i4i0","d0e1","d7h7","b2b1","h7a7","c6d6","a7f7","e1f2","f7e7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-516&&!strcmp(text,"b1i1"));
    }
    { /* Fresh DOS oracle case 79; fixed generalized behavior. */
        const char *line[]={"c3c4","h7c7","b2f2","h9g7","f2g2","b7b1","g0i2","b1b5","b0a2","b5b1","i2g0","c7f7","h2h1","a6a5","h1h8","f7f8","h8h2","b1b3","h0i2","b3b5","a2b0","f8f5","g3g4","c6c5","i0i1","i9h9"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-72&&!strcmp(text,"i1h1"));
    }
    { /* Fresh DOS oracle case 436; fixed generalized behavior. */
        const char *line[]={"h2h5","b7b3","e0e1","c6c5","b2f2","h7e7","f2b2","a9a7","i3i4","b9c7","b0c2","b3e3","a3a4","c7b5","i0i1","e3f3","h5e5","e9e8","i4i5","a7b7","a0b0","g9i7","g0e2","i6i5","b2b7","f3f7","b0a0","f7f2","c2a1","i5i4","b7d7","f2h2","e5g5","h2h7","a0b0","c9a7","d7d1","e7b7","d1d5"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==140&&!strcmp(text,"b7b0"));
    }
    { /* Fresh DOS oracle case 236; fixed generalized behavior. */
        const char *line[]={"h2h6","f9e8","e0e1","h7e7","i0i2","e7e3","a3a4","e3i3","h6h4","a9a7","b2f2","e9f9","i2g2","b7b4","h4f4"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-32767&&!strcmp(text,"f9e9"));
        assert(result.nodes==0);
    }
    { /* Confirmation 646: the minor-piece gate stays open after alpha rises. */
        const char *line[]={"h2g2","h7h5","c0e2","h5g5","b0a2","b9a7","a0a1","i9i8","g2g1","b7f7","b2b9","f7f6","h0i2","g9e7","g1h1","e9e8","h1h7","a7b9","g3g4","f6f4","h7h0","f4e4","a1h1","g5g0","e0e1","a9a8","a2c1","h9f8","h1h6","e8d8","e3e4"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==915&&!strcmp(text,"g0i0"));
    }
    { /* Confirmation 210: shared quiet-pass pruning boundary. */
        const char *line[]={"e0e1","h7d7","g0i2","d7h7","h2d2","h7d7","a0a1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==347&&!strcmp(text,"b7b0"));
    }
    { /* Confirmation 829: fail-soft root retry and its bonus ceiling. */
        const char *line[]={"h2c2","h7e7","b2b3","e7e8","c3c4","b7b0","a0a2","e8e3","g3g4","g9i7","i3i4","c6c5","c2i2","c5c4","b3d3","b0b3","a2c2","f9e8","d3d5","b3c3","g4g5","e3d3","c2a2","e6e5","i2i6","d3d1","a2f2","d1i1","f2f4","e8f9","h0g2","i7g9","f4d4","c3b3"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-61&&!strcmp(text,"i6i1"));
    }
    { /* Confirmation 208: equality cutoff followed by capped root bonus. */
        const char *line[]={"b2e2","e6e5","a0a1","b9c7","h0g2","c9a7","h2h3","h7g7","a1d1","i9i8","d1c1","g7f7","b0c2","a6a5","c1e1","f7h7","h3h6","b7b6","h6c6"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-103&&!strcmp(text,"i8e8"));
    }
    { /* Fresh 617: root pawn side-step ties preserve DOS record order. */
        const char *line[]={"f0e1","c9a7","e1f0","i9i7","h2e2","h7g7","e2h2","g7c7","h2e2","b7b6","b2b9","a9b9","e3e4","i7e7","i0i1","c7b7","e2c2","b9c9","f0e1","f9e8","c3c4","e7f7","e4e5","h9i7","e1f0","f7h7","i3i4","h7c7","c0a2","c7h7","g0i2","h7c7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==29&&!strcmp(text,"e5f5"));
    }
    { /* Fresh 568: saved PV response precedes ordinary check responses. */
        const char *line[]={"h2h6","b9a7","b2c2","b7b8","f0e1","a7c8","b0a2","i9i7","g3g4","h7h0","g0e2","c8d6","i3i4","b8a8","c2d2","i7i8","d2d4","i8c8","g4g5","c8d8","d4d8","a8a7","e2c4"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-330&&!strcmp(text,"a7a3"));
    }
    { /* Fresh 1004: palace steps bypass the reversal wrapper. */
        const char *line[]={"h2h9","b7b8","g0e2","c6c5","i0i2","g6g5","e3e4","h7h4","h9h7","g5g4","h7i7","g4f4","i7a7","b8b6","a7d7","h4h6","e0e1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-202&&!strcmp(text,"i9i7"));
    }
    { /* Fresh 1214: no check responses retain DOS's -20000 sentinel. */
        const char *line[]={"h2h1","h7h0","c0e2","i9i7","h1e1","i7d7","a0a2","h0h8","i3i4","b7b3","e2c4","e9e8","i0i3","e8e9","b2b1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==20000&&!strcmp(text,"b3b0"));
    }
    { /* 2,000-case audit 1242: selective cutoffs update the caller's slots. */
        const char *line[]={"b2a2","b7b2","a2a6","h7c7","h2f2","f9e8","b0c2","a9a7","f2f4","g6g5","f4c4","i9i7","a0b0","g5g4","c0e2","e8d7","c2e1","d7e8","a6b6","g4g3","b6b7","b2d2","b0b2","c7h7","b2b4","g3h3","h0i2","e9f9","b7g7","a7a4","c4f4","i7i8","f4f8"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==1159&&!strcmp(text,"a4b4"));
    }
    { /* Audit 880: stale SI during re-search must not rewrite move history. */
        const char *line[]={"h2e2","h7h2","c3c4","b7i7","i0i1","b9c7","b2b8","e9e8","g0i2","h2f2","b8b9","e8e7","e2a2","f2d2","i1e1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==285&&!strcmp(text,"a9b9"));
    }
    { /* Audit 391: eligible saved captures bypass selective margins. */
        const char *line[]={"h2f2","h7h4","b2a2","h4h6","g3g4","h9g7","a2a6","h6h9","f2f1","e6e5","i0i1","b9c7","a3a4","a9b9","a0a1","b7b2","f1f8","f9e8","f8h8","c7e6","a6a9","b2e2","h0g2","b9b1","g2f4","b1b4","e3e4","b4c4","a1a0","e2c2"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==733&&!strcmp(text,"c3c4"));
    }
    { /* Audit 540: checking cutoffs participate in saved-slot ordering. */
        const char *line[]={"b0c2","h7i7","e0e1","b9c7","i3i4","g9e7","e1e2","f9e8","a0b0","i9i8","b2b4","i8h8","c0a2","a9b9","h2h1","h8h6","h1d1","b7a7","b4b5","a7b7","b0b4","a6a5","i0i3"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==1163&&!strcmp(text,"h6h2"));
    }
    { /* Audit 750: non-positive-alpha reversal geometry. */
        const char *line[]={"b2b9","g9e7","b0c2","b7c7","c2a1","a9a7","h0i2"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==-258&&!strcmp(text,"a7b7"));
    }
    { /* Audit 1360: depth-two 500-point early exit and fallback score. */
        const char *line[]={"h2h5","i9i7","g0e2","a6a5","e0e1","h7f7","h0i2","f7e7","b2b9","a9a6","a0a2","e7c7","a3a4","c7e7","h5e5","e9e8","i0i1","b7b2","b9b8","b2d2","b0c2","e7e5","e1d1","e8e7","b8h8","e5h5","h8g8","i7h7","i2g1","h7g7","g1e0","h5h4","g8i8","a6b6","a2a0","b6b8","i1e1","h4c4","i8e8"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==588&&!strcmp(text,"c4d4"));
    }
    { /* Audit 1197: check responses do not probe the compact cache. */
        const char *line[]={"b2d2","e6e5","c0a2","b7b5","d2d4","d9e8","h0i2","h7h8","a2c4","b5b6","h2c2","h8h3","b0a2","h3h4","d4d5","b6b0","a2b0","h4f4","d5d9","a9a7","e3e4","e8d9","c2g2","a7i7","e4e5","i7i8","g2d2","c9a7","f0e1","i8c8","e1f0","h9g7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==11&&!strcmp(text,"b0c2"));
    }
    { /* Audit 1054: aligned cannon fallback omits outward checking captures. */
        const char *line[]={"b2b9","h7h4","h2h3","b7a7","c0a2","a7b7","c3c4","h4g4","i0i1","g4i4","i1i0","b7h7","b9d9"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-498&&!strcmp(text,"c9e7"));
    }
    { /* Audit 0123: rook ray stops at the zero table entry, without a tail. */
        const char *line[]={"h0i2","h7h8","e0e1","b7d7","b2a2","i9i7","g0e2","h8g8","a0a1","g8d8","e2g4","d8b8","c3c4","d7d3","e1e2","b8h8","a1b1","d3d8","h2h1","d8d5","b1f1","i7d7","h1h7","i6i5","i3i4","d5d1","f1f7","a9a8","f7f8","e6e5","f8f3","a8f8","a2a1","b9a7","e3e4","c9e7","a1b1","e7g5","h7f7","g5i7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-113&&!strcmp(text,"f7f6"));
    }
    { /* Audit 2731: cannon-check responses bypass horse development bonus. */
        const char *line[]={"d0e1","g9e7","h2f2","h7h3","e0d0","g6g5","i0i2","h9i7","f2f5","c6c5","b2b3","g5g4","e3e4","i7g6","i2d2","a9a8","f5f2","i9i7","b3b6","h3h6","f2f5","a8i8","c0a2","g6e5","b6b2","i8c8","d2d5","h6h7","b2b3","c8b8","a3a4","b7b5","g0e2","b5b7","c3c4","b8i8","b3b1","b7d7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-116&&!strcmp(text,"b0d1"));
    }
    { /* Audit 1050: failed raised-bound probe can replace an equal best. */
        const char *line[]={"h2g2","h7h2","b2d2","a9a7","b0c2","e9e8","d2e2","b7b4","i0i1","e8d8","i1i2","d8d7","c2a1","h2h5","a0b0","g9e7","e0e1","h5h2","c3c4","b4b3","e2a2","h2h8","g0e2","h8g8","e2g0","b3b6","g2e2","i9i7","e3e4","g8d8","e4e5"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==-87&&!strcmp(text,"a7b7"));
    }
    { /* Audit 2500: fail-high retries the same root, preserving record order. */
        const char *line[]={"h2h9","e6e5","b0c2","e9e8","c2b0","a9a7","a3a4","b9c7","b2f2","h7e7","f2h2","g6g5","h2h5","g5g4","a0a2","e8d8","a2f2","b7b1","e3e4","a7b7","i0i2","c9a7","h5h4","e7e4","a4a5","b7b9","i2i1","b1b5","e0e1","i9h9","h4h7","d8d7","h0i2","b9b6","f2f3","d7e7","h7f7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==100&&!strcmp(text,"b5d5"));
    }
    { /* Confirmation 49: retry hints without PVS do not suppress the cache. */
        const char *line[]={"h2f2","c9a7","f2g2","a7c9","i0i2","i9i7","c0a2","i7i8","b2b4","i8b8","b4b5","b7d7","b5a5","b8e8","e0e1","d7e7","a5b5","h7h4","b5a5","e8c8","a5d5","h9i7","g2c2","e6e5","i2i0","e7c7","h0i2","c7c3","e1e2","c3c5","e2f2","h4h8","c2c3","h8h1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-65&&!strcmp(text,"f2e2"));
    }
    { /* Audit 612: unchanged attacks are not perpetual chase. */
        const char *line[]={"g0e2","e9e8","e2g0"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-23&&!strcmp(text,"e8e9"));
    }
    { /* Audit 2276: black repetition penalty uses red-minus-black fields. */
        const char *line[]={"h2g2","h7g7","g2d2","b7b0","c3c4","i9i7","g3g4","b9c7","d2d5","i7i8","h0i2","i8e8","a3a4","e6e5","e0e1","g7g8","c0a2","b0f0","b2b3","g6g5","d5d1","g8f8","i2g3","f8h8","g0i2","f0f6","i2g0"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==885&&!strcmp(text,"f6f0"));
    }
    { /* Audit 3666: the cannon may undo a harmless reversible cycle. */
        const char *line[]={"i0i1","b7b3","h2g2","c6c5","a0a1","b3b8","c0a2","h7h1","a2c0"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-59&&!strcmp(text,"h1h7"));
    }
    { /* Audit 3428: saved palace moves use the reversal wrapper too. */
        const char *line[]={"b0c2","h7h8","a3a4","b9c7","h2f2","b7b6","g3g4","h8f8","f2i2","f8a8","b2b3","i9i8","a0a2","b6b8","g4g5","b8b6","b3b1","c7b9","g5g6","g9e7","e3e4","b6b3","b1f1","a8a4","f1b1","c6c5","a2a0","a4a5","e0e1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-14&&!strcmp(text,"b9c7"));
    }
    { /* Audit 2860: the 75-point exit does not try checking pawn moves. */
        const char *line[]={"i0i1","e9e8","a0a1","b9a7","c0e2","b7e7","a1a2","h7h0","i1a1","a7b9","b0c2","e7b7","e2c0","e6e5","g3g4","g9i7","h2h7","e8e7","e0e1","b7b6","b2b3","d9e8","e3e4","h0i0","g4g5","b6b5","e1e0","i9i8","c2e3","b5a5","a1d1","i8i9","e3c4","i0i2","h7h0"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==466&&!strcmp(text,"a5a2"));
    }
    { /* Audit 1279: 500-point fallback bypasses later quiet-pass gates. */
        const char *line[]={"h2c2","a9a8","b2a2","i6i5","a2b2","d9e8","b2b6","h7h8","g3g4","a8d8","h0g2","c6c5","g2f4","e8d9","a3a4","e9e8","b6d6","h8i8","c2d2","d8d6","e0e1","a6a5","d2d9","i8g8","g4g5","b7b3","g5f5","b3b2","e1f1","b2b5","d0e1","d6d3","i0h0","g9e7","f5f6","g8i8","d9d6","e6e5","a4a5","i8g8"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==803&&!strcmp(text,"d6e6"));
    }
    { /* Audit 3659: saved general steps bypass quiet margins. */
        const char *line[]={"h2f2","c6c5","f2c2","h9i7","d0e1","b7a7","i0i2","b9c7","h0g2","e9e8","e3e4","c7b5","a0a2","a7f7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==16&&!strcmp(text,"i2h2"));
    }
    { /* Audit 1950: the same saved-general rule affects deeper root selection. */
        const char *line[]={"b2e2","h7c7","b0a2","e6e5","a2b0","e9e8","e2b2","b9a7","h2h7","g9i7","i0i1","e5e4","b2f2","c7g7","d0e1","b7c7","h7c7","i7g9","f2g2","a9b9","b0c2","i9i8","g2g1","g7g8","i1i0","b9b5","e0d0","b5g5","a3a4","a7b9","a0a2","h9g7","g1i1","e4f4","c7e7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==104&&!strcmp(text,"g5d5"));
    }
    { /* Audit 3103: blocking horse precedes elephant/advisor identities. */
        const char *line[]={"h0g2","b7b5","b2b3","h9i7","b3b9","h7a7","i3i4","b5b2","h2h8","a7a8","g2i3","b2d2","h8h0","a8c8","f0e1","a9a8","i3h1","d2h2","c0e2","a8b8","e3e4","i7h9","e1d2","h2h3","e2g4","a6a5","h1f0","h3h5","h0h2","i9i8","b0a2","g9e7","i4i5","e6e5","b9d9","i8h8","h2h4","b8b5"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==1186&&!strcmp(text,"h4h8"));
    }
    { /* Audit 950: check-interposition order changes fail-low retry ceiling. */
        const char *line[]={"h2h4","e6e5","b2a2","b7b6","i0i2","b6b3","h4b4","h7h6","i2b2","c9a7","g0i2","b9c7","e0e1","a9b9","b4a4","i9i8","a4f4","i8h8","f4f8","b3e3","b2b6","a6a5","b6b5","c6c5","f8f2","h8h7","b5b8","e3f3","a2a5","f3f5","e1e0"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==643&&!strcmp(text,"b9b8"));
    }
    { /* Audit 4000d/1095: root advisor table groups positive-file steps. */
        const char *line[]={"f0e1","g9e7","h2e2","b9a7","i3i4","i6i5","e2g2","b7b5","i4i5","b5b7","b2b3","b7b5","g0i2","h7h2","b0c2","h2c2","g2h2","a7b9","h2h5","b9c7","h5g5","b5i5","g5e5","i5i7","a0b0","i7i0","h0f1","i0i1","e5f5","c2c1","b3b6","e6e5"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-1109&&!strcmp(text,"e1f0"));
    }
    { /* Audit 4000d/1355: red elephant root ties use DS:744a order. */
        const char *line[]={"g3g4","a9a7","g0e2","h7h4","h2h9","b7d7","g4g5","d7e7","i0i2","e7h7","b2b8","a7e7","b8b1","h4h9","a0a2","e7a7","a2c2","a7d7","i2h2","d9e8","d0e1","i6i5","h0i2","h9h2","g5g6","d7d9"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-640&&!strcmp(text,"e2g4"));
    }
    { /* Audit 4000d/3364: black uses the same absolute diagonal table. */
        const char *line[]={"b2b6","h9i7","e3e4","i6i5","h2f2","b7f7","g0i2","h7g7","f2f1","i7h5","b6b3","f7f5","f1f9","a6a5","b3b4","c9e7","f9f7","b9c7","e4e5","c6c5","c0e2","f5g5","b4b7","a9a7","b7b3","c7b5","h0g2","g5g2","f0e1","g7g3","e5d5","h5i7","b0c2","a7c7","b3b4"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==155&&!strcmp(text,"e7g5"));
    }
    { /* Audit 4000e/3229: terminal-loss roots are removed across retries;
       * only c8c7 survives, so DOS returns before the requested depth 3. */
        const char *line[]={"b0c2","c6c5","e0e1","d9e8","c0a2","h7h3","e3e4","b7f7","b2b1","f7e7","e1e2","h3h6","b1g1","h6h4","i0i1","e7c7","h2h9","i9i7","h0g2","a9a7","i3i4","e8d9","c2e1","e9e8","h9h7","c7c3","e2d2","h4h2","g0e2","c3f3","h7c7","f3f7","c7c8","f7h7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-135&&!strcmp(text,"c8c7")&&result.depth==2);
    }
    { /* Audit 1000/885: quiet-margin returns share the -6450 floor;
       * otherwise g7g9 is incorrectly removed before the fail-low retry. */
        const char *line[]={"f0e1","h9g7","b2b6","g7h9","a3a4","g6g5","h2h3","b7e7","e3e4","e7b7","e4e5","c9a7","b0a2","h7h5","e0f0","h5h4","h0i2","h4c4","h3h7","g9e7","f0f1","c4h4","c3c4","h4h1","c0e2","h9i7","b6b0","h1h2","b0b1","i7h9","b1a1","a9a8","h7g7","h2a2","i2g1","e6e5"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-694&&!strcmp(text,"g7g9"));
    }
    { /* Audit 4000f/1978: dedicated cannon-screen horse escape order
       * changes the failed aspiration maximum and subsequent bonus cap. */
        const char *line[]={"h0i2","h7h6","b0c2","f9e8","d0e1","b9a7","c2d0","b7e7","a0b0","e7b7","b2b1","b7b8","h2h4","b8b3","h4f4","i9i7","c0e2","i7i8","i3i4","g9e7","f4f5","a7b9","f5g5","e7g9","e3e4","e8d7","i0i1","i8i7","g5d5","b3b7","d5d6","g9e7","i1i0","b7b3","b1b2"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==665&&!strcmp(text,"b3b0"));
    }
    { /* Audit 4000g/2699: moving the existing cannon screen along its
       * checking ray is not an interposition. Deep Q cutoffs do not write
       * the skipped saved-slot stages either. */
        const char *line[]={"a3a4","f9e8","h2c2","a6a5","b2b3","a9a7","h0i2","h7d7","f0e1","a7a6","c2e2","d7d3","b3b9","b7b1","e1f0","a6b6","i0i1","b1b5","a0a3","b6b8"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==63&&!strcmp(text,"e2e6"));
    }
    { /* Audit 4000h/1036: answering a rook check bypasses ordinary
       * root recognizers, including the home-horse development bonus. */
        const char *line[]={"b2b3","h7g7","i3i4","c6c5","h2h6","g7g8","h0i2","d9e8","f0e1","e8d7","b3b2","d7e8","a0a2","b7e7","b2e2","e9d9","a2d2","e7d7","e2e6","c9a7","e6b6","i6i5","b6b8","g8i8","i4i5","i8f8","b8b2","i9i6","b2b8","d7d4","c0e2","i6h6","b8b7","h6i6","b7g7","a6a5","i0h0","h9g7","d2d4"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-511&&!strcmp(text,"b9d8"));
    }
    { /* Audit 4000j/2550: the fail-low root retry starts a fresh cache
       * generation; retaining the earlier upper bound changes the winner. */
        const char *line[]={"h2h5","b9c7","c0a2","b7b4","h5i5","b4f4","i0i1","c6c5","b2b9","f4f6","i1f1","f6f0","g0i2","c7b5","b0c2","i9i8","c2e1","h7f7","i5g5","b5a7","b9b1","f7f5","g5i5"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==164&&!strcmp(text,"i8h8"));
    }
    { /* Audit 4000m/2667: an additional root retry must not replay the
       * already consumed hint after the failed wide verification. */
        const char *line[]={"h0i2","b7b0","b2b5","e9e8","b5d5","h7h4","a3a4","b0b1","d5g5","h4b4","h2h5","i9i8","g5d5","b4i4","d5f5","e8d8","f5f2","b9a7","f2g2","b1b3","h5e5","i4d4","e5a5","g9e7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-244&&!strcmp(text,"g2d2"));
    }
    { /* Audit 4000n/3299: saved-PV PVS still applies while answering
       * a cannon check; the check restriction belongs to cache probing. */
        const char *line[]={"h2i2","b7e7","b0c2","e9e8","i0i1","h9g7","d0e1","h7h3","i1g1","h3h7","a0a2","e8d8","g1g2","g9i7","b2b0","d8e8","b0b4","e8f8","i2i6","c6c5","e0d0","f9e8","i6i4","i9i8","i4c4","i8g8","c4h4","e7d7","g2f2","e8f7","h4c4","d7b7","i3i4","b9a7","f2h2","b7e7","b4b0","h7h4"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==385&&!strcmp(text,"c4f4"));
    }
    { /* Audit 4000o/3303: cannon-screen escapes finish the positive-file
       * ray before the negative-file ray, preserving later cutoff slots. */
        const char *line[]={"g3g4","a6a5","b0c2","h7d7","b2b4","e9e8","b4e4","e8d8","e4f4","b7b4"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==45&&!strcmp(text,"f4d4"));
    }
    { /* Audit 4000r/1057: guards depend on opposing attacking material. */
        const char *line[]={"h2h9","i6i5","h0i2","i9i7","h9i9","a9a8","c0a2","a8i8","i9i7","i8g8","i7i8","h7d7","a3a4","b7b8","b2b9","g8f8","f0e1","i5i4","e3e4","d7d3","e1f2","f8d8","i8b8","d8h8","f2e1","h8d8","g0e2","d3d2","b9d9","i4i3","b8b6","d8d6","d9d7","e9e8","e1d2","e8e7","b0d1","d6d2","d7d6","g9i7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==1563&&!strcmp(text,"i2h0"));
    }
    { /* Audit 4000r/1829: capturing the opposing general precedes ordinary
       * evasions when the previous move leaves both generals attacked. */
        const char *line[]={"i3i4","h7e7","i0i3","b7d7","b2g2","i9i7","i4i5","i7i8","g2e2","g6g5","h2h7","g9i7","a0a2","c9a7","f0e1","i8f8","e1d2","d7d3","a2c2","f8e8","h7h5","e6e5","h5h7","d3d4","h7h5","e7e6","h5h6","i6i5","h6h7","h9f8","h7h9","i7g9","e0f0","f8h9"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-359&&!strcmp(text,"d0e1"));
    }
    { /* Audit 20000l/10428: horizon PVS and the previous same-side mover
       * precede the restricted deep-quiescence victim banks. */
        const char *line[]={"a0a2","b9a7","h2h9","i9h9","c0e2","a6a5","a2a0","b7g7","b0c2","h9h8","c2b0","g7g3","b0a2","h7h3","e2c0","h8h7","a0b0","d9e8","f0e1","g3g2","b2b8","g2h2","g0e2","a7b9","a3a4","h2f2","b0b4","f2f0","h0g2","g9e7","b4b6","a9a7","b8b7","h3f3","b6b2"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-109&&!strcmp(text,"e7g9"));
    }
    { /* Audit 20000m/19051: capturing and replacing an enemy cannon
       * screen is not a second-screen interposition. */
        const char *line[]={"b2a2","h7e7","h2g2","e9e8","e3e4","e8f8","a2f2","f8f7","e0e1","e7e8","f2f3","g9e7","e1d1","b7d7","i0i2","e8f8","f3f5","a6a5","d1e1","g6g5","b0a2","d7d1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==20000&&!strcmp(text,"g2f2"));
    }
    { /* Audit 20000/2082: both roots are removed on the first search pass.
       * DOS returns score -9656 with no PV; the protocol must emit 0000. */
        const char *line[]={"b0a2","b7g7","b2b0","h7h3","a3a4","h3h8","g0e2","d9e8","h0i2","i9i7","d0e1","b9c7","i3i4","h8h3","b0b5","i7i8","e0d0","c9e7","b5d5","h3h8","d5d4","e7c9","d4c4","g7g8","e2g4","g6g5","h2h7","c7d9","i0i1","i8i7","g4e2","a9a8","a0a1","d9c7","e2g0","c7d9","h7g7","a8b8","c4c9"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-9656&&result.pv_length==0&&!strcmp(text,"0000"));
    }
    { /* Audit 20000b/710: a screen elephant escapes before unrelated
       * interpositions; its dedicated order must not use normal quiet keys. */
        const char *line[]={"e3e4","h7e7","b2b4","h9i7","c0e2","e6e5","c3c4","e7c7","d0e1","a9a8","b4b5","i9h9","b0d1","d9e8","h2h5","a6a5","e1f2","h9h7","h5h6","a8a9","b5d5","h7f7","a0b0","c7e7","d5c5","b7b3","h6h3","f7f2","b0b3","e7f7","i0i2","f7e7","h3h7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==89&&!strcmp(text,"e8d7"));
    }
    { /* Audit 20000/10058: a cached hint leaves its continuation for the
       * next sibling, including quiet hints in stand-pat quiescence. */
        const char *line[]={"c3c4","h7h8","b2b6","h8e8","h2f2","b7i7","h0i2","i7i3","d0e1","a9a7","a0a2","a7f7","a3a4","b9a7","b6b7","a7b9","g3g4","e8g8","f2g2","f7f2","i2g3","g8g7","g3e2","g9e7","e2c3","i3i4","e1d0"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        CchPosition before=fixture;
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-11&&!strcmp(text,"f2f3"));
        assert(!memcmp(before.board,fixture.board,sizeof fixture.board));
        assert(!memcmp(before.identity,fixture.identity,sizeof fixture.identity));
        assert(before.side==fixture.side&&before.ply==fixture.ply);
    }
    { /* Audit 20000c/419: a cache hit has no child PV. Reusing the previous
       * child's line feeds stale moves into the next iteration's cursor. */
        const char *line[]={"g0e2","b7a7","g3g4","d9e8","b2c2","h7h5","h2h1","h9g7","a3a4","h5h7","h1f1","a7b7","c2c1","b7b8","f1i1","i6i5","i1i5","h7h2","c1a1","g6g5","a1c1","h2g2","i5i7","b8b2","c1e1","c6c5","e2c4","i9i7"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-122&&!strcmp(text,"h0g2"));
    }
    { /* Audit 20000e/1783: screen captures belong to their direction's
       * escape pass, not a separate early capture pass. */
        const char *line[]={"g0e2","b7b3","h2i2","h7g7","e2c4","c9a7","i2i1","i9i7","h0g2","b3e3","b2b5","e6e5","g2h0","g7f7","i1d1","c6c5","d1d2","f7f4","b5b6","i7b7","d2i2","f4f7","b6b3","e9e8","b3e3","b7e7","d0e1","f7f5","e1d2","c5c4","a3a4","e8f8","a4a5","f5f7","e3d3","f7f5"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==157&&!strcmp(text,"d3f3"));
    }
    { /* Audit 20000f/17017: selective checks have generator order, not
       * ordinary victim ordering; the 500-point gate is tested once. */
        const char *line[]={"h0i2","g9e7","b2b9","h7h4","h2e2","b7b5","i2g1","h4i4","a0a2","h9i7","b9b7","e9e8","e3e4","b5c5","i0i2","a9a8","b7b6","c5d5","e2e1","d5d2"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==946&&!strcmp(text,"e1e6"));
    }
    { /* Audit 20000g/5510: captures of the last mover precede terminal
       * general captures; removed root scores still update the root bound. */
        const char *line[]={"h2h6","e6e5","b2i2","h7f7","g3g4","b7b8","d0e1","f7g7","e1d0","g7i7","i2e2","i7h7","e2e5","b8d8","h0i2","g6g5","g0e2"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,4);cch_format_move(result.best_move,text);
        assert(result.score==-250&&!strcmp(text,"e9e8"));
    }
    { /* Audit 20000j/13637: losing the saved root disables a12e's root
       * null probes during the fail-low retry. */
        const char *line[]={"e0e1","h9g7","b2b3","b7b5","e1f1","h7i7","h2f2","e9e8","f2f7","i6i5","b3b1","b9c7","b1c1","b5e5","f1f2","c9e7","c0e2","i7f7","i3i4","i9i8","c1c0","a9c9"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,3);cch_format_move(result.best_move,text);
        assert(result.score==-486&&!strcmp(text,"e2g4"));
    }
    { /* Audit 20000j/18972: net-displacement repetition candidates can
       * close an odd history prefix; full board-and-turn equality misses it. */
        const char *line[]={"h2i2","b9a7","c0a2","c9e7","b2g2","a6a5","g2d2","h9i7","g0e2","b7b4","a0a1","b4c4","a1g1","c4a4","g1a1"};
        CchPosition fixture;cch_position_start(&fixture);
        for(size_t i=0;i<sizeof line/sizeof line[0];++i)play(&fixture,line[i]);
        result=cch_search(&fixture,2);cch_format_move(result.best_move,text);
        assert(result.score==-11&&!strcmp(text,"a4d4"));
    }
    assert(cch_cms1_time_budget_ticks(5,0,0)==135);
    assert(cch_cms1_time_budget_ticks(5,0,40)==202);
    assert(cch_cms1_time_budget_ticks(5,0,42)==138);
    assert(cch_cms1_time_budget_ticks(5,301,0)==90);
    assert(cch_cms1_time_budget_ticks(5,0,119)==90);
    assert(cch_profile_count()==36);
    assert(cch_profile_find("Skeleton")==8);
    assert(cch_profile(8)->display_power==2&&cch_profile_clock_argument(cch_profile(8))==47);
    assert(cch_profile_clock_argument(cch_profile(0))==0);
    { /* Handwritten synthetic tree: no original opening-library bytes. */
        CchBookNode nodes[]={
            {1,3,0x77,0x74}, {0x8002,0,0x77,0x73},
            {0,0,0x62,0x52}, {0,0,0x07,0x26}
        };
        CchBook synthetic={nodes,4};CchPosition initial;cch_position_start(&initial);
        CchMove chosen;uint16_t cursor=0;
        assert(cch_book_choice_count(&synthetic,0)==2);
        assert(cch_book_get_move_choice(&synthetic,0,1,&initial,&chosen));
        cch_format_move(chosen,text);assert(!strcmp(text,"c3c4"));
        assert(!cch_book_get_move_choice(&synthetic,0,2,&initial,&chosen));
        assert(cch_book_get_move(&synthetic,0,&initial,&chosen));
        assert(cch_book_advance(&synthetic,&cursor,chosen)&&cursor==3);
        cch_make_move(&initial,chosen);
        assert(cch_book_get_move(&synthetic,cursor,&initial,&chosen));
        cch_format_move(chosen,text);assert(!strcmp(text,"h9g7"));
        assert(!cch_book_advance(&synthetic,&cursor,chosen));
        assert(!cch_book_node(&synthetic,4));
    }
    if(argc>1){CchBook b;assert(cch_book_load(&b,argv[1]));assert(b.count==5778);
        assert(b.nodes[0].alternate==961&&b.nodes[0].continuation==1&&b.nodes[0].from==0x77&&b.nodes[0].to==0x74);
        assert(cch_book_choice_count(&b,0)==5);
        const char *root_choices[5]={"h2e2","g0e2","g3g4","h2f2","h0g2"};
        CchMove bm;for(size_t i=0;i<5;++i){assert(cch_book_get_move_choice(&b,0,i,&p,&bm));
            cch_format_move(bm,text);assert(!strcmp(text,root_choices[i]));}
        assert(cch_book_get_move(&b,0,&p,&bm));cch_format_move(bm,text);assert(!strcmp(text,"h2e2"));
        uint16_t cursor=0;assert(cch_book_advance(&b,&cursor,bm)&&cursor==1);cch_make_move(&p,bm);
        assert(cch_book_get_move(&b,cursor,&p,&bm));cch_format_move(bm,text);assert(!strcmp(text,"h9g7"));cch_book_free(&b);}
    puts("ok"); return 0;
}
