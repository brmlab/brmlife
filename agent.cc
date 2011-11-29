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
	if (!t2->agent || t2->agent->dead)
		return false;

	class agent *a = t2->agent;
	chenergy(world::attack_cost);
	a->chenergy(world::defense_cost);
	if (dead || a->dead)
		return true;

	int dice = random() % ((int) round(attr.attack * energy) + (int) round(a->attr.defense * a->energy));
	if (dice < attr.attack * energy) {
		a->die();
	} else {
		die();
	}
	return true;
}

void
agent::chenergy(int delta)
{
	energy += delta;
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
		die();
		conn->cancel();
		conn = NULL;
		return;
	}

	conn->actions(*this);
}

void
agent::on_tick(void)
{
	if (!tile)
		return;

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
herb::on_tick(void)
{
	agent::on_tick();

	assert(tile);
	if (energy > 4 * world::herb_energy) {
		spawn_herb(tile->tile_in_dir(1, 0));
		spawn_herb(tile->tile_in_dir(0, 1));
		spawn_herb(tile->tile_in_dir(-1, 0));
		spawn_herb(tile->tile_in_dir(0, -1));
		tile->on_agent_leave(*this);
		tile = NULL;
	}
}
