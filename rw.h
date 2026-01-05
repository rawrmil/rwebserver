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

typedef enum {
    RW_EV_HTTP_MSG,
    RW_EV_POLL,
} RW_Event;

typedef void (*RW_Handler)(RW_Event, void*);

typedef struct RW_Connection {
	int fd;
	uint16_t port;
	RW_Handler handler;
} RW_Connection;

typedef struct RW_HTTPMessage {
	RW_StringView message;
	RW_StringView method, uri, query, proto;
	RW_StringView head, body;
} RW_HTTPMessage;

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

bool rw_open_listener(RW_Connection* wc, uint16_t port, RW_Handler handler) {
	RW_LOG(RW_INFO, "port %lu", port);
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		RW_LOG(RW_ERROR, "socket error");
		return false;
	}

	rw_socket_nonblock(server_fd);

	wc->fd = server_fd;
	wc->port = port;
	wc->handler = handler;

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

bool rw_parse_http(RW_HTTPMessage* hmp, RW_StringView sv) {
	RW_HTTPMessage hm;
	hm.method = rw_trim_until(&sv, ' ');
	if (hm.method.buf == NULL) { return false; }

	hm.uri = rw_trim_until(&sv, ' ');
	if (hm.uri.buf == NULL) { return false; }

	hm.proto = rw_trim_until(&sv, '\r');
	if (hm.proto.buf == NULL) { return false; }

	if (sv.len < 1) { return false; }
	if (sv.buf[0] != '\n') { return false; }
	sv.len--;
	sv.buf++;

	*hmp = hm;
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
	RW_HTTPMessage hm;
	if (!rw_parse_http(&hm, rw_sv(buf, len))) {
		RW_LOG(RW_ERROR, "http handler failed");
		return;
	}
	if (wc->handler != NULL) { wc->handler(RW_EV_HTTP_MSG, &hm); }
	char resp[] =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello";
	write(client_fd, resp, strlen(resp));
}

#endif /* RW_IMPLEMENTATION */
