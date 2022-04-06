/*
 Program uruchamiamy z dwoma parametrami: nazwa serwera i numer jego portu.
 Program spróbuje połączyć się z serwerem, po czym będzie od nas pobierał
 linie tekstu i wysyłał je do serwera.  Wpisanie "exit" kończy pracę.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "err.h"
#include "common.h"

#define BUFFER_SIZE 1024
#define NUMBER_SIZE 30

static const char exit_string[] = "exit";

void send_file(FILE *fp, int sockfd){
	char data[BUFFER_SIZE];

	memset(data, 0, sizeof(data));

	while(fgets(data, BUFFER_SIZE, fp) != NULL) {
		send_message(sockfd, data, BUFFER_SIZE, 0);
		memset(data, 0, sizeof(data));
	}
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fatal("Usage: %s <host> <port> <filename>\n", argv[0]);
    }

	char *host = argv[1];
	uint16_t port = read_port(argv[2]);
	struct sockaddr_in server_address = get_address(host, port);

	int socket_fd = open_socket();

	connect_socket(socket_fd, &server_address);

	char *server_ip = get_ip(&server_address);
	uint16_t server_port = get_port(&server_address);

    printf("Connected to %s:%d\n", server_ip, server_port);

	FILE *f = fopen(argv[3], "r");
	if (f == NULL) {
		fatal("Opening file %s failed", argv[3]);
	}
	fseek(f, 0, SEEK_END); // seek to end of file
	size_t file_size = ftell(f); // get current file pointer
	fseek(f, 0, SEEK_SET); // seek back to beginning of file

	int flags = 0;
	char number_str[NUMBER_SIZE];
	sprintf(number_str, "%lu", file_size);
	strcat(argv[3], " ");
	strcat(argv[3], number_str);

	size_t message_length = strnlen(argv[3], BUFFER_SIZE);
	printf("Message: %s\n", argv[3]);


	send_message(socket_fd, argv[3], message_length, flags);

	send_file(f, socket_fd);

	fclose(f);
	printf("Sent file to server %s:%d successfully.\n", server_ip, server_port);

    CHECK_ERRNO(close(socket_fd));

    return 0;
}
