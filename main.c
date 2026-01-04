#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in

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

	while (1);

	close(server_fd);
	return 0;
}
