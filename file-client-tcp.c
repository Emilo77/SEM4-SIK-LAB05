/*
Program uruchamiamy z dwoma parametrami: nazwa serwera i numer jego portu.
Program spróbuje połączyć się z serwerem, po czym będzie od nas pobierał
linie tekstu i wysyłał je do serwera.  Wpisanie "exit" kończy pracę.
*/

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "err.h"

#define BUFFER_SIZE 32768

void send_file(FILE *fp, int sockfd) {
  char data[BUFFER_SIZE];
  memset(data, 0, sizeof(data));

  while (fgets(data, BUFFER_SIZE, fp) != NULL) {
    send_message(sockfd, data, strlen(data), 0);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fatal(1, "Usage: %s <host> <port> <filename>\n", argv[0]);
  }

  char *host = argv[1];
  uint16_t port = read_port(argv[2]);
  struct sockaddr_in server_address = get_address(host, port);

  int socket_fd = open_socket();

  connect_socket(socket_fd, &server_address);

  char *server_ip = get_ip(&server_address);
  uint16_t server_port = get_port(&server_address);

  printf("Connected to %s:%d\n", server_ip, server_port);

  size_t file_size = check_file_size(argv[3]);

  FILE *f = fopen(argv[3], "r");
  struct fileInfo fi = {.file_size = file_size};

  memset(fi.filename, 0, sizeof(fi.filename));
  strcpy(fi.filename, argv[3]);
  send_message(socket_fd, &fi, sizeof(struct fileInfo), NO_FLAGS);

  send_file(f, socket_fd);

  fclose(f);
  printf("Sent file to server %s:%d successfully.\n", server_ip, server_port);

  CHECK_ERRNO(close(socket_fd));

  return 0;
}