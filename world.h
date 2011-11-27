#ifndef BRMLIFE__WORLD_H
#define BRMLIFE__WORLD_H

struct world {
	const static int newborn_energy = 500;
	const static int move_cost = -10;
	const static int attack_cost = -40;
	const static int defense_cost = -20;
	const static int sun_energy = 1;

	const static int dead_body_energy = 200;
	const static double dead_body_energy_carryover = 0.5;
	const static int dead_decay = -1;
};

#endif
