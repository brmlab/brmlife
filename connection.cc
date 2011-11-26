#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include "agent.h"
#include "connection.h"

void
connection::senses(int tick_id, char around[4])
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "tick %d\naround %c%c%c%c\n\n", tick_id, around[0], around[1], around[2], around[3]);
	if (write(fd, buf, strlen(buf)) < (ssize_t) strlen(buf))
		error = true;
}
