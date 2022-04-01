/*
 Program tworzy gniazdo serwera (numer przydzielany dynamicznie), drukuje
 numer przydzielonego gniazka i oczekuje na nim na propozycje połączeń.
 Do obsługi połączeń wysługuje się podwładnymi, tworzonymi poprzez fork().
 Gdy podwładni pracują, szef odbiera kolejne telefony.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "err.h"
#include "common.h"

#define QUEUE_LENGTH     5

static pid_t make_child(int socket_fd, int client_fd) {
    pid_t pid = fork();
    if (pid < 0) {
        PRINT_ERRNO();
    } else if (pid > 0) {
        // Parent
        CHECK_ERRNO(close(client_fd));
        return pid;
    }

    int pid_int = getpid();

    printf("[%d] Child process starting \n", pid_int);

    // We inherited that socket, but we don't need it, so it should be closed
    CHECK_ERRNO(close(socket_fd));

    char buf[1024];
    ssize_t len;
    do {
        memset(buf, 0, sizeof(buf));
        len = read(client_fd, buf, sizeof(buf));
 	if (len < 0) 
            PRINT_ERRNO(); 
        if (len == 0)
            printf("[%d] Closing connection\n", pid_int);
        else
            printf("[%d]-->%.*s\n", pid_int, (int) len, buf);
    } while (len > 0);

    CHECK_ERRNO(close(client_fd));
    exit(0);
}

int main() {
    printf("My PID = %d\n", (int) getpid());

    int socket_fd = open_socket();
    uint16_t port = bind_socket_to_any_port(socket_fd);
    start_listening(socket_fd, QUEUE_LENGTH);
    printf("Listening on port #%d\n", port);


    for (;;) {
        int client_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
    	if (client_fd == -1)
           	PRINT_ERRNO ();
        pid_t child = make_child(socket_fd, client_fd);
        printf("I'm the parent, child's PID is %d\n", (int) child);
    }
    exit(0);  
}
