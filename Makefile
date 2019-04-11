
OBJS = lex.cpp parse.cpp codegen.cpp compiler.cpp

CC = clang++
COMPILER_FLAGS = -w -g `llvm-config --cxxflags --ldflags --system-libs --libs core`

all:	clean rum

rum:	clean $(OBJS)
	$(CC) $(COMPILER_FLAGS) $(OBJS) rum.cpp -o rum

rumi:	clean $(OBJS)
	$(CC) $(COMPILER_FLAGS) $(OBJS) rumi.cpp -o rumi

lex.cpp: lex.l parse.hpp
	flex  -o lex.cpp lex.l

test:
	./rum test.rum
	gcc out.o lib.c

parse.hpp: parse.cpp

parse.cpp: parse.y
	bison -d -l -o parse.cpp parse.y

clean:
	rm a.out test.ll lex.cpp rum rumi parse.cpp parse.hpp || true
