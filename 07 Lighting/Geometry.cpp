#include "Geometry.h"

const std::map<std::string, std::pair<size_t, size_t>> Geometry::semanticSizeMap = {
	{"POSITION", std::pair<size_t, size_t>(0, 12)},
	{"NORMAL", std::pair<size_t, size_t>(12, 24)},
	{"TANGENT", std::pair<size_t, size_t>(24, 40)},
	{"COLOR", std::pair<size_t, size_t>(40, 56)},
	{"TEXCOORD", std::pair<size_t, size_t>(56, 64)}
};