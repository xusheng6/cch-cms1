#include "cch.h"
#include <stdio.h>
#include <stdlib.h>

static uint16_t le16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}

bool cch_book_load(CchBook *book,const char *path){
    *book=(CchBook){0};FILE *f=fopen(path,"rb");if(!f)return false;
    if(fseek(f,0,SEEK_END)||ftell(f)<0){fclose(f);return false;}long size=ftell(f);
    if(size%6||fseek(f,0,SEEK_SET)){fclose(f);return false;}
    size_t count=(size_t)size/6;CchBookNode *nodes=calloc(count,sizeof *nodes);if(!nodes){fclose(f);return false;}
    for(size_t i=0;i<count;++i){uint8_t raw[6];if(fread(raw,1,6,f)!=6){free(nodes);fclose(f);return false;}
        nodes[i]=(CchBookNode){le16(raw),le16(raw+2),raw[5],raw[4]};}
    fclose(f);
    book->nodes=nodes;book->count=count;return true;
}
void cch_book_free(CchBook *book){free(book->nodes);*book=(CchBook){0};}
const CchBookNode *cch_book_node(const CchBook *book,uint16_t index){return book&&index<book->count?&book->nodes[index]:NULL;}

static uint8_t native_square(uint8_t raw){int rank=raw>>4,file=raw&15;return file<9&&rank<10?CCH_SQUARE(file,rank):0xff;}
static bool node_move(const CchBookNode *n,const CchPosition *p,CchMove *out){
    uint8_t from=native_square(n->from),to=native_square(n->to);if(from==0xff||to==0xff)return false;
    CchPosition copy=*p;CchMoveList l;cch_generate_legal(&copy,&l);
    for(size_t i=0;i<l.count;++i)if(l.moves[i].from==from&&l.moves[i].to==to){*out=l.moves[i];return true;}return false;
}
bool cch_book_get_move(const CchBook *b,uint16_t cursor,const CchPosition *p,CchMove *move){
    return cch_book_get_move_choice(b,cursor,0,p,move);
}
size_t cch_book_choice_count(const CchBook *b,uint16_t cursor){
    if(!b)return 0;size_t count=0,guard=0;
    while(cursor<b->count&&guard++<b->count){const CchBookNode *n=&b->nodes[cursor];
        /* CMS1 55cf..55ed: the high bit marks a sibling as non-selectable;
         * the remaining 15 bits are still its next-sibling record number. */
        if(!(n->alternate&0x8000))++count;
        cursor=(uint16_t)(n->alternate&0x7fff);if(!cursor)break;}
    return count;
}
bool cch_book_get_move_choice(const CchBook *b,uint16_t cursor,size_t choice,const CchPosition *p,CchMove *move){
    if(!b)return false;size_t guard=0;
    while(cursor<b->count&&guard++<b->count){const CchBookNode *n=&b->nodes[cursor];
        if(!(n->alternate&0x8000)){if(!choice)return node_move(n,p,move);--choice;}
        cursor=(uint16_t)(n->alternate&0x7fff);if(!cursor)break;}
    return false;
}
bool cch_book_advance(const CchBook *b,uint16_t *cursor,CchMove move){
    size_t guard=0;uint16_t i=*cursor;while(i<b->count&&guard++<b->count){const CchBookNode *n=&b->nodes[i];
        if(native_square(n->from)==move.from&&native_square(n->to)==move.to){if(!n->continuation||n->continuation>=b->count)return false;*cursor=n->continuation;return true;}
        i=(uint16_t)(n->alternate&0x7fff);if(!i||i>=b->count)return false;}return false;
}
