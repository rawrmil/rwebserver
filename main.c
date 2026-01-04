#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#define RW_IMPLEMENTATION
#include "rw.h"
#undef RW_IMPLEMENTATION

int main(int argc, char** argv) {

	RW_Connection wc = {0};

	rw_open_listener(&wc, 6969);

	while (1) {
		rw_listen(&wc);
	}

	close(wc.fd);
	return 0;
}
