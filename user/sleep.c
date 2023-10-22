#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(2, "sleep usage: sleep <ticks>\n");
        return 1;
    }

    int ticks = atoi(argv[1]);
    sleep(ticks);
    return 0;
}