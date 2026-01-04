#ifndef RW_H
#define RW_H

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define RW_BACKLOG 10

typedef struct RW_Connection {
	int fd;
	uint16_t port;
} RW_Connection;

#endif /* RW_H */

#ifdef RW_IMPLEMENTATION

bool rw_socket_nonblock(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) { return false; }
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		return false;
	}
	return true;
}

bool rw_open_listener(RW_Connection* wc, uint16_t port) {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("Socket error.");
		return false;
	}

	rw_socket_nonblock(server_fd);

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
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}
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

#endif /* RW_IMPLEMENTATION */
