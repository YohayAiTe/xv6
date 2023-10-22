#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(2, "pipe failed");
        return 1;
    }
    int child_pid = fork();
    if (child_pid == -1) {
        fprintf(2, "fork failed");
        return 1;
    }
    if (child_pid == 0) {
        char recv_byte;
        while (read(pipefd[0], &recv_byte, 1) == 0);
        fprintf(0, "%d: received ping\n", getpid());
        if (write(pipefd[1], &recv_byte, 1) != 1) {
            fprintf(2, "writing pong failed");
            return 1;
        }
        return 0;
    } else {
        char send_byte = 'a';
        int n;
        if ((n = write(pipefd[1], &send_byte, 1)) != 1) {
            fprintf(2, "writing ping failed");
            return 1;
        }
        int child_status;
        wait(&child_status);
        if (child_status != 0) {
            fprintf(2, "receiving pong failed: child failed");
            return 1;
        }
        char recv_byte;
        if (read(pipefd[0], &recv_byte, 1) != 1) {
            fprintf(2, "receiving pong failed");
            return 1;
        }
        if (recv_byte != send_byte) {
            fprintf(2, "receiving pong incorrect byte");
            return 1;
        }
        fprintf(0, "%d: received pong\n", getpid());
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }
}