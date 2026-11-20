#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <sys/wait.h>
#include "shared.h"

typedef struct {
    char memory[32], ready_name[32];
    Message *m;
    sem_t *ready;
    pid_t pid;
    int owns_memory;
} Channel;

static Channel ch[2];
static sem_t *ack;
static char ack_name[32];
static volatile sig_atomic_t child_failed;

static void on_exit_child(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)context;
    if (info->si_code != CLD_EXITED || info->si_status != 0) {
        int saved = errno;
        child_failed = 1;
        sem_post(ack);
        errno = saved;
    }
}

static void wait_reply(void)
{
    wait_sem(ack);
    if (child_failed) {
        fprintf(stderr, "Дочерний процесс завершился с ошибкой\n");
        exit(1);
    }
}

static void remove_names(void)
{
    for (int i = 0; i < 2; i++) {
        Channel *c = &ch[i];
        if (c->owns_memory) {
            if (shm_unlink(c->memory) == -1) perror("shm_unlink");
            else c->owns_memory = 0;
        }
        if (c->ready && c->ready != SEM_FAILED && c->ready_name[0]) {
            if (sem_unlink(c->ready_name) == -1) perror("sem_unlink");
            else c->ready_name[0] = 0;
        }
    }
    if (ack_name[0]) {
        if (sem_unlink(ack_name) == -1) perror("sem_unlink");
        else ack_name[0] = 0;
    }
}

static void cleanup(void)
{
    signal(SIGCHLD, SIG_DFL);
    remove_names();
    for (int i = 0; i < 2; i++) {
        Channel *c = &ch[i];
        if (c->pid > 0) {
            kill(c->pid, SIGTERM);
            while (waitpid(c->pid, NULL, 0) == -1 && errno == EINTR) {}
        }
        if (c->m && munmap(c->m, sizeof(Message)) == -1) perror("munmap");
        if (c->ready && c->ready != SEM_FAILED && sem_close(c->ready) == -1) perror("sem_close");
    }
    if (sem_close(ack) == -1) perror("sem_close");
}

static void start(Channel *c, int id, const char *file)
{
    snprintf(c->memory, sizeof(c->memory), "/l3m%ld_%d", (long)getpid(), id);
    snprintf(c->ready_name, sizeof(c->ready_name), "/l3r%ld_%d", (long)getpid(), id);
    int fd = shm_open(c->memory, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) fail("shm_open");
    c->owns_memory = 1;
    if (ftruncate(fd, sizeof(Message)) == -1) fail("ftruncate");
    c->m = map_message(fd);
    c->ready = sem_open(c->ready_name, O_CREAT | O_EXCL, 0600, 0);
    if (c->ready == SEM_FAILED) fail("sem_open");
    c->pid = fork();
    if (c->pid == -1) fail("fork");
    if (c->pid == 0) {
        execl("./child", "child", c->memory, c->ready_name, ack_name, file, (char *)NULL);
        perror("execl ./child");
        _exit(1);
    }
    wait_reply();
}

static void send_line(Channel *c, const char *line, size_t length)
{
    do {
        size_t n = length;
        if (n > sizeof(c->m->text)) n = sizeof(c->m->text);
        length -= n;
        memcpy(c->m->text, line + length, n);
        c->m->size = (int)n;
        c->m->last = (length == 0);
        post(c->ready);
        wait_reply();
    } while (length > 0);
}

int main(void)
{
    char *files[2] = {NULL, NULL};
    size_t cap = 0;
    for (int i = 0; i < 2; i++) {
        printf("Файл для child%d:\n", i + 1);
        ssize_t n = getline(&files[i], &cap, stdin);
        if (n < 0) {
            if (!feof(stdin)) fail("getline");
            fprintf(stderr, "Ожидалось имя файла\n");
            return 1;
        }
        if (files[i][n - 1] == '\n') files[i][n - 1] = '\0';
    }
    snprintf(ack_name, sizeof(ack_name), "/l3a%ld", (long)getpid());
    ack = sem_open(ack_name, O_CREAT | O_EXCL, 0600, 0);
    if (ack == SEM_FAILED) fail("sem_open");
    atexit(cleanup);

    struct sigaction action = {0};
    action.sa_sigaction = on_exit_child;
    action.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGCHLD, &action, NULL) == -1) fail("sigaction");

    for (int i = 0; i < 2; i++) {
        start(&ch[i], i, files[i]);
        free(files[i]);
    }
    remove_names();
    printf("Вводите строки (Ctrl+D для завершения):\n");
    char *line = NULL;
    ssize_t n;
    while ((n = getline(&line, &cap, stdin)) >= 0) {
        if (line[n - 1] == '\n') n--;
        int id = n > 10 ? 1 : 0;
        send_line(&ch[id], line, (size_t)n);
    }
    free(line);
    if (!feof(stdin)) fail("getline");
    for (int i = 0; i < 2; i++) {
        ch[i].m->size = -1;
        post(ch[i].ready);
    }
    for (int i = 0; i < 2; i++) {
        int status;
        while (waitpid(ch[i].pid, &status, 0) == -1)
            if (errno != EINTR) fail("waitpid");
        ch[i].pid = 0;
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return 1;
    }
    return 0;
}
