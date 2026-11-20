#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <sys/wait.h>
#include "shared.h"

typedef struct {
    char memory[32], ready_name[32];
    Message *m;
    sem_t *ready;
    pid_t pid;
} Channel;

static Channel ch[2];
static sem_t *ack;
static char ack_name[32];
static const char *sem_names[3];
static int sem_count, memory_count;
static volatile sig_atomic_t child_exited;

static sem_t *create_sem(const char *name)
{
    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, 0600, 0);
    if (sem == SEM_FAILED) fail("sem_open");
    sem_names[sem_count++] = name;
    return sem;
}

static void on_exit_child(int sig)
{
    int saved = errno;
    (void)sig;
    child_exited = 1;
    sem_post(ack);
    errno = saved;
}

static void wait_reply(void)
{
    wait_sem(ack);
    if (child_exited) {
        fprintf(stderr, "Дочерний процесс завершился до конца обмена\n");
        exit(1);
    }
}

static void remove_names(void)
{
    for (int i = 0; i < memory_count; i++)
        if (shm_unlink(ch[i].memory) == -1) perror("shm_unlink");
    for (int i = 0; i < sem_count; i++)
        if (sem_unlink(sem_names[i]) == -1) perror("sem_unlink");
    memory_count = sem_count = 0;
}

static void cleanup(void)
{
    remove_names();
    for (int i = 0; i < 2; i++) {
        if (ch[i].pid <= 0) continue;
        kill(ch[i].pid, SIGTERM);
        while (waitpid(ch[i].pid, NULL, 0) == -1 && errno == EINTR) {}
    }
}

static void start(int id, const char *file)
{
    Channel *c = &ch[id];
    snprintf(c->memory, sizeof(c->memory), "/l3m%ld_%d", (long)getpid(), id);
    snprintf(c->ready_name, sizeof(c->ready_name), "/l3r%ld_%d", (long)getpid(), id);
    int fd = shm_open(c->memory, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) fail("shm_open");
    memory_count++;
    if (ftruncate(fd, sizeof(Message)) == -1) fail("ftruncate");
    c->m = map_message(fd);
    c->ready = create_sem(c->ready_name);
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
    if (atexit(cleanup) != 0) return 1;
    ack = create_sem(ack_name);

    struct sigaction action = {.sa_handler = on_exit_child};
    action.sa_flags = SA_NOCLDSTOP;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGCHLD, &action, NULL) == -1) fail("sigaction");

    for (int i = 0; i < 2; i++) {
        start(i, files[i]);
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
        int status;
        while (waitpid(ch[i].pid, &status, 0) == -1)
            if (errno != EINTR) fail("waitpid");
        ch[i].pid = 0;
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return 1;
    }
    return 0;
}
