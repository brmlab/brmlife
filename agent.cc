#include <cassert>
#include <cstdlib>
#include <iostream>

#include "agent.h"
#include "connection.h"
#include "main.h"
#include "map.h"

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
		if (t2->agent->dead) {
			class agent *a = t2->agent;
			t2->on_agent_leave(*a);
			/* Nom. */
			chenergy(a->energy);
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

	int dice = rand() % (energy + a->energy);
	if (dice < energy) {
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

	conn->actions(*this);
}

void
agent::on_tick(void)
{
	if (!tile)
		return;

	if (!dead) {
		chenergy(world::sun_energy);

	} else {
		energy += world::dead_decay;
		if (energy < 0)
			energy = 0;
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
	if (tile)
		tile->on_agent_leave(*this);
	if (conn) {
		conn->cancel();
		conn = NULL;
	}
};
