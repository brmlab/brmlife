#ifndef BRMLIFE__CONNECTION_H
#define BRMLIFE__CONNECTION_H

#include <unistd.h>

#include "map.h"

class connection {
public:
	int fd;

	connection(int fd_) : fd(fd_) {}

	~connection()
	{
		close(fd);
	}

	void senses(int tick_id, char around[4]);
};

#endif
