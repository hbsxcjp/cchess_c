# 参考《C语言核心技术》第19章

CC = gcc
CFLAGS = -Wall -std=c11
LDFLAGS = -lm
OBJS = obj/tools.o obj/piece.o obj/board.o obj/move.o obj/instance.o obj/main.o

a: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJS): obj/%.o : src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJS): src/head/*.h
#dependencies: $(OBJS: .o=.c)
#	$(CC) -M $^ > $@\
#
#include dependencies

.PHONY: clean
clean:
	rm a obj/*.o