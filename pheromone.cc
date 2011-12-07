#include <cassert>
#include <cstdlib>
#include <cmath>
#include <iostream>

#include "pheromone.h"
#include "main.h"


void
pheromones::secrete(class pheromone &p)
{
	for (std::list<class pheromone>::iterator pi = spectrum.begin(); pi != spectrum.end(); pi++) {
		if (pi->id < p.id)
			continue;
		if (pi->id == p.id) {
			pi->val += p.val;
			return;
		}
		spectrum.insert(pi, p);
		return;
	}
	spectrum.push_back(p);
}

void
pheromones::seep(class pheromones &to, double alpha, double beta)
{
	for (std::list<class pheromone>::iterator pi = spectrum.begin(); pi != spectrum.end(); pi++) {
		class pheromone p(pi->id, pi->val * (alpha * PH_COUNT / pi->id + beta));
		to.secrete(p);
	}
}

void
pheromones::decay(double gamma, double delta)
{
	for (std::list<class pheromone>::iterator pi = spectrum.begin(); pi != spectrum.end(); pi++) {
next_p:
		pi->val *= gamma;
		pi->val -= delta;
		if (pi->val < 0.001) {
			pi = spectrum.erase(pi);
			if (pi != spectrum.end())
				goto next_p;
		}
	}
}
