#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>

#include "agent.h"
#include "connection.h"

void
connection::senses(int tick_id, bool dead, int energy, char around[4])
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "tick %d\r\n%senergy %d\r\naround %.8s\r\n\r\n", tick_id, dead ? "DEAD\r\n" : "", energy, around);

	pthread_mutex_lock(&buf_lock);
	out_buf.append(buf);
	pthread_mutex_unlock(&buf_lock);
}

void
connection::bump(void)
{
	pthread_mutex_lock(&buf_lock);
	out_buf.append("BUMP\r\n");
	pthread_mutex_unlock(&buf_lock);
}

void
connection::actions(class agent *agent)
{
	pthread_mutex_lock(&buf_lock);
	if (in_buf.find("\r\n\r\n") == std::string::npos) {
		/* Not enough data, needs to wait until next turn, sorry. */
		pthread_mutex_unlock(&buf_lock);
		return;
	}

	while (in_buf.c_str()[0] != '\r') {
		int nlofs = in_buf.find("\r\n");
		std::string line = in_buf.substr(0, nlofs);
		in_buf.erase(0, nlofs + 2);

		int spofs = line.find(' ');
		std::string cmd = line.substr(0, spofs);
		line.erase(0, spofs + 1);

		if (!cmd.compare("move_dir")) {
			int x = 0, y = 0;
			sscanf(line.c_str(), "%d %d", &x, &y);
			if (x < -1) x = -1; if (x > 1) x = 1;
			if (y < -1) y = -1; if (y > 1) y = 1;
			if (!agent->move_dir(x, y))
				bump();
		} else {
			std::cout << "unknown line " << cmd << " " << line << " ...\n";
		}
	}
	in_buf.erase(0, 2);

	pthread_mutex_unlock(&buf_lock);
}

void
connection::cancel(void)
{
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
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		while (select(fd + 1, &rfds, NULL, NULL, &tv)) {
			char cbuf[1024];
			len = read(fd, cbuf, sizeof(cbuf));
			if (len < 0) {
				error = true;
			} else if (len == 0) {
				break;
			} else {
				bool want_moar = false;
				pthread_mutex_lock(&buf_lock);
				in_buf.append(cbuf, len);
				want_moar = (in_buf.find("\r\n\r\n") == std::string::npos);
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
