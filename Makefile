
OBJS = lex.cpp main.cpp

rum:	clean $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o rum

lex.cpp: lex.l
	flex  -o lex.cpp lex.l

clean:
	rm lex.cpp rum || true
