#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cstring>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "agent.h"
#include "connection.h"
#include "map.h"

int
main(int argc, char *argv[])
{
	class map map(40, 20);

	srandom(time(NULL));

	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(27753);
	sin.sin_addr.s_addr = INADDR_ANY;
	int optval = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bind(lfd, (struct sockaddr *) &sin, sizeof(sin));
	listen(lfd, 10);

	int cfd;
	while ((cfd = accept(lfd, NULL, NULL)) >= 0) {
		class connection conn(cfd);
		class tile &agentpos = map.tile_at(random() % map.w, random() % map.h);
		class agent agent(0, agentpos, conn);

		while (true) {
			map.print_map();
			std::cout << '\n';

			char around[4] = {
				agent.tile->tile_in_dir(0, -1).symbol(),
				agent.tile->tile_in_dir(1, 0).symbol(),
				agent.tile->tile_in_dir(0, 1).symbol(),
				agent.tile->tile_in_dir(-1, 0).symbol(),
			};
			conn.senses(around);

			usleep(1000000);
		}

		/* TODO: destroy agent cleanly */
	}

	return 0;
}
