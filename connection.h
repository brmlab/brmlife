#ifndef BRMLIFE__CONNECTION_H
#define BRMLIFE__CONNECTION_H

#include <string>

#include <pthread.h>
#include <unistd.h>

#include "map.h"

class connection {
public:
	int fd;
	bool error;

	connection(int fd_)
	: fd(fd_), error(false)
	{
		spawn_thread();
	}

	~connection()
	{
		close(fd);
	}

	void senses(int tick_id, char around[4]);

	void cancel(void);

private:
	std::string out_buf;
	pthread_mutex_t buf_lock;

	pthread_cond_t cancel_cond;
	pthread_mutex_t cancel_lock;

	void spawn_thread(void);
	friend void *conn_thread_worker(void *ctx);
	void thread_loop(void);
};

#endif
