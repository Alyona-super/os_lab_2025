# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200112L
TARGETS = parallel_min_max zombie_example process_memory parallel_sum

# Файлы объектов
PARALLEL_MIN_MAX_OBJS = parallel_min_max.o utils.o find_min_max.o
PARALLEL_SUM_OBJS = parallel_sum.o utils.o

# Цель по умолчанию
all: $(TARGETS)

# Сборка parallel_min_max
parallel_min_max: $(PARALLEL_MIN_MAX_OBJS)
	$(CC) $(CFLAGS) -o parallel_min_max $(PARALLEL_MIN_MAX_OBJS) -lpthread

# Сборка parallel_sum
parallel_sum: $(PARALLEL_SUM_OBJS)
	$(CC) $(CFLAGS) -o parallel_sum $(PARALLEL_SUM_OBJS) -lpthread

# Сборка zombie_example
zombie_example: zombie_example.c
	$(CC) $(CFLAGS) -o zombie_example zombie_example.c

# Сборка process_memory
process_memory: process_memory.c
	$(CC) $(CFLAGS) -o process_memory process_memory.c

# Компиляция отдельных объектных файлов
parallel_min_max.o: parallel_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c parallel_min_max.c

parallel_sum.o: parallel_sum.c parallel_sum.h utils.h
	$(CC) $(CFLAGS) -c parallel_sum.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) $(CFLAGS) -c find_min_max.c

# Очистка
clean:
	rm -f $(TARGETS) *.o

# Пересборка
rebuild: clean all

# Отладочная сборка
debug: CFLAGS += -g
debug: rebuild

# Оптимизированная сборка
release: CFLAGS += -O2
release: rebuild

.PHONY: all clean rebuild debug release