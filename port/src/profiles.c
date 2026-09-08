#include "cch.h"
#include <string.h>

/* CCH.EXE DS:00aa names, DS:00f2 display words, and DS:012c controls. */
static const CchProfile profiles[] = {
    {"Rick Ton",4,3},{"Monkey Sun",4,1},{"Spoon Sa",2,2},{"Bob Ju",1,3},
    {"Eric New",3,3},{"Lotus Lee",3,4},{"Mum",2,3},{"Dragon",3,39989},
    {"Skeleton",2,47917},{"Hand",2,8080},{"Bogey",1,1076},{"Evil",4,32523},
    {"Micky Jumbo",3,27694},{"Michael Danger",2,9728},{"McDANA",1,26375},
    {"Terminal Man",2,10439},{"Master Jones",4,16680},{"Lonely Boy",2,1943},
    {"Knife Thomas",2,41516},{"Father Wang",3,25498},{"Cop Dioso",2,11324},
    {"Driver Brown",1,2510},{"Soldier Lee",2,18085},{"Miss Nash",3,43811},
    {"Mr. Iraq",2,20038},{"Mr. USSR",3,13310},{"Mrs. England",3,23529},
    {"Mr. USA",2,8872},{"Mr. China",4,4002},{"Mr. Texas",3,19164},
    {"Boxer",1,39750},{"Half-face",2,5796},{"Flash Man",3,22287},
    {"Butterfly",3,29830},{"Fire Man",4,15097},{"Fly Man",3,45664}
};

size_t cch_profile_count(void){return sizeof profiles/sizeof profiles[0];}
const CchProfile *cch_profile(size_t index){return index<cch_profile_count()?&profiles[index]:0;}
int cch_profile_find(const char *name){
    for(size_t i=0;i<cch_profile_count();++i)if(!strcmp(name,profiles[i].name))return (int)i;
    return -1;
}
uint16_t cch_profile_clock_argument(const CchProfile *profile){
    return profile?(uint16_t)(profile->raw_control/1000u):0;
}
