#ifndef UTILITY_H
#define UTILITY_H

#include <string_view>

#include <GLFW/glfw3.h>

#include "settings.h"

namespace Utility {
	GLFWwindow* initializeWindow();

	void framebufferSizeCallback(GLFWwindow* window, int width, int height);

	void processInput(GLFWwindow* window, Settings::RenderSettings& renderSettings, float deltaTime);

	unsigned int loadTexture(std::string_view path, int activeTextureUnit);

	void renderCube();

	void renderFloor();

	void renderQuad();

	void setupImguiWindow(Settings::RenderSettings& renderSettings);
}

#endif // !UTILITY_H
