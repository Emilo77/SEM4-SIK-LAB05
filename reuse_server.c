#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "err.h"
#include "common.h"

#define BUFFER_SIZE 1024
#define QUEUE_LENGTH 5

int main(int argc, char *argv[]) {
    if (argc != 2)
        fatal("Usage: %s port_number", argv[0]);

    int port = read_port(argv[1]);

    int socket_fd = open_socket();

    // Needs to be called before bind()
    set_port_reuse(socket_fd);

    bind_socket(socket_fd, port);

    start_listening(socket_fd, QUEUE_LENGTH);

    printf("Process %d starts listening on port %d.\n", getpid(), port);

    do {
        struct sockaddr_in client_addr;
        int client_fd = accept_connection(socket_fd, &client_addr);

        uint16_t client_port = get_port(&client_addr);
        char *client_ip = get_ip(&client_addr);
        printf("Accepted connection from %s:%d.\n", client_ip, client_port);

        ssize_t read;
        do {
            char buffer[BUFFER_SIZE];

            read = receive_message(client_fd, buffer, BUFFER_SIZE, 0);
            if (read < 0)
                PRINT_ERRNO();
            if (read == 0)
                printf("[%s:%d] Closing connection from\n", client_ip, client_port);
            else
                printf("[%s:%d] -> \"%.*s\"\n", client_ip, client_port, (int) read, buffer);
        } while (read > 0);

        CHECK_ERRNO(close(client_fd));
    } while (1);

    return 0;
}
