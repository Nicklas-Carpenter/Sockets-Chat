CC = gcc -ggdb
EXEC_FLAGS = -pthread
OBJS_FLAGS = -Iinclude -c

.PHONY: build test all clean remote_test

all: build test
build: bin/chat_server bin/chat_client bin/sockets_chat
test: bin/atoi_test bin/getopt_test bin/noncanon_test bin/regex_test \
      bin/curses_test remote_test

remote_test: bin/remote_test_parent bin/remote_test_child

clean: 
	rm objs/*.o

# Primary executables
bin/chat_server: objs/chat_server.o
	$(CC) $(EXEC_FLAGS) objs/chat_server.o -o bin/chat_server

bin/chat_client: objs/chat_client.o
	$(CC) $(EXEC_FLAGS) objs/chat_client.o -o bin/chat_client

bin/sockets_chat: objs/sockets_chat.o
	$(CC) $(EXEC_FLAGS) objs/sockets_chat.o -o bin/sockets_chat

# Primary objects
objs/chat_server.o: src/chat_server.c include/chat.h
	$(CC) $(OBJS_FLAGS) src/chat_server.c -o objs/chat_server.o

objs/chat_client.o: src/chat_client.c include/chat.h
	$(CC) $(OBJS_FLAGS) src/chat_client.c -o objs/chat_client.o

objs/sockets_chat.o: src/sockets_chat.c include/chat.h
	$(CC) $(OBJS_FLAGS) src/sockets_chat.c -o objs/sockets_chat.o