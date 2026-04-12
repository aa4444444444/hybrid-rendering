#ifndef INSTANCE_GPU_H
#define INSTANCE_GPU_H

#include <glm/glm.hpp>

struct InstanceGPU {
    glm::mat4 transform; // object to world
    glm::mat4 inverseTranspose; // transpose(inverse(transform)), for normals

    InstanceGPU() = default;

    InstanceGPU(
        const glm::mat4& _transform,
        const glm::mat4& _inverseTranspose
    )
        : transform(_transform)
        , inverseTranspose(_inverseTranspose)
    {
    }
};

#endif // !INSTANCE_GPU_H
