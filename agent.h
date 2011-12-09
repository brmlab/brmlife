#ifndef BRMLIFE__AGENT_H
#define BRMLIFE__AGENT_H

#include <string>

#include "map.h"
#include "pheromone.h"
#include "world.h"

class connection;

/* Agent object lifetime is slightly complicated, since an agent
 * may be tied to a tile or a connection or both:
 *
 * - tile set, connection set:  active client or connected corpse
 * - tile set, connection NULL: herb or disconnected corpse
 * - tile NULL, connection set: negotiating client or zombie connection with no corpse anymore
 *
 * Agents are created (we may not keep this list up to date) on incoming
 * connection, by breeding, by blooming herb or when converting corpse
 * to herb. Herbs and bred clients are immediately spawned (assigned a tile);
 * connecting clients are spawned only after negotiation.
 *
 * Agents are destroyed in the main loop when they are completely abandoned,
 * i.e. both their tile and connection become NULL. */

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
		long breeding_key;
	} attr;

	int energy;
	bool newborn, dead;

	class pheromones pheromones;

	agent(int id_, class connection *conn_, class map &map_)
	: id (id_), conn (conn_), map (map_), tile (NULL), newborn (true)
	{
		attr.move = 1.0;
		attr.attack = 0.5;
		attr.defense = 0.5;
		attr.breeding_key = 0;
		energy = world::newborn_energy;
		dead = false;
	};
	void spawn(void);
	void spawn_at(class tile &tile);

	bool move_dir(int dir_x, int dir_y);
	bool attack_dir(int dir_x, int dir_y);
	bool breed_dir(int dir_x, int dir_y, std::string info);
	bool secrete(int id, double intensity);

	void chenergy(int delta);
	virtual void die(void);

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

	void die(void);
	void on_tick(void);
private:
	void smell_herb(class tile &);
};

#endif
