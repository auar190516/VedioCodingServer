#ifndef PLY_LOADER_H
#define PLY_LOADER_H

#include <vector>
#include <string>

struct Point {
	float x, y, z;
	float r, g, b;
};

std::vector<Point> loadPLY(const std::string& filename);

#endif // PLY_LOADER_H
