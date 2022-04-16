#include "common.h"
#include "err.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <fcntl.h>

#define BUFFER_SIZE 1024
#define QUEUE_LENGTH 5

size_t sumOfData = 0;
pthread_mutex_t dataMutex;
char buffer[BUFFER_SIZE];

void initMutex() {
  int err;
  if ((err = pthread_mutex_init(&dataMutex, 0)) != 0) {
    fatal(err, "dataMutex init failed");
  }
}

void changeTotalData(size_t file_size) {
  int err;
  if ((err = pthread_mutex_lock(&dataMutex)) != 0)
    fatal(err, "lock failed");
  sumOfData += file_size;
  printf("Total data achieved: %lu\n", sumOfData);
  if ((err = pthread_mutex_unlock(&dataMutex)) != 0)
    fatal(err, "unlock failed");
}

void readFile(struct fileInfo info, int client_fd) {
  int fp = open(info.filename, O_CREAT | O_TRUNC | O_RDWR, 06666);
  size_t read_size = 0;
  while (read_size < info.file_size) {
    size_t read;
    read = receive_message(client_fd, &buffer, BUFFER_SIZE, 0);
    if (read < 0)
      PRINT_ERRNO();
    if (read == 0)
      break;
    read_size += read;
    write(fp, buffer, read);
  }
}

void *handle_connection(void *client_fd_ptr) {
  int client_fd = *(int *)client_fd_ptr;
  free(client_fd_ptr);

  char *ip = get_ip_from_socket(client_fd);
  int port = get_port_from_socket(client_fd);

  struct fileInfo file_info;
  size_t read =
      receive_message(client_fd, &file_info, sizeof(struct fileInfo), NO_FLAGS);
  if (read <= 0) {
    PRINT_ERRNO();
  }

  printf("--------------------------------------------------------------\n");
  printf("Received information about file: %s\n", file_info.filename);
  printf("Client [%s:%d] has sent its file of size=[%zu]\n", ip, port,
         file_info.file_size);

  readFile(file_info, client_fd);

  size_t size_achieved = check_file_size(file_info.filename);
  if (size_achieved != file_info.file_size) {
    printf("File size mismatch: expected %zu, got %zu\n", file_info.file_size,
           size_achieved);
  } else {
    printf("File received successfully\n");
    changeTotalData(size_achieved);
  }
  printf("[thread %lu, pid %d] connection closed\n",
         (unsigned long)pthread_self(), getpid());

  CHECK_ERRNO(close(client_fd));
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fatal(1, "Usage: %s <port>", argv[0]);
  }

  initMutex();

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
      fatal(1, "malloc");
    }
    *client_fd_pointer = client_fd;

    pthread_t thread;
    CHECK_ERRNO(
        pthread_create(&thread, 0, handle_connection, client_fd_pointer));
    CHECK_ERRNO(pthread_detach(thread));
  }

  //	if ((err = pthread_mutex_destroy(&dataMutex) != 0)) {
  //		fatal (err, "cond destroy 1 failed");
  //	}
}
