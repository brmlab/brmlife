#ifndef BRMLIFE__CONNECTION_H
#define BRMLIFE__CONNECTION_H

#include <string>

#include <pthread.h>
#include <unistd.h>

#include "map.h"

class agent;

class connection {
public:
	int fd;
	bool negotiation;
	bool error;

	connection(int fd_)
	: fd(fd_), negotiation(true), error(false)
	{
		spawn_thread();
	}

	~connection()
	{
		close(fd);
	}

	void senses(int tick_id, class agent &);
	void bred(int agent_id, std::string &info);
	void actions(class agent *);

	void cancel(void);

private:
	void bump(void);

	std::string out_buf, in_buf;
	pthread_mutex_t buf_lock;

	pthread_cond_t cancel_cond;
	pthread_mutex_t cancel_lock;

	void spawn_thread(void);
	friend void *conn_thread_worker(void *ctx);
	void thread_loop(void);
};

#endif
