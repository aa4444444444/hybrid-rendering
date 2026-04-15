#ifndef CPU_TEXTURE_H
#define CPU_TEXTURE_H

#include <glm/glm.hpp>
#include "stb_image.h"

struct CpuTexture {
    unsigned char* data{ nullptr };
    int width{ 0 };
    int height{ 0 };
    int channels{ 0 };

    bool valid() const { return data != nullptr && width > 0 && height > 0; }

    // Bilinear sample at UV coordinates [0,1]. Returns sRGB in [0,1].
    glm::vec3 sample(glm::vec2 uv) const {
        if (!valid()) return glm::vec3(0.8f); // neutral grey fallback

        // Wrap UVs (repeat mode)
        uv.x = uv.x - std::floor(uv.x);
        uv.y = uv.y - std::floor(uv.y);

        // stb_image loads with y=0 at top; flip to match OpenGL convention
        uv.y = 1.0f - uv.y;

        float px = uv.x * static_cast<float>(width - 1);
        float py = uv.y * static_cast<float>(height - 1);

        int x0 = static_cast<int>(px);
        int y0 = static_cast<int>(py);
        int x1 = std::min(x0 + 1, width - 1);
        int y1 = std::min(y0 + 1, height - 1);
        float fx = px - static_cast<float>(x0);
        float fy = py - static_cast<float>(y0);

        auto fetch = [&](int x, int y) -> glm::vec3 {
            int idx = (y * width + x) * channels;
            float r = data[idx + 0] / 255.0f;
            float g = (channels >= 2) ? data[idx + 1] / 255.0f : r;
            float b = (channels >= 3) ? data[idx + 2] / 255.0f : r;
            return glm::vec3(r, g, b);
            };

        // Bilinear interpolation
        glm::vec3 c00 = fetch(x0, y0);
        glm::vec3 c10 = fetch(x1, y0);
        glm::vec3 c01 = fetch(x0, y1);
        glm::vec3 c11 = fetch(x1, y1);

        return glm::mix(glm::mix(c00, c10, fx),
            glm::mix(c01, c11, fx), fy);
    }

    void free() {
        if (data) { stbi_image_free(data); data = nullptr; }
    }
};

#endif