#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[SIGCHLD] Процесс %d завершен с статусом %d\n", pid, WEXITSTATUS(status));
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Использование: %s <режим>\n", argv[0]);
        printf("Режимы:\n");
        printf("  zombie - создать зомби-процесс\n");
        printf("  wait   - нормальное завершение с wait\n");
        printf("  signal - использование обработчика SIGCHLD\n");
        return 1;
    }

    printf("Родительский процесс PID: %d\n", getpid());

    if (strcmp(argv[1], "zombie") == 0) {
        printf("\n=== РЕЖИМ: СОЗДАНИЕ ЗОМБИ-ПРОЦЕССА ===\n");
        
        pid_t pid = fork();
        
        if (pid == 0) {
            printf("Дочерний процесс PID: %d\n", getpid());
            printf("Дочерний процесс завершается...\n");
            exit(42);
        } else if (pid > 0) {
            printf("Родитель создал дочерний процесс с PID: %d\n", pid);
            printf("Родитель НЕ вызывает wait() - процесс станет зомби!\n");
            printf("Запустите 'ps aux | grep %d' в другой консоли чтобы увидеть зомби\n", pid);
            printf("Родитель спит 30 секунд...\n");
            sleep(30);
            printf("Родитель завершается - зомби исчезнет\n");
        }

    } else if (strcmp(argv[1], "wait") == 0) {
        printf("\n=== РЕЖИМ: НОРМАЛЬНОЕ ЗАВЕРШЕНИЕ С WAIT ===\n");
        
        pid_t pid = fork();
        
        if (pid == 0) {
            printf("Дочерний процесс PID: %d\n", getpid());
            printf("Дочерний процесс завершается...\n");
            exit(42);
        } else if (pid > 0) {
            printf("Родитель создал дочерний процесс с PID: %d\n", pid);
            printf("Родитель вызывает wait()...\n");
            
            int status;
            pid_t finished_pid = wait(&status);
            
            if (finished_pid > 0) {
                printf("Дочерний процесс %d завершен с кодом %d\n", 
                       finished_pid, WEXITSTATUS(status));
                printf("Зомби-процесс не создан!\n");
            }
        }

    } else if (strcmp(argv[1], "signal") == 0) {
        printf("\n=== РЕЖИМ: ОБРАБОТЧИК SIGCHLD ===\n");
        
        struct sigaction sa;
        sa.sa_handler = sigchld_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            return 1;
        }
        
        printf("Установлен обработчик SIGCHLD\n");
        
        // Несколько дочерних процессов
        for (int i = 0; i < 3; i++) {
            pid_t pid = fork();
            
            if (pid == 0) {
                printf("Дочерний процесс %d (PID: %d) запущен\n", i, getpid());
                sleep(i + 1); // Разное время выполнения для дочерних классов
                printf("Дочерний процесс %d завершается\n", i);
                exit(i + 10);
            } else if (pid > 0) {
                printf("Родитель создал дочерний процесс %d с PID: %d\n", i, pid);
            }
        }
        
        printf("Родительский процесс работает 10 секунд...\n");
        printf("Дочерние процессы будут завершаться асинхронно\n");
        sleep(10);
        printf("Родительский процесс завершается\n");

    } else {
        printf("Неизвестный режим: %s\n", argv[1]);
        return 1;
    }

    return 0;
}