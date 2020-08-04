// sockets_chat - A Unix sockets-based chat program
// Copyright (C) 2020  Nicklas Carpenter

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <poll.h>
#include <getopt.h>
#include <regex.h>
#include <chat.h>

// TODO Improve option handling (e.g. add a USAGE statement)
// TODO Can we daemonize a server
//      --executor; -e  : Server host is not a user, only a relay
//      --daemonize; -d : Host server runs in the background as a daemon?
// TODO Look into security considerations
// TODO Add color and formatting options

int listener; // The socket that handles incoming connections
int remote; // The socket connection to the client
char *username; // The username for this client
char *r_username; // The username for the remote server
char *remote_ip; // The IPv4 address of the remote device

bool was_last_sender; // Was this server the last entity to send a message?
bool connection_established; // Are we connected with a client?

void* receive_buffer; // Buffer that stores received messages
void* send_buffer; // Buffer that stores the messages to send

pthread_t receiver; // Thread responsible for receiving messages from the client
pthread_t sender; // Thread responsible for sending messages to the client
pthread_t handler; // Thread responsible for handling signals

void catch_signal(const int signo);
void wait_for_client_connection(const int port, const char *address);
void connect_to_host(const char *service, const char *address);
void *send_messages(void* empty);
void *receive_messages(void *empty);

int main(int argc, char **argv) {
    struct sigaction sig_handler, def_action;
    sigset_t mask;
    int port;
    char mode;
    char *address, *service;

    // Set up initial signal handling
    sigemptyset(&mask);

    // Set the signal mask. Used to establish this thread as the signal handler
    // when the sender and receiver threads are started
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    sig_handler.sa_handler = catch_signal;
    sig_handler.sa_mask = mask;
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, &def_action);

    // Obtain commandline options
    regmatch_t matches[2];
    regex_t regex;
    bool port_not_specified = true;
    bool address_not_specified = true;
    char opt, *arg_str = "p:a:h", **end_ptr = malloc(sizeof(char**));
    opterr = 0;
    mode = CLIENT;

    while((opt = getopt(argc, argv, arg_str)) > 0) {
        switch(opt) {
            case 'h':
                mode = HOST;
                break;
            case 'p':
                // TODO Perform validation on the received port
                service = optarg;
                long num_conv = strtol(service, end_ptr, 10);

                // The last valid character was something other than '\0', so
                // the string contains non-number components. Throw an error
                if (**end_ptr != '\0') {
                    fprintf(stderr, "%s is not a valid port", service);
                    return 2;
                }

                if (num_conv < PORT_MIN || num_conv > PORT_MAX) {
                    fprintf(
                        stderr, 
                        "Port %d is out of range (must be between %d and %d)\n",
                        num_conv, PORT_MIN, PORT_MAX
                    );

                    return 3;
                }

                // Safe cast since we already checked if 
                port = (int)num_conv;      
                port_not_specified = false;

                break;
            case 'a':
                address = optarg;

                regcomp(&regex, IPV4_REGEX, REG_EXTENDED);
                regexec(&regex, address, 1, matches, 0);

                if (matches[0].rm_so != 0 || matches[0].rm_eo != strlen(address)) {
                    fprintf(stderr, "%s is not a valid IPV4 address\n", address);
                    return 4;
                }
                regfree(&regex);
                address_not_specified = false;

                break;
            default:
                fprintf(stderr, "Invalid option: \"-%c\"\n", opt);
                return 1;
        }
    }

    if (port_not_specified) {
        fputs("Error: Missing port\n", stderr);
        return 5;
    }

    if (address_not_specified) {
        fputs("Error: Missing address\n", stderr);
        return 6;
    }

    // Reference to main thread so other threads can send signals here
    handler = pthread_self(); 

    // Set our username
    username = (char*) malloc(MAX_UNAME_SIZE);
    printf("Please enter a username: ");
    fgets(username, MAX_UNAME_SIZE, stdin);
    *(username + strlen(username) - 1) = '\0';

    if (mode == HOST) {
        wait_for_client_connection(port, address);
    } else {
        connect_to_host(service, address);
    }

    // Block signals; we use this thread as a signal handler. Uninstall our
    // old interrupt handler
    sigaction(SIGINT, &def_action, NULL);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);

    // Spin up the recption thread
    pthread_create(&receiver, NULL, receive_messages, NULL);
    pthread_create(&sender, NULL, send_messages, NULL);

    int signo;
    sigwait(&mask, &signo);
    connection_established = false;

    switch(signo) {
        case SIGINT:
            send(remote, EXIT_CMD, strlen(EXIT_CMD), NO_FLAGS);
        case SIGUSR1:
            printf("\nTerminated connection with %s (%s)\n", r_username, remote_ip);
            break;
        case SIGUSR2:
        default: 
            printf("\nTerminated connection by %s (%s)\n", r_username, remote_ip);
    }

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    close(remote);

    free(username);
    free(r_username);
    free(remote_ip);

    return 0;
}

void catch_signal(const int signo) {
    if (username != 0) {
        free(username);
    }

    if (r_username != 0) {
        free(r_username);
    }

    exit(-1);
}

void wait_for_client_connection(const int port, const char *address) {
    // Open a socket to listen to incoming connections
    listener = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (listener < 0) {
        perror("In main - failed to open socket");
        exit(-2);
    }

    // Allow us to reuse this address. Useful if we quit and immdiately 
    // rerun this program
    int reuse_addr = 1;
    int result = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));

    if (result < 0) {
        perror("In main - failed to set socket options");
        exit(-3);
    }

    struct sockaddr_in local_addr, remote_addr;

    /* Set-up socket address */
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    
    memset(&local_addr.sin_zero, 0, sizeof(local_addr.sin_addr));

    /* Bind the server */
    if (bind(listener, (struct sockaddr*) &local_addr, sizeof(local_addr)) < 0) {
        perror("In main - failed to bind address");
        exit(-4);
    }

    /* Obtain connections from clients */
    listen(listener, SOMAXCONN);

    // We found a client, attempt to establish communication
    socklen_t remote_addr_size = sizeof(remote_addr);
    remote = accept(
        listener, 
        (struct sockaddr*) &remote_addr, 
        (socklen_t* restrict) &remote_addr_size
    );
    
    if (remote < 0) {
        fprintf(stderr, "Failed to accept incoming connection\n");
    }

    // TODO Allow multiple conenctions in the future 
    close(listener);

    remote_ip = malloc(INET_ADDRSTRLEN);
    inet_ntop(
        AF_INET, 
        &remote_addr.sin_addr,
        remote_ip,
        INET_ADDRSTRLEN
    );

    int u_length;
    connection_established = true;
    r_username = malloc(MAX_UNAME_SIZE);
    u_length = recv(remote, (void*)r_username, MAX_UNAME_SIZE, NO_FLAGS);
    *(r_username + u_length) = '\0';
    printf("Connection established with %s (%s)\n", r_username, remote_ip);

    send(remote, (void*)username, strlen(username), NO_FLAGS);
}

void connect_to_host(const char* service, const char* address) {
    struct addrinfo *remote_addr, *host, hint; 
    struct sockaddr_in *server_info;

    /* Set-up hints to locat the chat_server host */
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    /* Obtain the host address and open the connection socket */
    int result = getaddrinfo(address, service, &hint, &remote_addr);

    if (result != 0) {
        fprintf(stderr, "Error obtaining address: %s\n", gai_strerror(result));
        exit(-1);
    }

    remote = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);

    if (remote < 0) {
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
            remote,
            remote_addr->ai_addr,
            remote_addr->ai_addrlen
        );

        /*
        *If we can't make the connection, close and reopen the socket
        * Some UNIX implementations have undefined behavior when attempting
        * to recall connect on a socket involved in a failed connect call
        */
        if (connection_status == CONNECTION_NOT_ESTABLISHED) {
            close(remote);
            remote = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
        }
    }
    connection_established = true;

    /* Send the client username and obtain the remote username */

    // Obtain the address of the server
    remote_ip = malloc(INET_ADDRSTRLEN);
    server_info = (struct sockaddr_in*) remote_addr->ai_addr;
    inet_ntop(AF_INET, &server_info->sin_addr, remote_ip, INET_ADDRSTRLEN);

    // Send the client's username
    send(remote, (void*)username, strlen(username), NO_FLAGS);

    // Receive the server's username
    int u_length;
    connection_established = true;
    r_username = malloc(MAX_UNAME_SIZE);
    u_length = recv(remote, (void*)r_username, MAX_UNAME_SIZE, NO_FLAGS);
    *(r_username + u_length) = '\0';
    printf("Connection established with %s (%s)\n", r_username, remote_ip);
}

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
            send(remote, send_buffer, MAX_MSG_SIZE, NO_FLAGS);

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
    pfd.fd = remote;
    pfd.events = POLLIN;

    char* exit_cmd = "~quit\n";
    receive_buffer = malloc(MAX_MSG_SIZE);
    
    while(connection_established) {
        if (poll(&pfd, 1, 0) > 0) {
            int msg_len = 0;
            msg_len = recv(
                remote,
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