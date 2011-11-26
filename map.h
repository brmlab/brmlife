#ifndef BRMLIFE__MAP_H
#define BRMLIFE__MAP_H

class agent;
class map;

class tile {
public:
	int x, y;
	class map &map;

	class agent *agent;

	tile(int x_, int y_, class map &map_) : x(x_), y(y_), map(map_), agent(NULL) {};

	char symbol(void);
	virtual char type_symbol(void) = 0;

	class tile &tile_in_dir(int dir_x, int dir_y);

	virtual bool on_agent_enter(class agent &);
	virtual void on_agent_leave(class agent &);

	virtual void on_tick(void);
};

class tile_ground : public tile {
public:
	tile_ground(int x_, int y_, struct map &map_) : tile(x_, y_, map_) {};
private:
	char type_symbol(void);
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

		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				tiles[y * h + x] = new tile_ground(x, y, *this);
			}
		}
	};

	class tile &tile_at(int x, int y)
	{
		return *tiles[y * h + x];
	};

	void print_map(void);
};

#endif
