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
pthread_mutex_t dataMutex;
pthread_mutex_t splitMutex;


void initMutexes() {
	int err;
	if ((err = pthread_mutex_init(&dataMutex, 0)) != 0) {
		syserr(err, "dataMutex init failed");
	}
	if ((err = pthread_mutex_init(&splitMutex, 0)) != 0) {
		syserr(err, "splitMutex init failed");
	}
}

void destroyMutexes() {
	int err;
	if ((err = pthread_mutex_destroy(&dataMutex) != 0)) {
		syserr(err, "mutex destroy failed");
	}
	if ((err = pthread_mutex_destroy(&splitMutex) != 0)) {
		syserr(err, "mutex destroy failed");
	}
}

void changeTotalData(size_t file_size) {
	int err;
	if ((err = pthread_mutex_lock(&dataMutex)) != 0)
		syserr(err, "lock failed");
	sumOfData += file_size;
	if ((err = pthread_mutex_unlock(&dataMutex)) != 0)
		syserr(err, "unlock failed");
}


void *handle_connection(void *client_fd_ptr) {
	int err;
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

	if ((err = pthread_mutex_lock(&splitMutex)) != 0)
		syserr(err, "lock failed");
	char *token = strtok(line, " ");
	char *filename = token;
	token = strtok(NULL, " ");
	char *file_size_str = token;
	if ((err = pthread_mutex_unlock(&splitMutex)) != 0)
		syserr(err, "unlock failed");

	char *startOfFile;
	size_t file_size = strtoul(file_size_str, &startOfFile, 10);

	printf("new client [%s:%d] size=[%lu bytes] file=[%s]\n", ip, port,
	       file_size, filename);

	FILE *fp = fopen(filename, "w+");
	fputs(startOfFile, fp);

	char newLine[LINE_SIZE];
	if (fp == NULL) {
		fatal("Could not open file");
	}
	while (1) {
		memset(newLine, 0, LINE_SIZE);
		read = receive_message(client_fd, &newLine, LINE_SIZE, 0);
		if (read < 0)
			PRINT_ERRNO();
		if (read == 0)
			break;

		for (size_t i = 0; i < read; i++) {
			if (newLine[i] != '\0')
				putc(newLine[i], fp);
		}
	}

	fclose(fp);
	printf("File received successfully\n");

	changeTotalData(file_size);
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

	initMutexes();

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

//	if ((err = pthread_mutex_destroy(&dataMutex) != 0)) {
//		syserr (err, "cond destroy 1 failed");
//	}
}

