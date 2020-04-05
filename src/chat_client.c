#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <poll.h>
#include <chat.h>

// TODO Implement non-canonical terminal interface
// TODO Combine server and client into one process that can act as both
// TODO Fix issues where we free unallocated pointers if we exit early
// TODO Clean up how we handle (or rather don't handle) cleaning up threads

int chat_server; // The socket connection to the server
char *username; // The username for this client
char *r_username; // The username for the remote server
char *server_ip; // The IPv4 address of the server the client connects to

bool was_last_sender; // Was this client the last entity to send a message?
bool connection_established; // Are we connected with a server?

void* receive_buffer; // Buffer that stores received messages
void* send_buffer; // Buffer taht stores the messages to send

pthread_t receiver; // Thread responsible for rreceiving messages from the server
pthread_t sender; // Thread responsible for sending messages to the server
pthread_t handler; // Thread responsible for handling signals

void *send_messages(void* empty) {
    send_buffer = malloc(MAX_MSG_SIZE);
    was_last_sender = true;
    struct pollfd pfd;
    pfd.fd = fileno(stdin);
    pfd.events = POLLIN;

    printf("<%s>: ", username); // Print the initial prompt
    fflush(stdout);
    while(connection_established) {
        if (poll(&pfd, 1, 0) > 0) {
            /*
            * Because fgets blocks our printing calls, our receiver thread 
            * prints the username after the message from the client so that all
            * text the user enters here appear after their username. We should 
            * only print the username if it hasn't already been printed, so we 
            * check to if we were the last sender. If so, we have yet to print 
            * the username
            */
            
            fgets((char*) send_buffer, MAX_MSG_SIZE, stdin);
            send(chat_server, send_buffer, MAX_MSG_SIZE, NO_FLAGS);

            // Check to see if the user is requesting to quit
            if (strcmp((char*) send_buffer, "~quit\n") == 0) {
                pthread_kill(handler, SIGUSR1);
                break;
            }
            
            printf("<%s>: ", username);
            fflush(stdout);
            was_last_sender = true;
            
            // Clear the message buffer to prevent parts of old messages
            // from appearing with new ones
            memset(send_buffer, 0, MAX_MSG_SIZE);
        }
    }

    free(send_buffer);
    pthread_exit(NULL);
}

/* Handles the receiving of messages from the remote client */
void *receive_messages(void* empty) {
    receive_buffer = malloc(MAX_MSG_SIZE);
    struct pollfd pfd;
    pfd.fd = chat_server;
    pfd.events = POLLIN;
    
    while(connection_established) {
        if (poll(&pfd, 1, 0) > 0) {
            int msg_len = 0;
            msg_len = recv(
                chat_server,
                receive_buffer,
                MAX_MSG_SIZE,
                NO_FLAGS
            );

            if (msg_len < 0) {
                perror("In receive_messages: ");
            } else {
                was_last_sender = false;

                if (strcmp((char*) receive_buffer, "~quit\n") == 0) {
                    pthread_kill(handler, SIGUSR2);
                    break;
                }

                printf("\n<%s>: %s", r_username, (char*)receive_buffer); 
                printf("<%s>: ", username);
                fflush(stdout); // Write standard out despite no newline

                memset(receive_buffer, 0, MAX_MSG_SIZE);
            }
        }
    }

    free(receive_buffer);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    struct addrinfo hint;
    struct addrinfo *server_addr, *host; 
    struct sockaddr_in *server_info;

    sigset_t mask;
    int signo;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    handler = pthread_self();

    // The client must specify the address they wish to connect to as a 
    // command line argument
    if (argc < 2) {
        fprintf(stderr, "Error: Please specify and address\n");
        exit(-3);
    }

    /* Set username */
    username = (char*) malloc(MAX_UNAME_SIZE); 
    printf("Please enter a username: ");
    fgets(username, MAX_UNAME_SIZE, stdin);
    *(username + strlen(username) - 1) = '\0';
   
    /* Set-up hints to locat the chat_server host */
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    /* Obtain the host address and open the connection socket */
    int result = getaddrinfo(argv[1], SERVICE, &hint, &server_addr);

    if (result != 0) {
        fprintf(stderr, "Error obtaining address: %s\n", gai_strerror(result));
        exit(-1);
    }

    chat_server = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);

    if (chat_server < 0) {
        fprintf(stderr, "Failed to open socket\n");
        exit(-2);
    }

    /*
     * Make a connection to the chat server. Currently very inefficient as it 
     * spams conenction requests until the connection succeeds instead of 
     * waiting.
     * 
     * #TODO Implement exponential backoff connection retry
     */

    int connection_status = CONNECTION_NOT_ESTABLISHED;
    while (connection_status == CONNECTION_NOT_ESTABLISHED) {
        connection_status = connect(
            chat_server,
            server_addr->ai_addr,
            server_addr->ai_addrlen
        );

        /*
         *If we can't make the connection, close and reopen the socket
         * Some UNIX implementations have undefined behavior when attempting
         * to recall connect on a socket involved in a failed connect call
         */
        if (connection_status == CONNECTION_NOT_ESTABLISHED) {
            close(chat_server);
            chat_server = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
        }
    }
    connection_established = true;

    /* Send the client username and obtain the remote username */

    // Obtain the address of the server
    server_ip = malloc(INET_ADDRSTRLEN);
    server_info = (struct sockaddr_in*) server_addr->ai_addr;
    inet_ntop(AF_INET, &server_info->sin_addr, server_ip, INET_ADDRSTRLEN);

    // Send the client's username
    send(chat_server, (void*)username, strlen(username), NO_FLAGS);

    // Receive the server's username
    r_username = (char*) malloc(MAX_UNAME_SIZE);
    recv(chat_server, (void*)r_username, MAX_UNAME_SIZE, NO_FLAGS);
    printf("Connection established with %s (%s)\n", r_username, argv[1]);

    pthread_sigmask(SIG_SETMASK, &mask, NULL);

    /* Spin up the reception thread */
    pthread_create(&receiver, NULL, receive_messages, NULL);
    pthread_create(&sender, NULL, send_messages, NULL);

    sigwait(&mask, &signo);
    connection_established = false;
    was_last_sender = false;

    switch(signo) {
        case SIGINT:
            send(chat_server, EXIT_CMD, strlen(EXIT_CMD), NO_FLAGS);
        case SIGUSR1:
            printf("\nTerminated connection with %s (%s)\n", r_username, server_ip);
            break;
        case SIGUSR2:
        default: 
            printf("\nTerminated connection by %s (%s)\n", r_username, server_ip);
    }

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    free(username);
    free(r_username);
    free(server_ip);

    return 0;

}