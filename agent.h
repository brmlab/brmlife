#ifndef BRMLIFE__AGENT_H
#define BRMLIFE__AGENT_H

#include "map.h"
#include "world.h"

class connection;

class agent {
public:
	int id;
	class connection *conn;

	class tile *tile;

	int energy;

	agent(int id_, class tile &tile_, class connection *conn_)
	: id (id_), conn (conn_), tile (&tile_)
	{
		put_at(tile_);
		energy = world::newborn_energy;
	};

	bool move_dir(int dir_x, int dir_y);

	void on_action_takes(void);
	void on_tick(void);
	void on_senses_update(void);

	~agent();

private:
	/* Just for initial placement. */
	void put_at(class tile &tile);
};

#endif
