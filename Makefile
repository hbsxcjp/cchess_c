# 参考《C语言核心技术》第19章
CC = gcc
CFLAGS = -Wall -std=c11
LDFLAGS = -lm
OBJS = piece.o board.o main.o

a: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJS): *.h

.PHONY: clean
clean:
	rm a *.o