#ifndef BRMLIFE__PHEROMONE_H
#define BRMLIFE__PHEROMONE_H

/* Pheromone spectrum in a particular state. */

#include <list>

#define PH_COUNT 65536

class pheromone {
public:
	int id;
	double val;

	pheromone(int id_ = 0, double val_ = 0)
	: id (id_), val (val_) {};
};

class pheromones {
public:
	std::list<class pheromone> spectrum;

	/* Add a pheromone to the spectrum. */
	void secrete(class pheromone &p);
	/* Merge slight trail of spectrum to another spectrum (transfer
	 * by touching). Pheromones with lower index are transmitted
	 * better than those with higher index:
	 * v' = v * (alpha / i + beta) */
	void seep(class pheromones &to, double alpha, double beta);
	/* Decay the spectrum, multiplying each value by gamma and
	 * then reducing it by delta. */
	void decay(double gamma, double delta);
};

#endif
