#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

typedef struct {
    double x, y, z;
} Point;

typedef struct {
    double area;
    int a, b, c;
} Result;

Point points[5000];
Result results[1024];
pthread_t threads[1024];
int ids[1024];
int n, p;

int number(char *s)
{
    int value = 0;
    if (s[0] == '\0') return -1;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        value = value * 10 + s[i] - '0';
        if (value > 5000) return -1;
    }
    return value;
}

double area(int a, int b, int c)
{
    double ux = points[b].x - points[a].x;
    double uy = points[b].y - points[a].y;
    double uz = points[b].z - points[a].z;
    double vx = points[c].x - points[a].x;
    double vy = points[c].y - points[a].y;
    double vz = points[c].z - points[a].z;
    double x = uy * vz - uz * vy;
    double y = uz * vx - ux * vz;
    double z = ux * vy - uy * vx;
    return sqrt(x * x + y * y + z * z) / 2;
}

void *search(void *arg)
{
    int id = *(int *)arg;
    results[id].area = -1;
    for (int i = id; i < n - 2; i += p) {
        for (int j = i + 1; j < n - 1; j++) {
            for (int k = j + 1; k < n; k++) {
                double value = area(i, j, k);
                if (value > results[id].area) {
                    results[id].area = value;
                    results[id].a = i;
                    results[id].b = j;
                    results[id].c = k;
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 5 || argv[1][0] != '-' || argv[1][1] != 't' || argv[1][2] != '\0' ||
        argv[3][0] != '-' || argv[3][1] != 'n' || argv[3][2] != '\0') {
        printf("Запуск: ./lab2 -t 10 -n 2000\n");
        return 1;
    }
    int count = number(argv[2]);
    n = number(argv[4]);
    if (count < 1 || count > 1024 || n < 3 || n > 5000) {
        printf("Нужно от 1 до 1024 потоков и от 3 до 5000 точек\n");
        return 1;
    }
    for (int i = 0; i < n; i++) {
        points[i].x = i * 17 % 2001 - 1000;
        points[i].y = i * i * 13 % 2001 - 1000;
        points[i].z = (i * i * 7 + i * 31) % 2001 - 1000;
    }
    double times[2];
    for (int mode = 0; mode < 2; mode++) {
        p = 1;
        if (mode == 1) p = count;
        for (int i = 0; i < p; i++) ids[i] = i;
        struct timespec start, finish;
        if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
            printf("Ошибка clock_gettime\n");
            return 1;
        }
        for (int i = 1; i < p; i++) {
            int error = pthread_create(&threads[i], NULL, search, &ids[i]);
            if (error != 0) {
                printf("Ошибка pthread_create: %d\n", error);
                return 1;
            }
        }
        search(&ids[0]);
        for (int i = 1; i < p; i++) {
            int error = pthread_join(threads[i], NULL);
            if (error != 0) {
                printf("Ошибка pthread_join: %d\n", error);
                return 1;
            }
        }
        Result best = results[0];
        for (int i = 1; i < p; i++) {
            if (results[i].area > best.area) best = results[i];
        }
        if (clock_gettime(CLOCK_MONOTONIC, &finish) != 0) {
            printf("Ошибка clock_gettime\n");
            return 1;
        }
        times[mode] = (finish.tv_sec - start.tv_sec) * 1000.0 +
                      (finish.tv_nsec - start.tv_nsec) / 1000000.0;
        printf("Потоки: %d; время: %.6f мс; площадь: %.10f; точки: %d %d %d\n",
               p, times[mode], best.area, best.a + 1, best.b + 1, best.c + 1);
    }
    if (times[1] > 0) {
        double speedup = times[0] / times[1];
        printf("Ускорение: %.3f; эффективность: %.4f\n", speedup, speedup / count);
    }
    return 0;
}
