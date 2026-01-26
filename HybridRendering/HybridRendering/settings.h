#ifndef SETTINGS_H
#define SETTINGS_H

#include "constants.h"
#include "camera.h"

namespace Settings {
	/*
		How we want to render objects
	*/
	enum RenderMode {
		// We don't use an enum class so we can leverage the implicit conversion to ints
		texture, // 0 
		position, // 1
		normals, // 2
		albedo, // 3
		spec, // 4
		num_options, // 5
	};

	/*
		Defines various render settings
	*/
	struct RenderSettings {
		RenderMode renderMode{ texture };
		bool enableMouseLook{ false };

		Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f) };

		float lastX{ Constants::SCR_WIDTH / 2.0f };
		float lastY{ Constants::SCR_HEIGHT / 2.0f };
	};
}

#endif