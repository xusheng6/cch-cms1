#include "cch.h"

int main(void){
    cch_set_modern_mode(true);
    return cch_protocol_loop();
}
