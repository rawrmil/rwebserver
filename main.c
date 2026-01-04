#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_ntoa

#include <string.h>

#include <stdio.h>

int main(int argc, char** argv) {

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("Socket error.");
		return 1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(6969);

	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Binding error.");
		return 1;
	}

	if (listen(server_fd, 10) < 0) {
		perror("Listen error.");
		return 1;
	}

	printf("hello: %d\n", 6969);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
		if (client_fd < 0) {
			perror("Accept error.");
			return 1;
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

	close(server_fd);
	return 0;
}
