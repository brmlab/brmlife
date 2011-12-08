#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>

#include "agent.h"
#include "connection.h"
#include "main.h"

#define buf_incomplete(buf) \
	(buf.find("\r\n") == std::string::npos || (buf.find("\r\n") > 0 && buf.find("\r\n\r\n") == std::string::npos))

void
connection::senses(int tick_id, class agent &a)
{
	assert(!negotiation);

	int dirs[][2] = {
		{0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1},
		{0,-2}, {1,-2}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {1,2}, {0,2}, {-1,2}, {-2,2}, {-2,1}, {-2,0}, {-2,-1}, {-2,-2}, {-1,-2},
	};
	int dir_n = sizeof(dirs) / sizeof(dirs[0]);

	char buf[1024];
	char *bufp = buf;
	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "tick %d\r\n", tick_id);
	if (a.dead)
		bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "DEAD\r\n");
	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "energy %d\r\n", a.energy);

	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "visual");
	for (int i = 0; i < dir_n; i++) {
		bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), " %s", a.tile->tile_in_dir(dirs[i][0], dirs[i][1]).str());
	}
	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "\r\n");

	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "pheromones");
	for (int i = 0; i < dir_n; i++) {
		bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), " (");
		class pheromones &ps = a.tile->tile_in_dir(dirs[i][0], dirs[i][1]).pheromones;
		class agent *ai = a.tile->tile_in_dir(dirs[i][0], dirs[i][1]).agent;
		if (ai) {
			/* We need to merge pheromones. */
			class pheromones &pt = ai->pheromones;
			std::list<class pheromone>::iterator pi, pj;
			for (pi = ps.spectrum.begin(), pj = pt.spectrum.begin();
			     pi != ps.spectrum.end() || pj != pt.spectrum.end(); ) {
				if (pi != ps.spectrum.begin() || pj != pt.spectrum.begin())
					bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), ",");
				int i; double v;
				if (pj == pt.spectrum.end() || (pi != ps.spectrum.end() && pi->id < pj->id)) {
					i = pi->id; v = pi->val; ++pi;
				} else if (pi == ps.spectrum.end() || pj->id < pi->id) {
					i = pj->id; v = pj->val; ++pj;
				} else {
					i = pi->id; v = pi->val + pj->val; ++pi, ++pj;
				}
				bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "%d:%f", i, v);
			}
		} else {
			for (std::list<class pheromone>::iterator pi = ps.spectrum.begin(); pi != ps.spectrum.end(); pi++) {
				if (pi != ps.spectrum.begin())
					bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), ",");
				bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "%d:%f", pi->id, pi->val);
			}
		}
		bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), ")");
	}
	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "\r\n");

	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), "\r\n");
	pthread_mutex_lock(&buf_lock);
	out_buf.append(buf);
	pthread_mutex_unlock(&buf_lock);
}

void
connection::bred(int agent_id, std::string &info)
{
	pthread_mutex_lock(&buf_lock);
	std::stringstream s;
	s << "BRED " << agent_id << " " << info << "\r\n";
	out_buf.append(s.str());
	pthread_mutex_unlock(&buf_lock);
}

void
connection::bump(void)
{
	/* Must be called with buf_lock held! */
	out_buf.append("BUMP\r\n");
}

void
connection::actions(class agent *agent)
{
	pthread_mutex_lock(&buf_lock);
	if (buf_incomplete(in_buf)) {
		/* Not enough data, needs to wait until next turn, sorry. */
		pthread_mutex_unlock(&buf_lock);
		return;
	}

	int mask = 0;
	while (in_buf.c_str()[0] != '\r') {
		int nlofs = in_buf.find("\r\n");
		std::string line = in_buf.substr(0, nlofs);
		in_buf.erase(0, nlofs + 2);

		int spofs = line.find(' ');
		std::string cmd = line.substr(0, spofs);
		line.erase(0, spofs + 1);

		if (negotiation && !cmd.compare("move")) {
			double rate = agent->attr.move;
			sscanf(line.c_str(), "%lf", &rate);
			if (rate >= 0 && rate <= 1)
				agent->attr.move = rate;
		} else if (negotiation && !cmd.compare("attack")) {
			double rate = agent->attr.attack;
			sscanf(line.c_str(), "%lf", &rate);
			if (rate >= 0 && rate <= 1)
				agent->attr.attack = rate;
		} else if (negotiation && !cmd.compare("defense")) {
			double rate = agent->attr.defense;
			sscanf(line.c_str(), "%lf", &rate);
			if (rate >= 0 && rate <= 1)
				agent->attr.defense = rate;
		} else if (negotiation && !cmd.compare("breeding_key")) {
			sscanf(line.c_str(), "%ld", &agent->attr.breeding_key);

		} else if (negotiation && !cmd.compare("agent_id")) {
			int id = -1;
			sscanf(line.c_str(), "%d", &id);
			if (id < 0) {
bump_negot:
				bump(); out_buf.append("\r\n");
			} else {
				class agent *a2 = NULL;
				for (std::list<class agent *>::iterator ai = agents.begin(); ai != agents.end(); ai++) {
					if ((*ai)->id == id) {
						a2 = *ai;
						break;
					}
				}
				if (!a2 || a2->conn || !a2->tile || (dynamic_cast<herb *> (a2)))
					goto bump_negot;
				/* Round and round she goes, where she stops, nobody knows. */
				a2->conn = this;
				agent->conn = NULL;
				agent = a2;
				negotiation = agent->newborn;
			}

		} else if (!negotiation && !cmd.compare("move_dir") && !(mask & 1)) {
			int x = 0, y = 0;
			sscanf(line.c_str(), "%d %d", &x, &y);
			if (x < -1) x = -1; if (x > 1) x = 1;
			if (y < -1) y = -1; if (y > 1) y = 1;
			if (!agent->move_dir(x, y))
				bump();
			mask |= 1;
		} else if (!negotiation && !cmd.compare("attack_dir") && !(mask & 2)) {
			int x = 0, y = 0;
			sscanf(line.c_str(), "%d %d", &x, &y);
			if (x < -1) x = -1; if (x > 1) x = 1;
			if (y < -1) y = -1; if (y > 1) y = 1;
			if (!agent->attack_dir(x, y))
				bump();
			mask |= 2;
		} else if (!negotiation && !cmd.compare("breed_dir") && !(mask & 4)) {
			int x = 0, y = 0, len = 0;
			sscanf(line.c_str(), "%d %d %n", &x, &y, &len);
			line.erase(0, len);
			if (x < -1) x = -1; if (x > 1) x = 1;
			if (y < -1) y = -1; if (y > 1) y = 1;
			if (!agent->breed_dir(x, y, line))
				bump();
			mask |= 4;
		} else if (!negotiation && !cmd.compare("secrete")) {
			int id = 0; double v = 0;
			sscanf(line.c_str(), "%d %lf", &id, &v);
			if (id < 0) id = 0; if (id >= PH_COUNT) id = PH_COUNT - 1;
			if (v < 0) v = 0;
			if (!agent->secrete(id, v))
				bump();
			/* We deliberately allow multiple secrete commands. */

		} else {
			std::cout << "unknown line " << cmd << " " << line << " ...\n";
		}
	}
	in_buf.erase(0, 2);

	if (negotiation) {
		agent->newborn = negotiation = false;
		if (!agent->tile)
			agent->spawn();

		std::stringstream s;
		s << "agent_id " << agent->id << "\r\n";
		out_buf.append(s.str());
	}

	pthread_mutex_unlock(&buf_lock);
}

void
connection::cancel(void)
{
	error = true; // XXX: atomic, hopefully
	pthread_cond_signal(&cancel_cond);
}


void *
conn_thread_worker(void *ctx)
{
	class connection *conn = (class connection *) ctx;
	conn->thread_loop();
	return NULL;
}

void
connection::spawn_thread(void)
{
	pthread_mutex_init(&buf_lock, NULL);
	pthread_cond_init(&cancel_cond, NULL);
	pthread_mutex_init(&cancel_lock, NULL);

	pthread_t tid;
	pthread_create(&tid, NULL, conn_thread_worker, (void *) this);
	pthread_detach(tid);
}

void
connection::thread_loop(void)
{
	while (!error) {
		/* Repeatedly try to write and read stuff. */
		std::string buf;
		ssize_t len;

		pthread_mutex_lock(&buf_lock);
		buf = out_buf;
		pthread_mutex_unlock(&buf_lock);

		if (buf.size() > 0) {
			len = write(fd, buf.c_str(), buf.size());
			if (len < 0) {
				pthread_mutex_lock(&cancel_lock);
				error = true;
			} else {
				pthread_mutex_lock(&buf_lock);
				out_buf.erase(0, len);
				pthread_mutex_unlock(&buf_lock);
			}
		}

		struct timeval tv;
		tv.tv_sec = 0; tv.tv_usec = 0;
		fd_set rfds; FD_ZERO(&rfds); FD_SET(fd, &rfds);
		fd_set efds; FD_ZERO(&efds); FD_SET(fd, &efds);
		while (select(fd + 1, &rfds, NULL, &efds, &tv)) {
			if (FD_ISSET(fd, &efds)) {
				error = true;
				break;
			}

			char cbuf[1024];
			len = read(fd, cbuf, sizeof(cbuf));
			if (len <= 0) {
				error = true;
				break;

			} else {
				bool want_moar = false;
				pthread_mutex_lock(&buf_lock);
				in_buf += std::string(cbuf, len);
				want_moar = buf_incomplete(in_buf);
				pthread_mutex_unlock(&buf_lock);
				if (!want_moar)
					break;
			}
		}

		usleep(10000); // XXX: Signal-oriented instead.
	}

	pthread_cond_wait(&cancel_cond, &cancel_lock);
	delete this;
}
