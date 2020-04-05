#ifndef CHAT_H
#define CHAT_H

#define DEFAULT_PROTOCOL 0 // The default protocol for a given socket type
#define NO_FLAGS 0 // Specify no flags in send and receive operations
#define CONNECTION_NOT_ESTABLISHED -1 // Connection to server not established
#define PORT 1024 // The port the chatserver is hosted on
#define SERVICE "1024" // The service name for the chat server
#define MAX_MSG_SIZE 140 // The size of the largest messages that can sent
#define MAX_UNAME_SIZE 20 // The maximum size a username is allowed to be
#define EXIT_CMD "~quit\n" // The command that initiates disconnect and quits

#endif