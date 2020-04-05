all: chat_server chat_client

dev: chat_server chat_client

clean: 
	rm src/*.o chat_client chat_server

clean_build:
	rm src/*.o

chat_server: src/chat_server.o
	gcc -ggdb src/chat_server.o -o chat_server -pthread

chat_client: src/chat_client.o
	gcc -ggdb src/chat_client.o -o chat_client -pthread

src/chat_server.o: src/chat_server.c include/chat.h
	gcc -I include -ggdb -c src/chat_server.c -o src/chat_server.o

src/chat_client.o: src/chat_client.c include/chat.h
	gcc -Iinclude -ggdb -c src/chat_client.c -o src/chat_client.o