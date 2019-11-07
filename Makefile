# 参考《C语言核心技术》第19章
CC = gcc
CFLAGS = -Wall -std=c11
LDFLAGS = -lm
OBJS = piece.o board.o move.o instance.o main.o

a: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJS): *.h
#dependencies: $(OBJS: .o=.c)
#	$(CC) -M $^ > $@\
#
#include dependencies

.PHONY: clean
clean:
	rm a *.o