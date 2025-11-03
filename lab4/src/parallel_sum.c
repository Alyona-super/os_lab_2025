#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include "utils.h"
#include "parallel_sum.h"

typedef struct {
    int* array;
    int start;
    int end;
    long long partial_sum;
} ThreadData;

void* partial_sum(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->partial_sum = 0;
    
    for (int i = data->start; i < data->end; i++) {
        data->partial_sum += data->array[i];
    }
    
    return NULL;
}

long long calculate_parallel_sum(int* array, int array_size, int threads_num) {
    pthread_t* threads = malloc(threads_num * sizeof(pthread_t));
    ThreadData* thread_data = malloc(threads_num * sizeof(ThreadData));
    
    int segment_size = array_size / threads_num;
    
    // Создание потоков
    for (int i = 0; i < threads_num; i++) {
        thread_data[i].array = array;
        thread_data[i].start = i * segment_size;
        thread_data[i].end = (i == threads_num - 1) ? array_size : (i + 1) * segment_size;
        
        if (pthread_create(&threads[i], NULL, partial_sum, &thread_data[i]) != 0) {
            perror("pthread_create");
            free(threads);
            free(thread_data);
            return -1;
        }
    }
    
    // Ожидание завершения потоков и суммирование результатов
    long long total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            free(threads);
            free(thread_data);
            return -1;
        }
        total_sum += thread_data[i].partial_sum;
    }
    
    free(threads);
    free(thread_data);
    return total_sum;
}

int main(int argc, char *argv[]) {
    int threads_num = -1;
    int seed = -1;
    int array_size = -1;
    
    // Аргументы командной строки
    struct option long_options[] = {
        {"threads_num", required_argument, 0, 't'},
        {"seed", required_argument, 0, 's'},
        {"array_size", required_argument, 0, 'a'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "t:s:a:", long_options, &option_index)) != -1) {
        switch (c) {
            case 't':
                threads_num = atoi(optarg);
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'a':
                array_size = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s --threads_num num --seed num --array_size num\n", argv[0]);
                return 1;
        }
    }
    
    // Проверка обязательных аргументов
    if (threads_num == -1 || seed == -1 || array_size == -1) {
        fprintf(stderr, "Error: All arguments are required\n");
        fprintf(stderr, "Usage: %s --threads_num num --seed num --array_size num\n", argv[0]);
        return 1;
    }
    
    if (threads_num <= 0 || array_size <= 0) {
        fprintf(stderr, "Error: threads_num and array_size must be positive\n");
        return 1;
    }
    
    int* array = malloc(array_size * sizeof(int));
    if (array == NULL) {
        perror("malloc failed");
        return 1;
    }
    GenerateArray(array, array_size, seed);
    
    // Время вычисления суммы
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    long long sum = calculate_parallel_sum(array, array_size, threads_num);
    
    gettimeofday(&end_time, NULL);
    
    // Время выполнения
    double execution_time = (end_time.tv_sec - start_time.tv_sec) + 
                           (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    printf("Array size: %d\n", array_size);
    printf("Threads number: %d\n", threads_num);
    printf("Seed: %d\n", seed);
    printf("Sum: %lld\n", sum);
    printf("Execution time: %.6f seconds\n", execution_time);
    
    free(array);
    return 0;
}