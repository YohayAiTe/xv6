#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define BUF_SIZE (512)


int find_in_path(char *path, char *to_find) {
    int fd;
    struct stat st;
    struct dirent de;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s(%d); fd=%d\n", path, strlen(path), fd);
        return 0;
    }
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return 1;
    }

    if (st.type != T_DIR) {
        close(fd);
        return 0;
    }

    if (strlen(path) + 1 + DIRSIZ + 1 > BUF_SIZE) {
        fprintf(2, "find: path too long\n");
        close(fd);
        return 1;
    }
    char *start = path + strlen(path);
    *(start++) = '/';
    start[DIRSIZ] = '\0';

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        *start = '\0';
        if (strcmp(de.name, ".") == 0) { continue; }
        if (strcmp(de.name, "..") == 0) { continue; }
        if (strcmp(de.name, to_find) == 0) {
            fprintf(0, "%s%s\n", path, to_find);
            continue;
        }
        memmove(start, de.name, DIRSIZ);
        int ret;
        if ((ret = find_in_path(path, to_find)) != 0) {
            return ret;
        }
    }
    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "usage: %s <path> <name>", argv[0]);
        return 1;
    }
    char path[BUF_SIZE];
    strcpy(path, argv[1]);
    return find_in_path(path, argv[2]);
}
