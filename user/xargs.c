#include "kernel/types.h"
#include "user/user.h"


int readline(char *buffer, int size) {
    int index = 0;
    while(read(0, buffer+index, 1) == 1) {
        if (buffer[index] == '\n') {
            buffer[index] = '\0';
            return 0;
        }
        index++;
        if (index+1 == size) break;
    }
    buffer[index] = '\0';
    return 1;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "usage: %s <command> [arguments]\n", argv[0]);
        return 1;
    }
    char *command = argv[1];
    for (int i = 0; i < argc-1; i++) {
        argv[i] = argv[i+1];
    }

    char buffer[256];
    argv[argc-1] = &buffer[0];
    while (readline(buffer, sizeof(buffer)) == 0) {
        int child_pid = fork();
        if (child_pid == 0) {  // child, execute command
            exec(command, argv);
            return 0;
        } else if (child_pid > 0) {  // parent
            wait(0);
        } else {  // error
            fprintf(2, "xargs: fork failed\n");
            return 1;
        }
    }

    return 0;
}