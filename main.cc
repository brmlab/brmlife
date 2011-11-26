#include <iostream>

#include "agent.h"
#include "map.h"

int
main(int argc, char *argv[])
{
	class map map(10, 10);
	map.print_map();
	return 0;
}
