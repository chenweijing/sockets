#include <stdio.h>

#include "sockets.h"

int main(int argc, char* argv[0]){
    bool is_block = true;
    int sfd = sockets::start(is_block, "6666");
    return 0;
}
