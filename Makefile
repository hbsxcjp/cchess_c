# 参考《C语言核心技术》第19章

CC = gcc
#CFLAGS = -Wall -std=c11
CFLAGS = -g -Wall -std=c11
LDFLAGS = -L/C/msys64/mingw64/lib -lpcre16 # /C/msys64/mingw64/lib/libpcre16.a
OBJS = obj/cJSON.o obj/tools.o obj/piece.o obj/board.o obj/move.o obj/instance.o obj/view.o obj/main.o

a: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) 
	
$(OBJS): obj/%.o : src/%.c
	$(CC) $(CFLAGS) -g -o $@ -c $<

$(OBJS): src/head/*.h
#dependencies: $(OBJS: .o=.c)
#	$(CC) -M $^ > $@\
#
#include dependencies

.PHONY: clean
clean:
	rm a obj/*.o