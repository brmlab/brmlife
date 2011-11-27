#ifndef BRMLIFE__AGENT_H
#define BRMLIFE__AGENT_H

#include "map.h"
#include "world.h"

class connection;

class agent {
public:
	int id;
	class connection *conn;

	class map &map;
	class tile *tile;

	struct {
		double move;
		double attack;
		double defense;
	} attr;

	int energy;
	bool dead;

	agent(int id_, class connection *conn_, class map &map_)
	: id (id_), conn (conn_), map (map_)
	{
		energy = world::newborn_energy;
		dead = false;
		attr.move = 1.0;
		attr.attack = 0.5;
		attr.defense = 0.5;
	};
	void spawn(void);
	void spawn_at(class tile &tile);

	bool move_dir(int dir_x, int dir_y);
	bool attack_dir(int dir_x, int dir_y);

	void chenergy(int delta);
	void die(void);

	void on_action_takes(void);
	virtual void on_tick(void);
	void on_senses_update(void);

	~agent();
};

class herb : public agent {
public:
	herb(int id_, class map &map_)
	: agent(id_, NULL, map_)
	{
		energy = world::herb_energy;
		attr.move = 0;
		attr.attack = 0;
		attr.defense = 0;
	}

	void on_tick(void);
};

#endif
