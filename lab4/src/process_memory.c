#include <stdio.h>
#include <stdlib.h>

/* Внешние переменные, определенные компоновщиком */
extern char etext, edata, end;

#define SHW_ADR(ID, I) (printf("ID %s \t is at virtual address: %8p\n", ID, (void*)&I))

int i = 0; /* инициализированная переменная в data сегменте */

void showit(char *msg); /* объявление функции */

int main(void) {
    char *cptr = "Hello World\n"; /* строка в сегменте кода */
    char buffer1[25];
    
    printf("Address of main: %p\n", (void*)main);
    printf("Address of showit: %p\n", (void*)showit);
    
    printf("\nAddress etext: %8p \n", (void*)&etext);
    printf("Address edata: %8p \n", (void*)&edata);
    printf("Address end  : %8p \n", (void*)&end);
    
    SHW_ADR("main", main);
    SHW_ADR("showit", showit);
    SHW_ADR("cptr", cptr);
    SHW_ADR("buffer1", buffer1);
    SHW_ADR("i", i);
    showit("from main"); /* вызов функции */
    
    printf("\nPress Enter to exit...");
    getchar();
    return 0;
}

void showit(char *msg) {
    char *buffer2;
    
    SHW_ADR("msg", msg);
    SHW_ADR("buffer2", buffer2);
    
    buffer2 = (char *) malloc(20); /* динамическое выделение памяти */
    if (buffer2 != NULL) {
        printf("Allocated memory at %p\n", (void*)buffer2);
    }
    
    SHW_ADR("buffer2 after malloc", buffer2);
    
    /* Освобождаем память */
    free(buffer2);
}