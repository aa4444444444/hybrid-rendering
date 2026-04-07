#ifndef TRIANGLE_GPU_H
#define TRIANGLE_GPU_H

#include <glm/glm.hpp>

#include "material_gpu.h"

// vec4 to match std430 layout
struct TriangleGPU {
	glm::vec4 v0;
	glm::vec4 v1;
	glm::vec4 v2;
    glm::vec4 normal;
    glm::vec4 diffuseColor;
	uint32_t id;
    uint32_t materialID;
	uint32_t padding[2]; // pad to 16-byte multiple

    TriangleGPU() = default;

    TriangleGPU(
        const glm::vec4& _v0,
        const glm::vec4& _v1,
        const glm::vec4& _v2,
        const glm::vec4& _normal,
        const glm::vec4& _diffuseColor,
        uint32_t _id,
        uint32_t _materialID
    )
        : v0(_v0)
        , v1(_v1)
        , v2(_v2)
        , normal(_normal)
        , diffuseColor(_diffuseColor)
        , id(_id)
        , materialID(_materialID)
        , padding{ 0, 0 }
    {
    }
};

#endif // !TRIANGLE_GPU_H
