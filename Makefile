
OBJS = lex.cpp main.cpp parse.cpp codegen.cpp

CC = g++
COMPILER_FLAGS = -w -g `llvm-config --cxxflags --ldflags --system-libs --libs core`

rum:	clean $(OBJS)
	$(CC) $(COMPILER_FLAGS) $(OBJS) -o rum

lex.cpp: lex.l parse.hpp
	flex  -o lex.cpp lex.l

parse.hpp: parse.cpp

parse.cpp: parse.y
	bison -d -l -o parse.cpp parse.y

clean:
	rm lex.cpp rum parse.cpp parse.hpp || true
