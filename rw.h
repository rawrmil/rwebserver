#ifndef RW_H
#define RW_H

#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define RW_BACKLOG 10

/* Logging System */

typedef enum {
    RW_INFO,
    RW_WARNING,
    RW_ERROR,
    RW_NONE,
} RW_LogLevel;

extern RW_LogLevel rw_log_level;

#define RW_UNREACHABLE(message) do { fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); abort(); } while(0)
#define RW_LOG(level_, ...) \
	do { \
		if ((level_) < rw_log_level) { break; } \
		if ((level_) == RW_NONE) { break; } \
		switch ((level_)) { \
			case RW_INFO: fprintf(stdout, "[INFO] "); break; \
			case RW_WARNING: fprintf(stdout, "[WARN] "); break; \
			case RW_ERROR: fprintf(stdout, "[ERR] "); break; \
			default: RW_UNREACHABLE("nob_log"); \
		} \
		fprintf(stdout, "%s:%d:%s: ", __FILE__, __LINE__, __func__); \
		fprintf(stdout, __VA_ARGS__); \
		fprintf(stdout, "\n"); \
	} while(0);

/* String View */

typedef struct RW_StringView {
	uint8_t* buf;
	size_t len;
} RW_StringView;

#define rw_sv(buf_, len_) (RW_StringView){ .buf=(buf_), .len=(len_) }
#define rw_sv_fmt "%.*s"
#define rw_sv_arg(sv) (int)(sv).len, (sv).buf

/* Connection */

typedef struct RW_Connection {
	int fd;
	uint16_t port;
} RW_Connection;

#endif /* RW_H */

#ifdef RW_IMPLEMENTATION

/* Logging System */

RW_LogLevel rw_log_level;

bool rw_socket_nonblock(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) { return false; }
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		return false;
	}
	return true;
}

bool rw_open_listener(RW_Connection* wc, uint16_t port) {
	RW_LOG(RW_INFO, "port %lu", port);
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		RW_LOG(RW_ERROR, "socket error");
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
		RW_LOG(RW_ERROR, "binding error");
		return false;
	}

	if (listen(server_fd, RW_BACKLOG) < 0) {
		RW_LOG(RW_ERROR, "listen error");
		return false;
	}

	return true;
}

RW_StringView rw_trim_until(RW_StringView* src, uint8_t c) {
	RW_StringView old = *src;
	while (src->len > 0) {
		src->buf++;
		src->len--;
		if (src->buf[-1] == c) {
			return rw_sv(old.buf, old.len - src->len - 1);
		}
	}
	return rw_sv(NULL, 0);
}

void rw_parse_http(RW_StringView sv) {
	RW_StringView method = rw_trim_until(&sv, ' ');
	if (method.buf == NULL) { return; }

	RW_StringView uri = rw_trim_until(&sv, ' ');
	if (uri.buf == NULL) { return; }

	RW_StringView version = rw_trim_until(&sv, '\r');
	if (version.buf == NULL) { return; }

	if (sv.len < 1) { return; }
	if (sv.buf[0] != '\n') { return; }
	sv.len--;
	sv.buf++;

	RW_LOG(RW_INFO, "method: '"rw_sv_fmt"'", rw_sv_arg(method));
	RW_LOG(RW_INFO, "uri: '"rw_sv_fmt"'", rw_sv_arg(uri));
	RW_LOG(RW_INFO, "version: '"rw_sv_fmt"'", rw_sv_arg(version));
}

void rw_listen(RW_Connection* wc) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(wc->fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}
		RW_LOG(RW_ERROR, "accept error");
		return;
	}
	RW_LOG(RW_INFO, "connected %s", inet_ntoa(client_addr.sin_addr));
	char buf[1024] = {0};
	ssize_t len = read(client_fd, buf, sizeof(buf) - 1);
	if (len < 0) {
		RW_LOG(RW_ERROR, "read error");
		return;
	}
	rw_parse_http(rw_sv(buf, len));
	char resp[] =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello";
	write(client_fd, resp, strlen(resp));
}

#endif /* RW_IMPLEMENTATION */
