#ifndef SETTINGS_H
#define SETTINGS_H

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
	};
}

#endif