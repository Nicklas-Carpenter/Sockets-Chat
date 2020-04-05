# sockets_chat
A set of simple chat programs using the C Sockets API

## Description
sockets_chat is a terminal-based chat application that allows communication between two users over a network. Hypothetically, if the server is port-forwarded, the client and the server can be on different networks, although this has not been tested. sockets_chat is comprised of two programs, a client and a server. The server program starts a host on port 1024. The client attempts to connect a server on port 1024 with the address that is passed. Once a connection is established between the client and the server, the two programs act virtually identically. 

## Usage
This application should work on virtually any UNIX-based system (e.g. GNU/Linux, BSD, macOS).
### Requirements
The user must have the following packages installed on their system:
* gcc
* make
* Standard C libraries, including:
  * unistd.h
  * stdlib.h
  * string.h
  * stdbool.h
* Standard network libraries, including:
  * netinet.in/h
  * arpa/inet.h
* Standard UNIX libraries including:
  * sys/sockets.h
  * signal.h
  * pthread.h
  * poll.h
* git (in order to use the recommended way to obtain the programs) 

Most UNIX-based systems should come with all the libraries pre-installed.

### Building
1. Open a terminal and move to a desired working directory
2. Clone the repository by running: 
```bash
git clone https://github.com/Nicklas-Carpenter/sockets_chat.git
```
3. Alternatively, download the repository as a zip into the desired working directory and extract the zip file
4. Within the working directory, move into the repository with 
```bash
cd sockets_chat`
```
5. Once in the repository, build the executable by executing:
```bash
make
```

### Running
#### Starting the chat server
1. To start the chat server, simply run:
```bash
./chat_server
```
2. You will then be prompted for username. Enter a username and hit return
3. The server will wait for a connection from a client

#### Starting the chat client
1. To start the client run:
```bash
./chat_client [address]
```
Where `[address]` is the IPv4 address of the machine running chat_server (in the form xxx.xxx.xxx.xxx)

2. You will be prompted for a username. Enter a username and hit return

#### Messaging
1. When the server or client discovers a connection, it will indicate this with a message that includes the username and IPv4 address of the discovered user
2. Both the client and the server function identically when messaging. Received messages will appear on as they come in. To send a message, type out the message contents and hit return
3. To exit, type `~quit` and hit return. The connection will terminate and both the server and the client will quit

## Known Issues
* sockets_chat currently uses canonical terminal output. This leads to the following complications:
  1. In order to guarantee the presence of the username prompt whenever the user is typing, the prompt is reprinted when a message is received. 
  2. Since input and output share the same screen, and characters must be printed as they are typed, if a user receives a message as they are typing a message, the text of their message will appear disjoint between lines

* When the user forcibly exits either chat_client or chat_server (e.g. by entering control-c) before a connection is established, sockets_chat will leak memory. This is becuase the main thread is reused as a signal handling thread when a conenction has been estalbished and threads for sending and receiving messages have been issues. The method used for signal handling requires the masking of signals. The signal masking does not occur until a network connection has been established (so the user can quit the program before a network connection is made), but a handler is not installed at this point, so the program has no chance to free any allocated pointers.
