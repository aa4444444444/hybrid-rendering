#include <array>

#include <glad/glad.h>
#include <plog/Log.h>
#include "plog/Appenders/ColorConsoleAppender.h"
#include <plog/Formatters/TxtFormatter.h>
#include "plog/Initializers/ConsoleInitializer.h"
#include "stb_image.h"

#include "utility.h"
#include "constants.h"
#include "camera.h"
#include <imgui.h>

namespace Utility {
    GLFWwindow* initializeWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Using OpenGL 4.6
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window{ glfwCreateWindow(Constants::SCR_WIDTH, Constants::SCR_HEIGHT, "HybridRendering", NULL, NULL) };
        if (window == NULL) {
            PLOGE << "Failed to create GLFW window";
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            PLOGE << "Failed to initialize GLAD";
            return nullptr;
        }

        glViewport(0, 0, Constants::SCR_WIDTH, Constants::SCR_HEIGHT);
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

        return window;
    }

    void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

    void processInput(GLFWwindow* window, Settings::RenderSettings& renderSettings, float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Change the interactivity
        // Pressing 1 will disable camera movement on mouse movement and
        // will reveal the cursor
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            renderSettings.enableMouseLook = false;
        }
        // Pressing 2 will enable camera movement on mouse movement and
        // will disable the cursor
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            renderSettings.enableMouseLook = true;
        }

        // Move camera position with keys
        Camera& camera = renderSettings.camera;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);

        // Adjust camera angle with keys
        constexpr float adjustAmount{ 0.4f };
        float& lastY = renderSettings.lastY;
        float& lastX = renderSettings.lastX;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            lastY += adjustAmount;
            if (lastY > Constants::SCR_HEIGHT) {
                lastY = Constants::SCR_HEIGHT;
            }
            camera.ProcessMouseMovement(0.0f, adjustAmount);
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            lastY -= adjustAmount;
            if (lastY < 0.0f) {
                lastY = 0.0f;
            }
            camera.ProcessMouseMovement(0.0f, -adjustAmount);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            lastX -= adjustAmount;
            if (lastX < 0.0f) {
                lastX = 0.0f;
            }
            camera.ProcessMouseMovement(-adjustAmount, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            lastX += adjustAmount;
            if (lastX > Constants::SCR_WIDTH) {
                lastX = Constants::SCR_WIDTH;
            }
            camera.ProcessMouseMovement(adjustAmount, 0.0f);
        }
    }

    unsigned int loadTexture(std::string_view path, int activeTextureUnit)
    {
        unsigned int textureID{};
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path.data(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glActiveTexture(activeTextureUnit);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glActiveTexture(GL_TEXTURE0);

            stbi_image_free(data);
        }
        else
        {
            PLOGE << "Texture failed to load at path: " << path;
            stbi_image_free(data);
        }

        return textureID;
    }

    // renderCube() renders a 1x1 3D cube in NDC.
    unsigned int cubeVAO{ 0 };
    unsigned int cubeVBO{ 0 };
    void renderCube()
    {
        // initialize (if necessary)
        if (cubeVAO == 0) {
            // Positions, normals, and texture coordinates of a single 3D cube
            constexpr std::array<float, 8 * 36> cubeVertices{
                // positions          // normals           // texture coords
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
                 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
                 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
                -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
                -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

                 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
                 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
                 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
                 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
                 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
                 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
                 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
                 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
            };
            glGenVertexArrays(1, &cubeVAO);
            glGenBuffers(1, &cubeVBO);
            // fill buffer
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices.data(), GL_STATIC_DRAW);
            // link vertex attributes
            glBindVertexArray(cubeVAO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
        // render Cube
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    unsigned int floorVAO{ 0 };
    unsigned int floorVBO{ 0 };
    void renderFloor() {
        if (floorVAO == 0) {
            float floorVertices[] = {
                // positions        // normals        // texture coords
                -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                 1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,

                 1.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            };

            // setup floor VAO
            glGenVertexArrays(1, &floorVAO);
            glGenBuffers(1, &floorVBO);
            glBindVertexArray(floorVAO);
            glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), &floorVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    // renderQuad() renders a 1x1 XY quad in NDC
    unsigned int quadVAO{ 0 };
    unsigned int quadVBO{};
    void renderQuad()
    {
        if (quadVAO == 0)
        {
            float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            };
            // setup plane VAO
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void setupImguiWindow(Settings::RenderSettings& renderSettings) {
        ImGui::Begin("Render Settings");

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        static ImGuiComboFlags renderModeFlags = 0;
        renderModeFlags |= ImGuiComboFlags_PopupAlignLeft;
        renderModeFlags |= ImGuiComboFlags_WidthFitPreview;
        renderModeFlags &= ~ImGuiComboFlags_NoPreview;

        const std::array<std::string, 5> renderModes{
            "Textures",
            "Position",
            "Normals",
            "Albedo",
            "Specular",
        };

        const std::string renderModePreview{ renderModes[renderSettings.renderMode] };

        if (ImGui::BeginCombo("Render Mode", renderModePreview.c_str(), renderModeFlags)) {
            for (int i{ 0 }; i < Settings::RenderMode::num_options; ++i) {
                bool is_selected{ renderSettings.renderMode == i };

                if (ImGui::Selectable(renderModes[i].c_str(), is_selected))
                    renderSettings.renderMode = static_cast<Settings::RenderMode>(i);

                if (renderSettings.renderMode == i)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::End();
    }
}
