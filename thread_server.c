#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "err.h"
#include "common.h"

#define LINE_SIZE 100
#define QUEUE_LENGTH 5

void *handle_connection(void *client_fd_ptr) {
    int client_fd = *(int *) client_fd_ptr;
    free(client_fd_ptr);

    char *ip = get_ip_from_socket(client_fd);
    int port = get_port_from_socket(client_fd);

    printf("[thread %lu, pid %d] %s:%d connected\n",
           (unsigned long) pthread_self(), getpid(), ip, port);

    char line[LINE_SIZE];
    for (;;) {
        memset(line, 0, sizeof(line));
        ssize_t read = receive_message(client_fd, line, sizeof(line) - 1, NO_FLAGS);
        if (read < 0)
            PRINT_ERRNO();
        if (read == 0)
            break;
        if (line[read - 1] != '\n')
            line[read++] = '\n';
        printf("[thread %lu, pid %d] --> %.*s", (unsigned long) pthread_self(), getpid(), (int) read, line);
    }

    printf("[thread %lu, pid %d] connection closed\n", (unsigned long) pthread_self(), getpid());
    CHECK_ERRNO(close(client_fd));
    return 0;
}

int main() {
    int socket_fd = open_socket();

    int port = bind_socket_to_any_port(socket_fd);
    printf("Listening on port %d\n", port);

    start_listening(socket_fd, QUEUE_LENGTH);

    for (;;) {
        struct sockaddr_in client_addr;

        int client_fd = accept_connection(socket_fd, &client_addr);

        // Arguments for the thread must be passed by pointer
        int *client_fd_pointer = malloc(sizeof(int));
        if (client_fd_pointer == NULL) {
            fatal("malloc");
        }
        *client_fd_pointer = client_fd;

        pthread_t thread;
        CHECK_ERRNO(pthread_create(&thread, 0, handle_connection, client_fd_pointer));
        CHECK_ERRNO(pthread_detach(thread));
    }
}

