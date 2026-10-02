#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t start_child(const char *file, int in[2], int other[2]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        int out = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out == -1) {
            perror("open");
            exit(1);
        }
        if (dup2(in[0], STDIN_FILENO) == -1 || dup2(out, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        close(out);
        close(in[0]);
        close(in[1]);
        close(other[0]);
        close(other[1]);

        execl("./child", "child", NULL);
        perror("execl");
        exit(1);
    }
    return pid;
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    char *line = NULL;
    size_t cap = 0;

    printf("Файл для child1: ");
    fflush(stdout);
    if (getline(&line, &cap, stdin) == -1) return 1;
    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '\0') {
        fprintf(stderr, "Имя файла не введено\n");
        return 1;
    }
    char *file1 = strdup(line);

    printf("Файл для child2: ");
    fflush(stdout);
    if (getline(&line, &cap, stdin) == -1) return 1;
    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '\0') {
        fprintf(stderr, "Имя файла не введено\n");
        return 1;
    }
    char *file2 = strdup(line);

    if (file1 == NULL || file2 == NULL) {
        perror("strdup");
        return 1;
    }

    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
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
        printf("Вводите строки (Ctrl+D - конец):\n");
        while (getline(&line, &cap, stdin) != -1) {
            size_t n = strcspn(line, "\n");
            line[n] = '\n';

            // длиннее 10 символов - во второй канал, иначе в первый
            int fd = (n > 10) ? pipe2[1] : pipe1[1];
            if (write(fd, line, n + 1) == -1) {
                perror("write");
                result = 1;
                break;
            }
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    int status;
    if (pid1 > 0) {
        waitpid(pid1, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) result = 1;
    }
    if (pid2 > 0) {
        waitpid(pid2, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) result = 1;
    }

    free(file1);
    free(file2);
    free(line);
    return result;
}
