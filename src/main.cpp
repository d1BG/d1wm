#include "server.h"

int main(int argc, char *argv[]) {
	wlr_log_init(WLR_DEBUG, nullptr);
	char *startup_cmd = nullptr;

	int c;
	while ((c = getopt(argc, argv, "s:h")) != -1) {
		switch (c) {
		case 's':
			startup_cmd = optarg;
			break;
		default:
			printf("Usage: %s [-s startup command]\n", argv[0]);
			return 0;
		}
	}
	if (optind < argc) {
		printf("Usage: %s [-s startup command]\n", argv[0]);
		return 0;
	}

	struct d1_server *server = new d1_server(startup_cmd);
	delete server;
	return 0;
}
