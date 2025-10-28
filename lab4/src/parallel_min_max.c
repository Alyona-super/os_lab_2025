#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальная переменная для хранения PID дочерних процессов
pid_t *child_pids = NULL;
int child_count = 0;

// Обработчик сигнала SIGALRM
void alarm_handler(int sig) {
    printf("Таймаут! Отправляю SIGKILL всем дочерним процессам...\n");
    
    for (int i = 0; i < child_count; i++) {
        if (child_pids[i] > 0) {
            printf("Завершаю процесс %d\n", child_pids[i]);
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = -1;  // таймаут в секундах
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 't'},  // вставка параметра timeout
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "ft:", options, &option_index);  // Добавляю 't:'

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 3:
                        with_files = true;
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case 't':  // Обработка параметра timeout
                timeout = atoi(optarg);
                if (timeout <= 0) {
                    printf("timeout must be a positive number\n");
                    return 1;
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
               argv[0]);
        return 1;
    }

    // Инициализация массива для хранения PID дочерних процессов
    child_pids = malloc(pnum * sizeof(pid_t));
    child_count = pnum;

    // Если задан таймаут, устанавливаем обработчик сигнала и будильник
    if (timeout > 0) {
        signal(SIGALRM, alarm_handler);
        alarm(timeout);
        printf("Таймаут установлен на %d секунд\n", timeout);
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    
    // pipes для каждого процесса
    int **pipes = NULL;
    if (!with_files) {
        pipes = malloc(pnum * sizeof(int*));
        for (int i = 0; i < pnum; i++) {
            pipes[i] = malloc(2 * sizeof(int));
            if (pipe(pipes[i]) == -1) {
                perror("pipe failed");
                return 1;
            }
        }
    }

    int active_child_processes = 0;
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            active_child_processes += 1;
            child_pids[i] = child_pid;  // Сохраняем PID дочернего процесса
            
            if (child_pid == 0) {
                // Дочерний процесс
                int start = i * (array_size / pnum);
                int end = (i == pnum - 1) ? array_size : (i + 1) * (array_size / pnum);
                
                struct MinMax local_min_max = GetMinMax(array, start, end);
                
                if (with_files) {
                    // ФАЙЛ
                    char filename[100];
                    sprintf(filename, "min_max_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    if (file != NULL) {
                        fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                        fclose(file);
                    }
                } else {
                    // PIPE
                    close(pipes[i][0]);
                    write(pipes[i][1], &local_min_max.min, sizeof(int));
                    write(pipes[i][1], &local_min_max.max, sizeof(int));
                    close(pipes[i][1]);
                }
                free(array);
                if (!with_files) {
                    for (int j = 0; j < pnum; j++) {
                        free(pipes[j]);
                    }
                    free(pipes);
                }
                free(child_pids);  // Освобождаем в дочернем процессе
                exit(0);
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    // Ожидание завершения дочерних процессов с проверкой статуса
    while (active_child_processes > 0) {
        int status;
        pid_t finished_pid = wait(&status);
        
        if (finished_pid > 0) {
            active_child_processes -= 1;
            
            // Проверяем, был ли процесс завершен по сигналу
            if (WIFSIGNALED(status)) {
                int term_signal = WTERMSIG(status);
                printf("Процесс %d завершен сигналом %d", finished_pid, term_signal);
                if (term_signal == SIGKILL) {
                    printf(" (SIGKILL - по таймауту)");
                }
                printf("\n");
            } else if (WIFEXITED(status)) {
                printf("Процесс %d завершился до окончания таймера с кодом %d\n", 
                       finished_pid, WEXITSTATUS(status));
            }
        }
    }

    // Отменяем будильник, если все процессы завершились до таймаута
    if (timeout > 0) {
        alarm(0);
        printf("Все процессы завершились до истечения таймаута\n");
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    // Сбор результатов от завершившихся процессов
    int completed_processes = 0;
    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;
        bool result_available = false;

        if (with_files) {
            char filename[100];
            sprintf(filename, "min_max_%d.txt", i);
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                if (fscanf(file, "%d %d", &min, &max) == 2) {
                    result_available = true;
                }
                fclose(file);
                remove(filename); 
            }
        } else {
            // Проверяем, есть ли данные в pipe
            close(pipes[i][1]);
            ssize_t bytes_read_min = read(pipes[i][0], &min, sizeof(int));
            ssize_t bytes_read_max = read(pipes[i][0], &max, sizeof(int));
            close(pipes[i][0]);
            
            if (bytes_read_min == sizeof(int) && bytes_read_max == sizeof(int)) {
                result_available = true;
            }
        }

        if (result_available) {
            completed_processes++;
            if (min < min_max.min) min_max.min = min;
            if (max > min_max.max) min_max.max = max;
        } else {
            printf("Процесс %d не успел завершить вычисления\n", child_pids[i]);
        }
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            free(pipes[i]);
        }
        free(pipes);
    }

    printf("\nРезультаты:\n");
    printf("Минимум: %d\n", min_max.min);
    printf("Максимум: %d\n", min_max.max);
    printf("Завершено процессов: %d из %d\n", completed_processes, pnum);
    printf("Затраченное время: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}