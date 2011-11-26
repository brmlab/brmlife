#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "agent.h"
#include "connection.h"

void
connection::senses(int tick_id, char around[4])
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "tick %d\naround %c%c%c%c\n\n", tick_id, around[0], around[1], around[2], around[3]);
	write(fd, buf, strlen(buf));
}
