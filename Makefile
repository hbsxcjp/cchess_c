# 参考《C语言核心技术》第19章

CC = gcc
#CFLAGS = -Wall -std=c11
CFLAGS = -Wall -std=c11 -fexec-charset=gbk # -g 
#LDFLAGS = -L/C/msys64/mingw64/lib -lpcre16 # /C/msys64/mingw64/lib/libpcre16.a
LDFLAGS = -L/C/msys32/mingw32/lib -lpcre16 lib/pdcurses.a
SP = src/
OP = obj/
OBJS = $(OP)tools.o $(OP)piece.o $(OP)board.o $(OP)move.o $(OP)chessManual.o $(OP)console.o $(OP)main.o
FIXEDOBJ = $(OP)cJSON.o # 固定的目标文件，一般只编译一次

a.exe: $(OBJS) $(FIXEDOBJ)
	$(CC) -Wall -o $@ $^ $(LDFLAGS) 

	
$(OBJS): $(OP)%.o : $(SP)%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJS): $(OP)%.o : $(SP)/head/%.h $(SP)/head/base.h

$(FIXEDOBJ): $(OP)%.o : $(SP)%.c
	$(CC) $(CFLAGS) -o $@ -c $<

#以下依赖条件替换为所有.c文件，通过-M预处理发现所有.h文件的变动
#dependencies: $(OBJS: .o=.c)
#	$(CC) -M $^ > $@\
#
#include dependencies

.PHONY: clean
clean:
	rm a obj/*.o
