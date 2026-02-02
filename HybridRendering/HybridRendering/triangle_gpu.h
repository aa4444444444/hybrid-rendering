#ifndef TRIANGLE_GPU_H
#define TRIANGLE_GPU_H

#include <glm/glm.hpp>

// vec4 to match std430 layout
struct TriangleGPU {
	glm::vec4 v0;
	glm::vec4 v1;
	glm::vec4 v2;
	glm::vec4 normal;
};

#endif // !TRIANGLE_GPU_H
