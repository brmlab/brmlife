#include <iostream>
#include <cassert>

#include "map.h"

void
tile::on_agent_enter(class agent *a)
{
	agent = a;
}

void
tile::on_agent_leave(class agent *a)
{
	assert(a == agent);
	agent = NULL;
}

void
tile::on_tick(void)
{
}

char
tile_ground::symbol(void)
{
	return '.';
}

class position *
position::next_in(int dir_x, int dir_y)
{
	int x2 = (map->w + x + dir_x) % map->w;
	int y2 = (map->h + y + dir_y) % map->h;
	return new position(x2, y2, map);
}


void
map::print_map(void)
{
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			class position p = position(x, y, this);
			std::cout << tile_at(p).symbol();
		}
		std::cout << '\n';
	}
}
