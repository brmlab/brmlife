#ifndef BRMLIFE__CONNECTION_H
#define BRMLIFE__CONNECTION_H

#include <cstdio>

#include "map.h"

class connection {
public:
	int fd;

	connection(int fd_) : fd(fd_) {}

	void senses(int tick_id, char around[4]);
};

#endif
