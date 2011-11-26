#include <cassert>
#include <cstdlib>
#include <iostream>

#include "agent.h"
#include "map.h"

void
agent::put_at(struct position *pos)
{
	class tile *t = &pos->map->tile_at(*pos);
	if (!t->on_agent_enter(this)) {
		std::cerr << "Collision.";
		exit(EXIT_FAILURE);
	}
}
