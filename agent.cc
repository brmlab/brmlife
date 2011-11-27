#include <cassert>
#include <cstdlib>
#include <iostream>

#include "agent.h"
#include "connection.h"
#include "main.h"
#include "map.h"

void
agent::put_at(class tile &t)
{
	if (!t.on_agent_enter(*this)) {
		std::cerr << "Collision.";
		exit(EXIT_FAILURE);
	}
}

bool
agent::move_dir(int dir_x, int dir_y)
{
	if (dead)
		return false;

	energy -= world::move_cost;

	class tile *t2 = &tile->tile_in_dir(dir_x, dir_y);
	if (t2->agent) {
		if (t2->agent->dead) {
			class agent *a = t2->agent;
			t2->on_agent_leave(*a);
			/* Nom. */
			energy += a->energy;
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

void
agent::die(void)
{
	assert(!dead);
	dead = true;
	energy = world::dead_body_energy;
}

void
agent::on_action_takes(void)
{
	if (!conn)
		return;

	conn->actions(this);
}

void
agent::on_tick(void)
{
	if (!dead) {
		energy += world::sun_energy;
		if (energy <= 0)
			die();

	} else {
		energy += world::dead_decay;
		if (energy < 0)
			energy = 0;
	}
}

void
agent::on_senses_update(void)
{
	if (!conn)
		return;

	char around[8] = {
		tile->tile_in_dir(0, -1).symbol(),
		tile->tile_in_dir(1, -1).symbol(),
		tile->tile_in_dir(1, 0).symbol(),
		tile->tile_in_dir(1, 1).symbol(),
		tile->tile_in_dir(0, 1).symbol(),
		tile->tile_in_dir(-1, 1).symbol(),
		tile->tile_in_dir(-1, 0).symbol(),
		tile->tile_in_dir(-1, -1).symbol(),
	};
	conn->senses(tick_id, dead, energy, around);
}

agent::~agent()
{
	tile->on_agent_leave(*this);
	if (conn) {
		conn->cancel();
		conn = NULL;
	}
};
