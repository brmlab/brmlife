#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <iostream>
#include <list>

#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "agent.h"
#include "connection.h"
#include "main.h"
#include "map.h"

int tick_id = 0;

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

	signal(SIGPIPE, SIG_IGN);
	int flags = fcntl(lfd, F_GETFL, 0);
	fcntl(lfd, F_SETFL, flags | O_NONBLOCK);

	std::list<class agent *> agents;

	/* Main tick loop. */

	while (true) {
		std::cout << "tick " << tick_id << '\n';

		/* Drop disconnected agents. */

		for (std::list<class agent *>::iterator agent = agents.begin(); agent != agents.end(); agent++)
		{
next_agent:
			if ((*agent)->conn && (*agent)->conn->error) {
				delete *agent;
				agent = agents.erase(agent);
				if (agent != agents.end())
					goto next_agent;
			}
		}

		/* Accept new agents. */

		int cfd = accept(lfd, NULL, NULL);
		if (cfd >= 0) {
			class connection *conn = new class connection(cfd);
			class tile *agentpos;
			do {
				agentpos = &map.tile_at(random() % map.w, random() % map.h);
			} while (agentpos->agent);
			agents.push_back(new class agent(0, *agentpos, conn));
		}

		/* Run on_tick everywhere. */

		map.on_tick();
		for (std::list<class agent *>::iterator agent = agents.begin(); agent != agents.end(); agent++)
			(*agent)->on_tick();

		/* Finish a tick. */

		map.print_map();
		std::cout << '\n';
		usleep(1000000);
		tick_id++;
	}

	return 0;
}
