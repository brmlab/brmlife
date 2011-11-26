#include <iostream>

#include "agent.h"
#include "map.h"

int
main(int argc, char *argv[])
{
	class map map(10, 10);

	class position agentpos(4, 4, map);
	class agent agent(0, agentpos);

	map.print_map();
	return 0;
}
