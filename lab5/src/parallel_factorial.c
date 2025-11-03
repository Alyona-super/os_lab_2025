#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>

// Глобальные переменные
long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи аргументов в поток
typedef struct {
    int start;
    int end;
    int mod;
} thread_args_t;

// Функция для вычисления частичного произведения
void* partial_factorial(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    long long partial_result = 1;
    
    // Вычисляем частичное произведение
    for (int i = args->start; i <= args->end; i++) {
        partial_result = (partial_result * i) % args->mod;
    }
    
    // Синхронизированно обновляем общий результат
    pthread_mutex_lock(&mutex);
    result = (result * partial_result) % args->mod;
    pthread_mutex_unlock(&mutex);
    
    pthread_exit(NULL);
}

// Функция для разбора аргументов командной строки
void parse_arguments(int argc, char* argv[], int* k, int* pnum, int* mod) {
    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'k':
                *k = atoi(optarg);
                break;
            case 'p':
                *pnum = atoi(optarg);
                break;
            case 'm':
                *mod = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
                exit(1);
        }
    }
    
    // Проверка обязательных аргументов
    if (*k <= 0 || *pnum <= 0 || *mod <= 0) {
        fprintf(stderr, "Error: All parameters must be positive integers\n");
        fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    int k = 0, pnum = 0, mod = 0;
    
    // Разбор аргументов
    parse_arguments(argc, argv, &k, &pnum, &mod);
    
    printf("Computing %d! mod %d using %d threads\n", k, mod, pnum);
    
    // Создание потоков
    pthread_t threads[pnum];
    thread_args_t thread_args[pnum];
    
    // Распределение работы между потоками
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        int numbers_for_this_thread = numbers_per_thread;
        if (i < remainder) {
            numbers_for_this_thread++;
        }
        
        thread_args[i].start = current_start;
        thread_args[i].end = current_start + numbers_for_this_thread - 1;
        thread_args[i].mod = mod;
        
        current_start += numbers_for_this_thread;
        
        printf("Thread %d: computing product from %d to %d\n", 
               i, thread_args[i].start, thread_args[i].end);
        
        // Создание потока
        if (pthread_create(&threads[i], NULL, partial_factorial, &thread_args[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }
    
    // Вывод результата
    printf("\nResult: %d! mod %d = %lld\n", k, mod, result);
    
    return 0;
}