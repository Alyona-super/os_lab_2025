#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    printf("Parent process (PID: %d): Starting child process...\n", getpid());
    
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Child process (PID: %d): Executing sequential_min_max...\n", getpid());
        
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};

        execv("./sequential_min_max", args);
        
        perror("execv failed");
        exit(1);
    } else if (pid > 0) {
        int status;
        wait(&status);
        printf("Parent process: Child finished with status %d\n", status);
    } else {
        perror("fork failed");
        return 1;
    }
    
    return 0;
}