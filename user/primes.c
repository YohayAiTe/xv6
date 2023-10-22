#include "kernel/types.h"
#include "user/user.h"

#define END (0xff)

void worker(int inputfd) {
    int prime;
    int read_status = 0;
    while (read_status == 0) {
        read_status = read(inputfd, &prime, 1);
        if (read_status == -1) {
            fprintf(2, "reading prime failed");
            exit(1);
        }
    }
    fprintf(0, "prime %d\n", prime);

    int pipefd[2] = {0, 0};
    int n;
    int child_pid = -1;
    while (read_status != -1) {
        read_status = read(inputfd, &n, 1);
        if (read_status != 1) continue;
        if (n == END) {
            if (child_pid != -1) {
                if (write(pipefd[1], &n, 1) != 1) {
                    fprintf(2, "prime %d writing %d failed", prime, n);
                }
            }
            break;
        } else if (n % prime != 0) {
            // create child as needed
            if (child_pid == -1) {
                if (pipe(pipefd) != 0) {
                    fprintf(2, "prime %d pipe creation failed", prime);
                    exit(1);
                }    
                child_pid = fork();
                if (child_pid == 0) {  // child thread
                    close(pipefd[1]);
                    worker(pipefd[0]);
                } else if (child_pid < 0) {  // error
                    fprintf(2, "prime %d forking failed", prime);
                    exit(1);
                }
                close(pipefd[0]);
            }

            // write to child
            if (write(pipefd[1], &n, 1) != 1) {
                fprintf(2, "prime %d writing %d failed", prime, n);
            }
        }
    }
    close(inputfd);
    if (pipefd[1]) close(pipefd[1]);
    wait(0);
    exit(0);
}

int main(int argc, char *argv[]) {
    int first_pipe[2];
    if (pipe(first_pipe) != 0) {
        fprintf(2, "pipe creation failed");
        return 1;
    }

    int child_pid = fork();
    if (child_pid > 0) {  // main thread
        close(first_pipe[0]);
        for (int i = 2; i < 35; ++i) {
            if (write(first_pipe[1], &i, 1) != 1) {
                fprintf(2, "writing %d failed", i);
            }
        }
        char end = END;
        if (write(first_pipe[1], &end, 1) != 1) {
            fprintf(2, "writing %d (end) failed", END);
        }
        close(first_pipe[1]);
        wait(0);
        exit(0);
    } else if (child_pid == 0) {  // child thread
        close(first_pipe[1]);
        worker(first_pipe[0]);
    } else {
        fprintf(2, "main forking failed");
        return 1;
    }
    return 0;
}