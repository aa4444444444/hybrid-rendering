#ifndef SCENE_CONFIG_H
#define SCENE_CONFIG_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

enum class SceneID {
	Backpack9,
	Sponza,
};

inline constexpr SceneID ACTIVE_SCENE = SceneID::Sponza;

struct SceneConfig {
	std::string modelPath{};

	std::vector<glm::vec3> instancePositions{};
	float instanceScale{ 1.0f };

	bool includeFloor{ false };

	glm::vec3 cameraStart{ 0.0f, 0.0f, 7.7f };

	glm::vec3 lightPosition{ 0.0f, 3.5f, 0.0f };

	glm::vec3 meshAlbedo{ 0.9f, 0.7f, 0.6f };
	float meshReflectivity{ 1.0f };
	float meshRoughness{ 0.0f };

	// Only used when includeFloor = true
	glm::vec3 floorAlbedo{ 0.9f, 0.9f, 0.9f };
	float floorReflectivity{ 1.0f };
	float floorRoughness{ 0.0f };

	// Per-frame animation
	bool animateInstances{ false };
};

inline SceneConfig getSceneConfig(SceneID id) {
    switch (id) {

    case SceneID::Backpack9: {
        SceneConfig cfg;
        cfg.modelPath = "resources/objects/backpack/backpack.obj";
        cfg.instancePositions = {
            {-3.0f, -0.5f, -3.0f}, { 0.0f, -0.5f, -3.0f}, { 3.0f, -0.5f, -3.0f},
            {-3.0f, -0.5f,  0.0f}, { 0.0f, -0.5f,  0.0f}, { 3.0f, -0.5f,  0.0f},
            {-3.0f, -0.5f,  3.0f}, { 0.0f, -0.5f,  3.0f}, { 3.0f, -0.5f,  3.0f},
        };
        cfg.instanceScale = 0.3f;
        cfg.includeFloor = true;
        cfg.cameraStart = { 0.0f, 0.0f, 7.7f };
        cfg.lightPosition = { 0.0f, 0.05f, 2.0f };
        cfg.meshAlbedo = { 0.8f, 0.7f, 0.6f };
        cfg.meshReflectivity = 1.0f;
        cfg.meshRoughness = 0.0f;
        cfg.floorAlbedo = { 0.9f, 0.9f, 0.9f };
        cfg.floorReflectivity = 1.0f;
        cfg.floorRoughness = 0.0f;
        cfg.animateInstances = true;
        return cfg;
    }

    case SceneID::Sponza: {
        SceneConfig cfg;
        cfg.modelPath = "resources/objects/sponza-glTF/Sponza.gltf";
        cfg.instancePositions = { { 0.0f, 0.0f, 0.0f } }; // single instance
        cfg.instanceScale = 1.0f;
        cfg.includeFloor = false; // Sponza has its own floor geometry
        cfg.cameraStart = { 0.0f, 1.5f, 0.0f };
        cfg.lightPosition = { 0.0f, 3.5f, 0.0f };
        cfg.meshAlbedo = { 0.8f, 0.8f, 0.8f };
        cfg.meshReflectivity = 0.3f;
        cfg.meshRoughness = 0.7f;
        cfg.animateInstances = false;
        return cfg;
    }
    }
    return SceneConfig{};
}

#endif