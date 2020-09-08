# 参考《C语言核心技术》第19章

#ifdef __linux
FEXEC_CHARSET = uft-8
LPCRE = -lpcre32
#else
FEXEC_CHARSET = GBK
LPCRE = -lpcre16
#endif

CC = gcc
#CFLAGS = -Wall -std=c11
CFLAGS = -Wall -std=c11 -finput-charset=utf-8 -fexec-charset=$(FEXEC_CHARSET) -g # GBK utf-8
#LDFLAGS = -L/C/msys64/mingw64/lib -lpcre16 # /C/msys64/mingw64/lib/libpcre16.a
LDFLAGS = -lm -lcunit LPCRE #lib/pdcurses.a -lpcre32
SP = src/
OP = obj/
OBJS = $(OP)sha1.o $(OP)md5.o $(OP)tools.o $(OP)piece.o $(OP)board.o $(OP)move.o $(OP)aspect.o $(OP)chessManual.o $(OP)play.o $(OP)ecco.o $(OP)unitTest.o $(OP)main.o # $(OP)console.o
FIXEDOBJ = $(OP)cJSON.o # 固定的目标文件，一般只编译一次

a: $(OBJS) $(FIXEDOBJ)
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
