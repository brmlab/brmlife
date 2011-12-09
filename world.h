#ifndef BRMLIFE__WORLD_H
#define BRMLIFE__WORLD_H

struct world {
	const static int newborn_energy = 5000;
	const static int herb_energy = 1000;
	const static int max_energy = 20000;

	const static int move_cost = -50;
	const static int attack_cost = -400;
	const static int defense_cost = -200;
	const static int breed_out_cost = -newborn_energy/4;
	const static int breed_in_cost = -newborn_energy*3/4;
	const static int pheromone_cost = -10;

	const static int move_idle_cost = -15; /* ... * attr.move */
	const static int attack_idle_cost = -15; /* ... * attr.attack */
	const static int defense_idle_cost = -15; /* ... * attr.defense */
	const static int sun_energy = 10;
	const static int soil_energy = 20; /* ... times five for lone herbs, times one for dense forests */

	const static int dead_body_energy = 2000;
	const static double dead_body_energy_carryover = 0.5;
	const static int dead_decay = -50;

	const static int herb_rate = 15; /* initially, one herb per herb_rate tiles */
	const static double herb_phintensity = 1.0;

	const static long breeding_kappa = 10000;

	const static double phseep_alpha = 0.1;
	const static double phseep_beta = 0.05;
	const static double phdecay_gamma = 0.95;
	const static double phdecay_delta = 0.01;
};

#endif
