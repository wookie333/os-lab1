#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "io_utils.h"

pid_t start_child(const char *file, int in[2], int other[2]) {
    pid_t pid = fork();
    if (pid == -1) {
        print_error("fork");
        return -1;
    }
    if (pid == 0) {
        int out = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out == -1) {
            print_error("open");
            exit(1);
        }
        if (dup2(in[0], STDIN_FILENO) == -1 || dup2(out, STDOUT_FILENO) == -1) {
            print_error("dup2");
            exit(1);
        }
        close(out);
        close(in[0]);
        close(in[1]);
        close(other[0]);
        close(other[1]);

        execl("./child", "child", NULL);
        print_error("execl");
        exit(1);
    }
    return pid;
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    char *line = NULL;
    size_t cap = 0;

    write_all(STDOUT_FILENO, "Файл для child1: ", sizeof("Файл для child1: ") - 1);
    if (read_line(STDIN_FILENO, &line, &cap) == -1) return 1;
    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '\0') {
        write_all(STDERR_FILENO, "Имя файла не введено\n", sizeof("Имя файла не введено\n") - 1);
        return 1;
    }
    char *file1 = strdup(line);

    write_all(STDOUT_FILENO, "Файл для child2: ", sizeof("Файл для child2: ") - 1);
    if (read_line(STDIN_FILENO, &line, &cap) == -1) return 1;
    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '\0') {
        write_all(STDERR_FILENO, "Имя файла не введено\n", sizeof("Имя файла не введено\n") - 1);
        return 1;
    }
    char *file2 = strdup(line);

    if (file1 == NULL || file2 == NULL) {
        print_error("strdup");
        return 1;
    }

    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        print_error("pipe");
        return 1;
    }

    pid_t pid1 = start_child(file1, pipe1, pipe2);
    pid_t pid2 = -1;
    if (pid1 != -1) pid2 = start_child(file2, pipe2, pipe1);

    close(pipe1[0]);
    close(pipe2[0]);

    int result = 0;

    if (pid1 == -1 || pid2 == -1) {
        result = 1;
    } else {
        write_all(STDOUT_FILENO, "Вводите строки (Ctrl+D - конец):\n", sizeof("Вводите строки (Ctrl+D - конец):\n") - 1);
        while (read_line(STDIN_FILENO, &line, &cap) != -1) {
            size_t n = strcspn(line, "\n");
            line[n] = '\n';

            int fd = (n > 10) ? pipe2[1] : pipe1[1];
            if (write_all(fd, line, n + 1) == -1) {
                print_error("write");
                result = 1;
                break;
            }
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    int status;
    if (pid1 > 0) {
        if (waitpid(pid1, &status, 0) == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) result = 1;
    }
    if (pid2 > 0) {
        if (waitpid(pid2, &status, 0) == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) result = 1;
    }

    free(file1);
    free(file2);
    free(line);
    return result;
}
