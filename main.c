#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_ntoa

#include <string.h>

#include <stdio.h>
#include <stdbool.h>

#define RW_BACKLOG 10

typedef struct RW_Connection {
	int fd;
	uint16_t port;
} RW_Connection;

bool rw_open_listener(RW_Connection* wc, uint16_t port) {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("Socket error.");
		return false;
	}

	wc->fd = server_fd;
	wc->port = port;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Binding error.");
		return false;
	}

	if (listen(server_fd, RW_BACKLOG) < 0) {
		perror("Listen error.");
		return false;
	}

	return true;
}

void rw_listen(RW_Connection* wc) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(wc->fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		perror("Accept error.");
		return;
	}
	printf("connected:%s\n", inet_ntoa(client_addr.sin_addr));
	char buf[1024] = {0};
	read(client_fd, buf, sizeof(buf) - 1);
	printf("req:%s\n", buf);
	char resp[] =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello";
	write(client_fd, resp, strlen(resp));
}

int main(int argc, char** argv) {

	RW_Connection wc = {0};

	rw_open_listener(&wc, 6969);

	while (1) {
		rw_listen(&wc);
	}

	close(wc.fd);
	return 0;
}
