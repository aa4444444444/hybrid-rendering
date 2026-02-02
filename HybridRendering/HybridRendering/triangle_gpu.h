#ifndef TRIANGLE_GPU_H
#define TRIANGLE_GPU_H

#include <glm/glm.hpp>

// vec4 to match std430 layout
struct TriangleGPU {
	glm::vec4 v0;
	glm::vec4 v1;
	glm::vec4 v2;
	glm::vec4 normal;
	uint32_t id;
	uint32_t padding[3]; // pad to 16-byte multiple

    TriangleGPU(
        const glm::vec4& _v0,
        const glm::vec4& _v1,
        const glm::vec4& _v2,
        const glm::vec4& _normal,
        uint32_t _id
    )
        : v0(_v0)
        , v1(_v1)
        , v2(_v2)
        , normal(_normal)
        , id(_id)
        , padding{ 0, 0, 0 }
    {
    }
};

#endif // !TRIANGLE_GPU_H
