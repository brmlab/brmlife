#ifndef BRMLIFE__MAP_H
#define BRMLIFE__MAP_H

class agent;
class map;

class tile {
public:
	class agent *agent;

	virtual void on_agent_enter(class agent *);
	virtual void on_agent_leave(class agent *);

	virtual void on_tick(void);

	virtual char symbol(void) = 0;
};

class tile_ground : public tile {
	char symbol(void);
};

class position {
public:
	int x, y;
	class map *map;

	position(int x_, int y_, class map *map_)
	{
		x = x_;
		y = y_;
		map = map_;
	};

	class position *next_in(int dir_x, int dir_y);
};

class map {
public:
	class tile **tiles;
	int w, h;

	map(int w_, int h_)
	{
		w = w_;
		h = h_;
		tiles = new class tile * [w * h];

		for (int i = 0; i < w * h; i++) {
			tiles[i] = new tile_ground;
		}
	};

	class tile &tile_at(class position &pos)
	{
		return *tiles[pos.y * h + pos.x];
	};

	void print_map(void);
};

#endif
