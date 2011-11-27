#ifndef BRMLIFE__WORLD_H
#define BRMLIFE__WORLD_H

struct world {
	const static int newborn_energy = 500;
	const static int move_cost = -10;
	const static int sun_energy = 1;

	const static int dead_body_energy = 500;
	const static int dead_decay = -1;
};

#endif
