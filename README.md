# sockets_chat
sockets_chat is a terminal-based chat application, utilizing the Unix Sockets API,
that allows instant messaging communication between two users over a TCP/IP.

## Table of Contents
* [sockets_chat](#sockets_chat)
* [Table of Contents](#table-of-contents)
* [Installation](#Installation)
  * [Requirements](#Requirements)
  * [Building](#Building)
* [Usage](#Usage)
  * [Running in host mode](#running-in-host-mode)
  * [Running in client mode](#running-in-client-mode)
  * [Messaging](#Messaging)
  * [Testing](#Testing)
* [Known Issues](#known-issues)
* [License](#License)

## Installation
This application should work on virtually any UNIX-based system (e.g.
GNU/Linux, BSD, macOS).
### Requirements
The user must have the following libraries and packages installed on their
system. Most of these should be present by default in most Unix-based systems:
* gcc
* make
* git (The recommended way to obtain the program)
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

### Building
1. Open a terminal and move to a desired working directory
2. Clone the repository by running: 
```bash
git clone https://github.com/Nicklas-Carpenter/sockets_chat.git
```
3. Alternatively, download the repository as a zip into the desired working 
   directory and extract the zip file
4. Within the working directory, move into the repository with 
```bash
cd sockets_chat`
```
5. Once in the repository, build the executable by executing:
```bash
make
```
6. The resulting executable will be written to the `bin` directory

## Usage
sockets_chat can be run in two modes: host in client. Running sockets_chat in
host mode opens a chat server that an instance of sockets_chat running in
client mode can connect to. Currently, messaging can only occur between one
client and one host.

### Running in host mode
1. To run sockets_chat in host mode, execute the following in the main
   sockets_chat directory:
```bash
bin/sockets_chat -h -p PORT
```
Where `-h` specifies host mode and `PORT` is the network port on which to host
the chat server
2. You will then be prompted for username. Enter a username and hit return
3. The server will wait for a connection from a client

### Running in client mode
1. To run sockets_chat in client mode, execute the following in the main
   sockets_chat directory:
```bash
bin/sockets_chat -a ADDRESS -p PORT
```
Where `ADDRESS` is the IPv4 address of the machine running chat_server (in the
form \xxx.xxx.xxx.xxx\) and `PORT` is the port the server is running on

2. You will be prompted for a username. Enter a username and hit return

### Messaging
1. When the host or client discovers a connection, it will indicate this with
   a message that includes the username and IPv4 address of the discovered user
2. Both the host and the client function identically when messaging.
   Received messages will appear on as they come in. To send a message, type
   out the message contents and hit return
3. To exit, type `~quit` and hit return or press `control-c` on either the host 
   or client . The connection will terminate and both the host and client 
   processes will exit

### Testing
The simplest way to run sockets_chat is to run both the host and the client on
the same machine. Utilizing the process described in [usage](#Usage), do the
following:
1. Start the host with a non-reserved port (i.e. choose a port between `1024`
   and `65535` inclusive).
2. In a seperate terminal, run the client with the same port and the address
   `127.0.0.1`

## Known Issues
* sockets_chat currently uses canonical terminal output. This leads to the
  following complications:
  1. In order to guarantee the presence of the username prompt whenever the
     user is typing, the prompt is reprinted when a message is received.
  2. Since input and output share the same screen, and characters must be
      printed as they are typed, if a user receives a message as they are
      typing a message, the text of their message will appear disjoint between
      lines
* If the client is attempting to connect to a host on the same machine (e.g.
  127.0.0.1) and the host process is not running, the client will connect to
  itself.

## License
This project is licensed under the GNU GPL v3.0 or greater. For more
information see [LICENSE](LICENSE)
