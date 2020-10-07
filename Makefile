CC = gcc -ggdb
EXEC_FLAGS = -pthread
OBJS_FLAGS = -Iinclude -c

.PHONY: clean

bin/sockets_chat: objs/sockets_chat.o
	$(CC) $(EXEC_FLAGS) objs/sockets_chat.o -o bin/sockets_chat

objs/sockets_chat.o: src/sockets_chat.c include/chat.h
	$(CC) $(OBJS_FLAGS) src/sockets_chat.c -o objs/sockets_chat.o

clean:
	rm objs/*.o bin/*