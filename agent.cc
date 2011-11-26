#include <cassert>
#include <cstdlib>
#include <iostream>

#include "agent.h"
#include "map.h"

void
agent::put_at(class tile &t)
{
	if (!t.on_agent_enter(*this)) {
		std::cerr << "Collision.";
		exit(EXIT_FAILURE);
	}
}

bool
agent::move_dir(int dir_x, int dir_y)
{
	class tile *t2 = &tile->tile_in_dir(dir_x, dir_y);
	if (!t2->on_agent_enter(*this))
		return false;

	tile->on_agent_leave(*this);
	tile = t2;
	return true;
}
