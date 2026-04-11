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
    glm::vec4 diffuseColor0;
    glm::vec4 diffuseColor1;
    glm::vec4 diffuseColor2;
	uint32_t id;
    uint32_t materialID;
	uint32_t padding[2]; // pad to 16-byte multiple

    TriangleGPU() = default;

    TriangleGPU(
        const glm::vec4& _v0,
        const glm::vec4& _v1,
        const glm::vec4& _v2,
        const glm::vec4& _normal,
        const glm::vec4& _diffuseColor0,
        const glm::vec4& _diffuseColor1,
        const glm::vec4& _diffuseColor2,
        uint32_t _id,
        uint32_t _materialID
    )
        : v0(_v0)
        , v1(_v1)
        , v2(_v2)
        , normal(_normal)
        , diffuseColor0(_diffuseColor0)
        , diffuseColor1(_diffuseColor1)
        , diffuseColor2(_diffuseColor2)
        , id(_id)
        , materialID(_materialID)
        , padding{ 0, 0 }
    {
    }
};

#endif // !TRIANGLE_GPU_H
