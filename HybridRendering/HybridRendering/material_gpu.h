#ifndef MATERIAL_GPU_H
#define MATERIAL_GPU_H

#include <glm/glm.hpp>

// vec4 to match std430 layout
struct MaterialGPU {
    glm::vec4 albedo; // w component unused
    float reflectivity; // 0 = fully diffuse, 1 = perfect mirror
    float roughness; // 0 = perfect mirror, 1 = fully rough
    float padding[2]; // Padding to 16-byte multiple for std430

    MaterialGPU() = default;

    MaterialGPU(
        const glm::vec3& _albedo,
        float _reflectivity,
        float _roughness
    )
        : albedo(_albedo, 0.0f)
        , reflectivity(_reflectivity)
        , roughness(_roughness)
        , padding{ 0.0f, 0.0f }
    {
    }
};

#endif // !MATERIAL_GPU_H
