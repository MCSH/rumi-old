
OBJS = lex.cpp main.cpp parse.cpp

CC = g++

rum:	clean $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o rum

lex.cpp: lex.l parse.hpp
	flex  -o lex.cpp lex.l

parse.hpp: parse.cpp

parse.cpp: parse.y
	bison -d -l -o parse.cpp parse.y

clean:
	rm lex.cpp rum parse.cpp parse.hpp || true
