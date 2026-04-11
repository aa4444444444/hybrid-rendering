#ifndef BAKED_TRIANGLE_H
#define BAKED_TRIANGLE_H

#include <glm/glm.hpp>

// Per-triangle baked data.
// Uses object-space vertices, object-space normal, and diffuse colors.
// These are baked once and re-used every frame.
// 
// Note that only the world-space positions/normals need to be re-computed when 
// an object moves, so we separate them from the actual color data. 
struct BakedTriangle {
	glm::vec3 lv0, lv1, lv2;
	glm::vec3 lNormal;
	glm::vec4 diffuse0, diffuse1, diffuse2;
	uint32_t materialID;
	uint32_t instanceIndex;
	uint32_t id;

	BakedTriangle() = default;

	BakedTriangle(
		const glm::vec3 _lv0,
		const glm::vec3 _lv1,
		const glm::vec3 _lv2,
		const glm::vec3 _lNormal,
		const glm::vec4 _diffuse0,
		const glm::vec4 _diffuse1,
		const glm::vec4 _diffuse2,
		const uint32_t _materialID,
		const uint32_t _instanceIndex,
		const uint32_t _id
	)
		: lv0(_lv0)
		, lv1(_lv1)
		, lv2(_lv2)
		, lNormal(_lNormal)
		, diffuse0(_diffuse0)
		, diffuse1(_diffuse1)
		, diffuse2(_diffuse2)
		, materialID(_materialID)
		, instanceIndex(_instanceIndex)
		, id(_id)
	{
	}

};

#endif