#ifndef RAWIO
#define RAWIO

#include "map.h"
#include <fstream>
#include <iostream>

int rawio_map(map *map)
{
	std::ofstream mapout;
	mapout.open("rawio_map",std::ios::out|std::ios::trunc);
	if(mapout.is_open())
	{
		for (int y = 0; y < map->h; y++)
		{
			for (int x = 0; x < map->w; x++)
				mapout << map->tile_at(x, y).symbol();
			mapout << std::endl;
		}
		mapout.close();
		return 0;
	}
	return -1;//cannot open outstream
}

int rawio_cfg(map *map)
{
	std::ofstream cfgout;
	cfgout.open("rawio_cfg",std::ios::out|std::ios::trunc);
	if(cfgout.is_open())
	{
		cfgout<<map->w<<std::endl;
		cfgout<<map->h<<std::endl;
		cfgout.close();
		return 0;
	}
	return -1;
}
#endif