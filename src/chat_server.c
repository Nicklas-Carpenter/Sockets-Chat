#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <poll.h>
#include <chat.h>

// TODO Combine server and client into one process that can act as both
// TODO Fix issues where we free unallocated pointers if we exit early

int listener; // The socket that handles incoming connections
int chat_client; // The socket connection to the client
char *username; // The username for this client
char *r_username; // The username for the remote server
char *client_ip; // The IPv4 address of the client is connected to

bool was_last_sender; // Was this server the last entity to send a message?
bool connection_established; // Are we connected with a client?

void* receive_buffer; // Buffer that stores received messages
void* send_buffer; // Buffer taht stores the messages to send

pthread_t receiver; // Thread responsible for receiving messages from the client
pthread_t sender; // Thread responsible for sending messages to the client
pthread_t handler; // Thread responsible for handling signals

void *send_messages(void* empty) {
    send_buffer = malloc(MAX_MSG_SIZE);
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
            send(chat_client, send_buffer, MAX_MSG_SIZE, NO_FLAGS);

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
    struct pollfd pfd;
    pfd.fd = chat_client;
    pfd.events = POLLIN;

    char* exit_cmd = "~quit\n";
    receive_buffer = malloc(MAX_MSG_SIZE);
    
    while(connection_established) {
        if (poll(&pfd, 1, 0) > 0) {
            int msg_len = 0;
            msg_len = recv(
                chat_client,
                receive_buffer,
                MAX_MSG_SIZE,
                NO_FLAGS
            );

            if (msg_len < 0) {
                perror("In receive_messages: ");
            } else {
                was_last_sender = false;

                if (strcmp((char*) receive_buffer, exit_cmd) == 0) {
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
    struct sockaddr_in server_addr, client_addr;
    sigset_t mask;
    int signo, result;

    // Set the signal mask. Used to establish this thread as the signal handler
    // when the sender and receiver threads are started
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    // Reference to main thread so other threads can send signals here
    handler = pthread_self(); 

    // Set our username
    username = (char*) malloc(MAX_UNAME_SIZE);
    printf("Please enter a username: ");
    fgets(username, MAX_UNAME_SIZE, stdin);
    *(username + strlen(username) - 1) = '\0';

    // Open a socket to listen to incoming connections
    listener = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (listener < 0) {
        perror("In main - failed to open socket");
        return -1;
    }

    // Allow us to reuse this address. Useful if we quit and immdiately 
    // rerun this program
    int reuse_addr = 1;
    result = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));

    if (result < 0) {
        perror("In main - failed to set socket options");
        return -4;
    }

    /* Set-up socket address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    memset(&server_addr.sin_zero, 0, sizeof(server_addr.sin_addr));

    /* Bind the server */
    if (bind(listener, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("In main - failed to bind address");
        return -2;
    }

    /* Obtain connections from clients */
    listen(listener, SOMAXCONN);

    socklen_t client_addr_size = sizeof(client_addr);
    chat_client = accept(
        listener, 
        (struct sockaddr*) &client_addr, 
        (socklen_t* restrict) &client_addr_size);
    
    if (chat_client < 0) {
        fprintf(stderr, "Failed to accept incoming connection\n");
    }

    char* restrict client_ip = malloc(INET_ADDRSTRLEN);
    inet_ntop(
        AF_INET, 
        &client_addr.sin_addr,
        client_ip,
        INET_ADDRSTRLEN);

    connection_established = true;
    r_username = malloc(MAX_UNAME_SIZE);
    recv(chat_client, (void*)r_username, MAX_UNAME_SIZE, NO_FLAGS);
    printf("Connection established with %s (%s)\n", r_username, client_ip);

    send(chat_client, (void*)username, strlen(username), NO_FLAGS);

    // Block signals; we use this thread as a signal handler with 
    // 
    pthread_sigmask(SIG_SETMASK, &mask, NULL);

    /* Spin up the reception thread */
    pthread_create(&receiver, NULL, receive_messages, NULL);
    pthread_create(&sender, NULL, send_messages, NULL);

    sigwait(&mask, &signo);
    connection_established = false;

    switch(signo) {
        case SIGINT:
            send(chat_client, EXIT_CMD, strlen(EXIT_CMD), NO_FLAGS);
        case SIGUSR1:
            printf("\nTerminated connection with %s (%s)\n", r_username, client_ip);
            break;
        case SIGUSR2:
        default: 
            printf("\nTerminated connection by %s (%s)\n", r_username, client_ip);
    }

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    close(chat_client);
    close(listener);

    free(username);
    free(r_username);
    free(client_ip);

    return 0;
}