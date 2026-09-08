#include "cch.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    CchPosition p; cch_position_start(&p);
    if(argc==3 && argv[1][0]=='p') {
        unsigned d=(unsigned)strtoul(argv[2],0,10);
        printf("%llu\n",(unsigned long long)cch_perft(&p,d)); return 0;
    }
    if(argc==1)return cch_protocol_loop();
    char fen[128]; cch_position_to_fen(&p,fen,sizeof fen); puts(fen); return 0;
}
