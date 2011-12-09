#include <cassert>
#include <cstdlib>
#include <cmath>
#include <iostream>

#include "agent.h"
#include "connection.h"
#include "main.h"
#include "map.h"

static void spawn_herb(class tile &t);

void
agent::spawn(void)
{
	spawn_at(map.agent_startpos());
}

void
agent::spawn_at(class tile &t)
{
	tile = &t;
	if (!t.on_agent_enter(*this)) {
		std::cerr << "Collision.";
		exit(EXIT_FAILURE);
	}
}

bool
agent::move_dir(int dir_x, int dir_y)
{
	if (dead || !tile)
		return false;

	if ((double) random() / (double) RAND_MAX > attr.move)
		return false;

	chenergy(world::move_cost);

	class tile *t2 = &tile->tile_in_dir(dir_x, dir_y);
	if (t2->agent) {
		if (t2->herb_here()) {
			class agent *a = t2->agent;
			t2->on_agent_leave(*a);
			a->tile = NULL;
			chenergy(a->energy); /* Nom. */

		} else if (t2->agent->dead) {
			class agent *a = t2->agent;
			t2->on_agent_leave(*a);
			a->tile = NULL; // XXX: If one agent kills another while third is trying to move at that place, the killed agent never receives a DEATH.
			chenergy(a->energy); /* Nom. */
			a->energy = 0;

		} else {
			return false;
		}
	}

	if (!t2->on_agent_enter(*this))
		return false;

	tile->on_agent_leave(*this);
	tile = t2;
	return true;
}

bool
agent::attack_dir(int dir_x, int dir_y)
{
	if (dead || !tile)
		return false;

	class tile *t2 = &tile->tile_in_dir(dir_x, dir_y);
	if (!t2->agent || t2->agent->dead || t2->herb_here())
		return false;

	class agent *a = t2->agent;
	chenergy(world::attack_cost);
	a->chenergy(world::defense_cost);
	if (dead || a->dead)
		return true;

	/* Very simple RPG-ish interaction. */
	int attack_dice = round(attr.attack * energy);
	int attack_roll = random() % attack_dice;
	int defense_dice = round(a->attr.defense * a->energy);
	int defense_roll = random() % defense_dice;
	if (attack_roll > defense_roll)
		a->chenergy(defense_roll - attack_roll);
	return attack_roll >= defense_roll;
}

bool
agent::breed_dir(int dir_x, int dir_y, std::string info)
{
	if (dead || !tile)
		return false;

	class tile *t2 = &tile->tile_in_dir(dir_x, dir_y);
	class agent *a = t2->agent;
	if (!a || a->dead || !a->conn)
		return false;

	/* Self-breeding may not be a bad thing, but there is just
	 * a technical problem with in/out buf locking. */
	assert(a != this);

	if (abs(a->attr.breeding_key - attr.breeding_key) > world::breeding_kappa)
		return false;

	chenergy(world::breed_out_cost);
	a->chenergy(world::breed_in_cost);
	if (a->dead)
		return false;

	/* Grab a tile. */
	int spawn_dirs[][2] = {
		{0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1},
	};
	int spawn_dir_n = sizeof(spawn_dirs) / sizeof(spawn_dirs[0]);
	class tile *tb = NULL;
	for (int i = 0; i < spawn_dir_n; i++) {
		class tile *t = &tile->tile_in_dir(spawn_dirs[i][0], spawn_dirs[i][1]);
		if (!t->agent) {
			tb = t;
			break;
		}
	}
	if (!tb)
		return false; // still-born

	/* New agent, yay. */
	class agent *ab = new class agent(agent_id++, NULL, map);
	agents.push_back(ab);
	ab->spawn_at(*tb);
	a->conn->bred(ab->id, info);
	return true;
}

bool
agent::secrete(int id, double intensity)
{
	pheromone p(id, intensity);
	pheromones.secrete(p);
	chenergy(intensity * world::pheromone_cost);
	return true;
}

void
agent::chenergy(int delta)
{
	energy += delta;
	if (energy > world::max_energy)
		energy = world::max_energy;
	if (energy <= 0)
		die();
}

void
agent::die(void)
{
	assert(!dead);
	dead = true;
	if (energy < 0) energy = 0;
	energy = energy * world::dead_body_energy_carryover + world::dead_body_energy;
}

void
agent::on_action_takes(void)
{
	if (!conn)
		return;

	if (conn->error) {
		conn->cancel();
		conn = NULL;
		return;
	}

	conn->actions(this);
}

void
agent::on_tick(void)
{
	pheromones.decay(world::phdecay_gamma, world::phdecay_delta);

	if (!tile)
		return;

	pheromones.seep(tile->pheromones, world::phseep_alpha, world::phseep_beta);

	if (!dead) {
		chenergy(world::sun_energy);
		chenergy(attr.move * world::move_idle_cost);
		chenergy(attr.attack * world::attack_idle_cost);
		chenergy(attr.defense * world::defense_idle_cost);

	} else {
		energy += world::dead_decay;
		if (energy < 0) {
			tile->on_agent_leave(*this);
			spawn_herb(*tile);
			tile = NULL;
		}
	}
}

void
agent::on_senses_update(void)
{
	if (!conn || !tile)
		return;

	conn->senses(tick_id, *this);
}

agent::~agent()
{
	if (tile && tile->agent == this)
		tile->on_agent_leave(*this);
	if (conn) {
		conn->cancel();
		conn = NULL;
	}
}


void
herb::die(void)
{
	/* No corpse, just clean up tile. */
	tile->on_agent_leave(*this);
	tile = NULL;
}

static void
spawn_herb(class tile &t)
{
	if (t.agent)
		return;
	class herb *h = new class herb(agent_id++, t.map);
	agents.push_back(h);
	h->spawn_at(t);
}

void
herb::smell_herb(class tile &t)
{
	/* Herb pheromone ("smell") #32768. */
	class pheromone p(32768, world::herb_phintensity);
	t.pheromones.secrete(p);
	chenergy(p.val * world::pheromone_cost);
}

void
herb::on_tick(void)
{
	agent::on_tick();

	assert(tile);
	if (energy > 4 * world::herb_energy) {
		spawn_herb(tile->tile_in_dir(1, 0));
		spawn_herb(tile->tile_in_dir(0, 1));
		spawn_herb(tile->tile_in_dir(-1, 0));
		spawn_herb(tile->tile_in_dir(0, -1));
		die();
	} else {
		smell_herb(tile->tile_in_dir(1, 0));
		smell_herb(tile->tile_in_dir(0, 1));
		smell_herb(tile->tile_in_dir(-1, 0));
		smell_herb(tile->tile_in_dir(0, -1));
	}
}
