#ifndef BRMLIFE__AGENT_H
#define BRMLIFE__AGENT_H

class position;

class agent {
public:
	int id;
	class position *pos;

	agent(int id_, class position &pos_) : id (id_), pos (&pos_)
	{
		put_at(pos);
	};

	void put_at(struct position *pos);
};

#endif
