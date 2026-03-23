#ifndef SETTINGS_H
#define SETTINGS_H

#include "constants.h"
#include "camera.h"

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
		Whether to use BVH
	*/
	enum class BVHRenderMode {
		off, // 0
		on, // 1
		num_options
	};

	/*
		Defines various render settings
	*/
	struct RenderSettings {
		GBufferRenderMode gBufferRenderMode { GBufferRenderMode::texture };
		DeferredShadingRenderMode deferredShadingRenderMode { DeferredShadingRenderMode::texture };
		SVGFRenderMode svgfRenderMode{ SVGFRenderMode::on };
		BVHRenderMode bvhRenderMode{ BVHRenderMode::on };
		bool enableMouseLook{ false };

		Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f) };

		float lastX{ Constants::SCR_WIDTH / 2.0f };
		float lastY{ Constants::SCR_HEIGHT / 2.0f };
	};
}

#endif