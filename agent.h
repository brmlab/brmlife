#ifndef BRMLIFE__AGENT_H
#define BRMLIFE__AGENT_H

#include "map.h"

class agent {
public:
	int id;
	class tile *tile;

	agent(int id_, class tile &tile_) : id (id_), tile (&tile_)
	{
		put_at(tile_);
	};

	bool move_dir(int dir_x, int dir_y);

private:
	/* Just for initial placement. */
	void put_at(class tile &tile);
};

#endif
