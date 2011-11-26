#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "agent.h"
#include "connection.h"

void
connection::senses(char around[4])
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "around %c%c%c%c\n\n", around[0], around[1], around[2], around[3]);
	write(fd, buf, strlen(buf));
}
