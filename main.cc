#include <iostream>

#include "agent.h"
#include "map.h"

int
main(int argc, char *argv[])
{
	class map map(10, 10);

	class tile &agentpos = map.tile_at(4, 4);
	class agent agent(0, agentpos);

	map.print_map();
	std::cout << '\n';

	agent.move_dir(1, 0);
	map.print_map();
	std::cout << '\n';

	agent.move_dir(0, -1);
	map.print_map();
	std::cout << '\n';

	return 0;
}
