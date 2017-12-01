#include <stdio.h>
#include "trans.h"
#include "bt_parse.h"

bt_config_t config;
queue *has_chunks;
int main(int argc, char **argv) {
    bt_init(&config, argc, argv);
    init_has_chunks("./A.chunks");
    queue *q=init_whohas_queue("./D.chunks");
    print_queue(q);
    //printf("Hello, World!\n");
    return 0;
};