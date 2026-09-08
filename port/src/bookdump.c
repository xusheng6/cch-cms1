#include "cch.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv){
    if(argc<2){fprintf(stderr,"usage: %s OPENING.LIB [start [count]]\n",argv[0]);return 2;}
    CchBook b;if(!cch_book_load(&b,argv[1])){fprintf(stderr,"cannot parse %s\n",argv[1]);return 1;}
    size_t start=argc>2?(size_t)strtoul(argv[2],0,0):0,n=argc>3?(size_t)strtoul(argv[3],0,0):20;
    printf("nodes: %zu\n",b.count);
    for(size_t i=start;i<b.count&&i<start+n;++i){const CchBookNode *x=&b.nodes[i];
        printf("%5zu alt=%5u cont=%5u move=%02x-%02x\n",i,x->alternate,x->continuation,x->from,x->to);}
    cch_book_free(&b);return 0;
}
