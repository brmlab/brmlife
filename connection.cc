#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <pthread.h>

#include "agent.h"
#include "connection.h"

void
connection::senses(int tick_id, char around[4])
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "tick %d\naround %c%c%c%c\n\n", tick_id, around[0], around[1], around[2], around[3]);

	pthread_mutex_lock(&buf_lock);
	out_buf.append(buf);
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

		len = write(fd, buf.c_str(), buf.size());
		if (len < 0) {
			pthread_mutex_lock(&cancel_lock);
			error = true;
		} else {
			pthread_mutex_lock(&buf_lock);
			out_buf.erase(0, len);
			pthread_mutex_unlock(&buf_lock);
		}

		/* TODO: The reading. ;-) */

		usleep(10000); // XXX: Signal-oriented instead.
	}

	pthread_cond_wait(&cancel_cond, &cancel_lock);
	delete this;
}
