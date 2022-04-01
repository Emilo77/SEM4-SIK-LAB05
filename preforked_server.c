/*
 Uwaga - to jest przykład jak NIE NALEŻY pisać serwera wieloprocesowego
 
 Program tworzy gniazdo serwera (numer przydzielany dynamicznie), drukuje
 numer przydzielonego gniazka i oczekuje na nim na propozycje połączeń.
 Do obsługi połączeń wysługuje się podwładnymi, tworzonymi poprzez fork().
 Szef nic nie robi, a wolny podwładny odbiera kolejny telefon.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

#include "err.h"
#include "common.h"

#define QUEUE_LENGTH     5
#define NFORKS           4

static pid_t pids[NFORKS];

static void handle_signal(int signal) {

    // Stop all children
    for (int i = 0; i < NFORKS; i++) {
        kill(pids[i], SIGTERM);
    }

    while (wait(NULL) > 0);

    fprintf(stderr, "Signal %d catched.\n", signal);
}

static pid_t make_child(int socket_fd) {
    pid_t pid = fork();
    if (pid < 0) {
        PRINT_ERRNO();
    } else if (pid > 0) {
        // Parent
        return pid;
    }

    // Child starts working
    pid = getpid();
    printf("[%d] Child starting\n", (int) pid);

    int pid_int = (int) pid;

    char buf[1024];
    ssize_t read;
    for (;;) {
        struct sockaddr_in client_addr;
        int client_socket = accept_connection(socket_fd, &client_addr);

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        uint16_t client_port = ntohs(client_addr.sin_port);

        printf("[%d] Client %s:%d connected\n", pid_int, client_ip, client_port);
        do {
            memset(buf, 0, sizeof(buf));
            read = receive_message(client_socket, buf, sizeof(buf), 0);
            if (read < 0)
                PRINT_ERRNO();
            if (read == 0) {
                printf("[%d] Ending connection\n", pid_int);
            } else {
                printf("[%d]-->%.*s\n", pid_int, (int) read, buf);
            }
        } while (read > 0);

        CHECK_ERRNO(close(client_socket));
    }
    return 1;
}

int main() {
    printf("My PID = %d\n", (int) getpid());

    int socket_fd = open_socket();

    uint16_t port = bind_socket_to_any_port(socket_fd);
    printf("Socket port #%d\n", port);

    start_listening(socket_fd, QUEUE_LENGTH);

    for (int i = 0; i < NFORKS; i++) {
        pids[i] = make_child(socket_fd);
    }

    install_signal_handler(SIGINT, handle_signal);

    // Wait for children
    while (wait(NULL) > 0);

    exit(0);
}

