#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#include "agent.h"
#include "map.h"

bool
tile::on_agent_enter(class agent &a)
{
	if (agent) return false;
	agent = &a;
	return true;
}

void
tile::on_agent_leave(class agent &a)
{
	assert(&a == agent);
	agent = NULL;
}

void
tile::on_tick(void)
{
}

char
tile::symbol(void)
{
	if (agent) {
		if (agent->dead)
			return 'a';
		if (agent->attr.move < 0.01)
			return 'x';
		return '0' + (agent->id % 10);
	}
	return type_symbol();
}

char
tile_ground::type_symbol(void)
{
	return '.';
}

char *
tile::str(void)
{
	snprintf(str_buf, sizeof(str_buf),
		"%c%c",
		type_symbol(),
		agent ? (!agent->dead ? 'A' : 'a') : '-');
	return str_buf;
}

class tile &
tile::tile_in_dir(int dir_x, int dir_y)
{
	int x2 = (map.w + x + dir_x) % map.w;
	int y2 = (map.h + y + dir_y) % map.h;
	return map.tile_at(x2, y2);
}


class tile &
map::agent_startpos(void)
{
	/* Find a random starting tile that is not occupied yet. */
	class tile *tile;
	do {
		tile = &tile_at(random() % w, random() % h);
	} while (tile->agent);
	return *tile;
}

void
map::on_tick(void)
{
	for (int i = 0; i < w * h; i++)
		tiles[i]->on_tick();
}

void
map::print_map(void)
{
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			std::cout << tile_at(x, y).symbol();
		}
		std::cout << '\n';
	}
}
