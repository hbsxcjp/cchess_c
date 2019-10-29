head = piece.h 
src = piece.c main.c
obj = piece.o main.o

a.exe: $(obj)
	gcc -std=c11 -Wall $(obj) -o a.exe

$(obj): $(src) $(head)
	gcc -std=c11 -Wall -c $(src)


.PHONY: clean
clean:
	rm a.exe $(obj)