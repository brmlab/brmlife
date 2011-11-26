#ifndef BRMLIFE__AGENT_H
#define BRMLIFE__AGENT_H

#include "map.h"

class connection;

class agent {
public:
	int id;
	class tile *tile;
	class connection *conn;

	agent(int id_, class tile &tile_, class connection *conn_)
	: id (id_), tile (&tile_), conn (conn_)
	{
		put_at(tile_);
	};

	bool move_dir(int dir_x, int dir_y);

	void on_tick(void);
	void on_senses_update(void);

	~agent();

private:
	/* Just for initial placement. */
	void put_at(class tile &tile);
};

#endif
