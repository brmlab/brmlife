#ifndef BRMLIFE__AGENT_H
#define BRMLIFE__AGENT_H

#include "map.h"
#include "pheromone.h"
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

	class pheromones pheromones;

	agent(int id_, class connection *conn_, class map &map_)
	: id (id_), conn (conn_), map (map_), tile (NULL)
	{
		attr.move = 1.0;
		attr.attack = 0.5;
		attr.defense = 0.5;
		energy = world::newborn_energy;
		dead = false;
	};
	void spawn(void);
	void spawn_at(class tile &tile);

	bool move_dir(int dir_x, int dir_y);
	bool attack_dir(int dir_x, int dir_y);
	bool secrete(int id, double intensity);

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
		attr.move = 0;
		attr.attack = 0;
		attr.defense = 0;
		energy = world::herb_energy;
	}

	void on_tick(void);
private:
	void smell_herb(class tile &);
};

#endif
