#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#define RW_IMPLEMENTATION
#include "rw.h"
#undef RW_IMPLEMENTATION

void fn(RW_Event ev, void* fn_data) {
	if (ev == RW_EV_HTTP_MSG) {
		RW_HTTPMessage* hm = (RW_HTTPMessage*)fn_data;
		RW_LOG(RW_INFO, "method: '"rw_sv_fmt"'", rw_sv_arg(hm->method));
		RW_LOG(RW_INFO, "uri: '"rw_sv_fmt"'", rw_sv_arg(hm->uri));
		RW_LOG(RW_INFO, "query: '"rw_sv_fmt"'", rw_sv_arg(hm->query));
		RW_LOG(RW_INFO, "proto: '"rw_sv_fmt"'", rw_sv_arg(hm->proto));
	}
}

int main(int argc, char** argv) {

	RW_Connection wc = {0};

	rw_open_listener(&wc, 6969, fn);

	while (1) {
		rw_listen(&wc);
	}

	close(wc.fd);
	return 0;
}
