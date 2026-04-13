#ifndef INSTANCE_GPU_H
#define INSTANCE_GPU_H

#include <glm/glm.hpp>

struct InstanceGPU {
    glm::mat4 transform; // object to world
    glm::mat4 worldToObject; // world to object
    glm::mat4 inverseTranspose; // transpose(inverse(transform)), for normals
    glm::uvec4 blasInfo; // Info about the blas

    // blasInfo.x = index of the first triangle in the shared SSBO for this BLAS
    // blasInfo.y = number of triangles in this BLAS
    // blasInfo.z = which BVH node SSBO to use (index)
    // blasInfo.w is UNUSED

    InstanceGPU() = default;

    InstanceGPU(
        const glm::mat4& _transform,
        uint32_t _blasOffset,
        uint32_t _blasTriCount,
        uint32_t _node
    )
        : transform(_transform)
        , worldToObject(glm::inverse(_transform))
        , inverseTranspose(glm::transpose(glm::inverse(_transform)))
        , blasInfo(_blasOffset, _blasTriCount, _node, 0u)
    {
    }
};

#endif // !INSTANCE_GPU_H
