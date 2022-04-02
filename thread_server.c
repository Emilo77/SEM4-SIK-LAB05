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

#define LINE_SIZE 1024
#define QUEUE_LENGTH 5

size_t sumOfData = 0;

void write_file(int sockfd, char *filename) {
	size_t read;
	FILE *fp;

	char line[LINE_SIZE];
	memset(line, 0, LINE_SIZE);

//	fp = fopen(filename, "ab+");
	fp = fopen("result.txt", "w+");
	if (fp == NULL) {
		fatal("Could not open file");
	}
	while (1) {
		read = receive_message(sockfd, line, LINE_SIZE, 0);
		if (read < 0)
			PRINT_ERRNO();
		if (read == 0)
			break;
		fputs(line, fp);
	}
	fclose(fp);
	printf("File received successfully\n");
}

void *handle_connection(void *client_fd_ptr) {
	int client_fd = *(int *) client_fd_ptr;
	free(client_fd_ptr);

	char *ip = get_ip_from_socket(client_fd);
	int port = get_port_from_socket(client_fd);

	char line[LINE_SIZE];
	memset(line, 0, sizeof(line));
	size_t read = receive_message(client_fd, line, sizeof(line) - 1, NO_FLAGS);
	if (read <= 0) {
		PRINT_ERRNO();
	}
	char *token = strtok(line, " ");
	char *filename = token;
	token = strtok(NULL, " ");
	char *file_size_str = token;
	size_t file_size = strtoul(file_size_str, NULL, 10);

	printf("new client [%s:%d] size=[%lu bytes] file=[%s]\n", ip, port,
	       file_size, filename);

	write_file(client_fd, filename);

	//TODO MUTEX P
	sumOfData += file_size;
	//TODO MUTEX Q

	printf("Total data achieved: %lu\n", sumOfData);

	printf("[thread %lu, pid %d] connection closed\n",
	       (unsigned long) pthread_self(), getpid());
	CHECK_ERRNO(close(client_fd));
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fatal("Usage: %s <port>", argv[0]);
	}

	int port = read_port(argv[1]);

	int socket_fd = open_socket();
	bind_socket(socket_fd, port);

	start_listening(socket_fd, QUEUE_LENGTH);
	printf("Listening on port %d\n", port);

	for (;;) {
		struct sockaddr_in client_address;
		int client_fd = accept_connection(socket_fd, &client_address);

		// Arguments for the thread must be passed by pointer
		int *client_fd_pointer = malloc(sizeof(int));
		if (client_fd_pointer == NULL) {
			fatal("malloc");
		}
		*client_fd_pointer = client_fd;

		pthread_t thread;
		CHECK_ERRNO(pthread_create(&thread, 0, handle_connection,
		                           client_fd_pointer));
		CHECK_ERRNO(pthread_detach(thread));
	}
}

