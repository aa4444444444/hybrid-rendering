#ifndef SETTINGS_H
#define SETTINGS_H

#include "constants.h"
#include "camera.h"
#include <glm/glm.hpp>
#include <chrono>
#include <array>

namespace Settings {
	/*
		How we want to "render" objects in the geometry pass
	*/
	enum class GBufferRenderMode {
		// We don't use an enum class so we can leverage the implicit conversion to ints
		texture, // 0 
		position, // 1
		normals, // 2
		albedo, // 3
		spec, // 4
		motion, // 5
		depth, // 6
		num_options,
	};

	/*
		How we want to render lighting in the deferred shading pass
	*/
	enum class DeferredShadingRenderMode {
		texture, // 0
		shadows, // 1
		num_options
	};

	/*
		Whether to use SVGF
	*/
	enum class SVGFRenderMode {
		on, // 0
		temporal, // 1
		spatial, // 2
		off, // 3
		num_options
	};

	/*
		Whether to use Reflections
	*/
	enum class ReflectionRenderMode {
		off, // 0
		on, // 1
		num_options
	};

	/*
		Whether to use BVH
	*/
	enum class BVHRenderMode {
		off, // 0
		on, // 1
		num_options
	};

	/*
		Intensity of the lights
	*/
	enum class LightIntensity {
		zero, // 0
		one, // 1
		two, // 2
		three, // 3
		four, // 4
		five, // 5
		six, // 6
		seven, // 7
		eight, // 8
		nine, // 9
		ten, // 10
		eleven, //11
		num_options
	};

	inline std::vector<glm::vec3> lightAttenuation {
		{1.0, 0.7, 1.8},
		{1.0, 0.35, 0.44},
		{1.0, 0.22, 0.20},
		{1.0, 0.14, 0.07},
		{1.0, 0.09, 0.032},
		{1.0, 0.07, 0.017},
		{1.0, 0.045, 0.0075},
		{1.0, 0.027, 0.0028},
		{1.0, 0.022, 0.0019},
		{1.0, 0.014, 0.0007},
		{1.0, 0.007, 0.0002},
		{1.0, 0.0014, 0.000007},
	};

	// Per-stage GPU timing helper struct
	// Call glFinish() before and after each stage of the hybrid pipeline
	// Using exponential moving average with alpha = 0.05 so the displayed
	// value smooths out frame-to-frame noise without lagging too much. 
	struct StageTimer {
		static constexpr float kAlpha{ 0.05f }; // EMA smoothing factor

		float avgMs{ 0.0f };

		// Call this once per frame with the raw measured milliseconds for this stage.
		void update(float measuredMs) {
			if (avgMs == 0.0f)
				avgMs = measuredMs;
			else
				avgMs = kAlpha * measuredMs + (1.0f - kAlpha) * avgMs;
		}
	};

	// record a wall-clock start point (call glFinish() BEFORE this).
	inline std::chrono::steady_clock::time_point stageBegin() {
		return std::chrono::steady_clock::now();
	}

	// call glFinish(), then compute elapsed ms and feed the timer.
	inline void stageEnd(StageTimer& timer, std::chrono::steady_clock::time_point start) {
		glFinish();
		auto end = std::chrono::steady_clock::now();
		float ms = std::chrono::duration<float, std::milli>(end - start).count();
		timer.update(ms);
	}

	/*
		Defines various render settings
	*/
	struct RenderSettings {
		GBufferRenderMode gBufferRenderMode { GBufferRenderMode::texture };
		DeferredShadingRenderMode deferredShadingRenderMode { DeferredShadingRenderMode::texture };
		SVGFRenderMode svgfRenderMode{ SVGFRenderMode::on };
		ReflectionRenderMode reflectionRenderMode{ ReflectionRenderMode::on };
		BVHRenderMode bvhRenderMode{ BVHRenderMode::on };
		LightIntensity lightIntensity{ LightIntensity::six };
		bool enableMouseLook{ false };

		Camera camera{ glm::vec3(0.0f, 0.0f, 7.7f) };

		float lastX{ Constants::SCR_WIDTH / 2.0f };
		float lastY{ Constants::SCR_HEIGHT / 2.0f };

		StageTimer timerGBuffer{}; // Geometry pass
		StageTimer timerShadowRT{}; // Shadow ray-tracing compute
		StageTimer timerReflectionRT{}; // Reflection ray-tracing compute
		StageTimer timerTemporalAccum{}; // SVGF temporal accumulation
		StageTimer timerSpatialFilter{}; // SVGF spatial (A-Trous) filter
		StageTimer timerLightingPass{};  // Deferred lighting / shading quad

		bool showPipelineTime{ false };
	};
}

#endif