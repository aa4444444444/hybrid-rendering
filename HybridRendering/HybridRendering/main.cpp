#include <array>
#include <iostream>
#include <string_view>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <plog/Log.h>
#include "plog/Appenders/ColorConsoleAppender.h"
#include <plog/Formatters/TxtFormatter.h>
#include "plog/Initializers/ConsoleInitializer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "constants.h"
#include "camera.h"
#include "shader.h"
#include "settings.h"
#include "utility.h"
#include "triangle_gpu.h"

// forward declarations
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// settings
bool firstMouse{ true };
bool firstRenderPass{ true };
bool firstFrame{ true };
Settings::RenderSettings renderSettings{};

// timing
float deltaTime{ 0.0f };
float lastFrame{ 0.0f };

// Object id counter
static uint32_t nextID = 0;

int main() {
    plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);

    PLOGD << "Initializing Window";
        GLFWwindow* window{ Utility::initializeWindow() };
        if (window == nullptr) {
            PLOGE << "initializeWindow() failed";
            return -1;
        }
    PLOGD << "Finished Initializing Window";

    PLOGD << "Setting up Dear ImGUI";
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io{ ImGui::GetIO() };
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
        glfwSetCursorPosCallback(window, mouse_callback);    // Make sure this happens AFTER we install ImGui callbacks
        ImGui_ImplOpenGL3_Init();
    PLOGD << "Finished Setting up Dear ImGUI";

    // Setup OpenGL State
    glEnable(GL_DEPTH_TEST);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // Build and compile shaders
    Shader shaderGeometryPass{ "gbuffer.vert", "gbuffer.frag" };
    Shader shaderLightingPass{ "deferred_shading.vert", "deferred_shading.frag" };
    Shader shaderLightBox{ "deferred_light.vert", "deferred_light.frag" };
    Shader rayTraceShader{ "ray_trace.comp" };
    Shader temporalAccumulationShader{ "temporal_accumulation.comp" };
    Shader spatialFilteringShader{ "spatial_filtering.comp" };

    // Object positions
    std::vector<glm::vec3> objectPositions{};
    objectPositions.emplace_back(-3.0, -0.5, -3.0);
    objectPositions.emplace_back(0.0, -0.5, -3.0);
    objectPositions.emplace_back(3.0, -0.5, -3.0);
    objectPositions.emplace_back(-3.0, -0.5, 0.0);
    objectPositions.emplace_back(0.0, -0.5, 0.0);
    objectPositions.emplace_back(3.0, -0.5, 0.0);
    objectPositions.emplace_back(-3.0, -0.5, 3.0);
    objectPositions.emplace_back(0.0, -0.5, 3.0);
    objectPositions.emplace_back(3.0, -0.5, 3.0);

    // Object model transforms
    std::vector<glm::mat4> objectTransforms{};
    for (unsigned int i{ 0 }; i < objectPositions.size(); ++i) {
        glm::mat4 model{ glm::mat4(1.0f) }; 
        model = glm::translate(model, objectPositions[i]);
        model = glm::scale(model, glm::vec3(0.7f));
        
        
        objectTransforms.push_back(model);
    }

    // Floor model transform
    glm::mat4 floorModel{ glm::mat4(1.0f) };
    floorModel = glm::translate(floorModel, glm::vec3{ 0.0f, -1.5f, 0.0f });
    floorModel = glm::scale(floorModel, glm::vec3(5.0f));
    

    // Contains the triangles in the scene that will get passed to the GPU in an SSBO
    std::vector<TriangleGPU> gpuTriangles{};

    // Populating the gpuTriangles vector
    for (unsigned int i{ 0 }; i < objectPositions.size(); ++i)
    {
        glm::mat4 normalModel{ glm::transpose(glm::inverse(glm::mat3(objectTransforms[i]))) };

        // Since each point has 8 elements and we want 3 points per triangle
        for (unsigned int j{ 0 }; j < Utility::cubeVertices.size(); j += (8 * 3)) {
            gpuTriangles.emplace_back(
                objectTransforms[i] * glm::vec4{ Utility::cubeVertices[j], Utility::cubeVertices[j + 1], Utility::cubeVertices[j + 2], 1.0f },
                objectTransforms[i] * glm::vec4{ Utility::cubeVertices[j + 8], Utility::cubeVertices[j + 9], Utility::cubeVertices[j + 10], 1.0f },
                objectTransforms[i] * glm::vec4{ Utility::cubeVertices[j + 16], Utility::cubeVertices[j + 17], Utility::cubeVertices[j + 18], 1.0f },
                glm::normalize(normalModel * glm::vec4{ Utility::cubeVertices[j + 19], Utility::cubeVertices[j + 20], Utility::cubeVertices[j + 21], 0.0f }), // normal
                nextID++
            );
        }
    }

    glm::mat4 floorNormalModel{ glm::transpose(glm::inverse(glm::mat3(floorModel))) };

    // Since each point has 8 elements and we want 3 points per triangle
    for (unsigned int j{ 0 }; j < Utility::floorVertices.size(); j += (8 * 3)) {
        gpuTriangles.emplace_back(
            floorModel * glm::vec4{ Utility::floorVertices[j], Utility::floorVertices[j + 1], Utility::floorVertices[j + 2], 1.0f },
            floorModel * glm::vec4{ Utility::floorVertices[j + 8], Utility::floorVertices[j + 9], Utility::floorVertices[j + 10], 1.0f },
            floorModel * glm::vec4{ Utility::floorVertices[j + 16], Utility::floorVertices[j + 17], Utility::floorVertices[j + 18], 1.0f },
            glm::normalize(floorNormalModel * glm::vec4{ Utility::floorVertices[j + 19], Utility::floorVertices[j + 20], Utility::floorVertices[j + 21], 0.0f }), // normal
            nextID++
        );
    }

    /*for (const TriangleGPU& triangle : gpuTriangles) {
        PLOGD << "TRIANGLE:";
        PLOGD << "v0: " << glm::to_string(triangle.v0);
        PLOGD << "v1: " << glm::to_string(triangle.v1);
        PLOGD << "v2: " << glm::to_string(triangle.v2);
        PLOGD << "normal: " << glm::to_string(triangle.normal);
    }*/

    // Set up triangle SSBO
    unsigned int triangleSSBO{};
    glGenBuffers(1, &triangleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuTriangles.size() * sizeof(TriangleGPU), gpuTriangles.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // load textures
    unsigned int crateDiffuseMap{ Utility::loadTexture("resources/textures/container2.png", GL_TEXTURE0) };
    unsigned int crateSpecularMap{ Utility::loadTexture("resources/textures/container2_specular.png", GL_TEXTURE1) };
    unsigned int floorDiffuseMap{ Utility::loadTexture("resources/textures/floor.jpg", GL_TEXTURE2) };
    unsigned int floorSpecularMap{ Utility::loadTexture("resources/textures/floor_specular.jpg", GL_TEXTURE3) };
    unsigned int blueNoise{ Utility::loadNoiseTexture("resources/textures/blue_noise.png", GL_TEXTURE4)};

    /*rayTraceShader.use();
    rayTraceShader.setInt("gPosition", 0);
    rayTraceShader.setInt("gNormal", 1);*/

    shaderGeometryPass.use();
    shaderGeometryPass.setInt("texture_diffuse1", 0);
    shaderGeometryPass.setInt("texture_specular1", 1);

    // Configure the G-Buffer
    unsigned int gBuffer{};
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // G-Buffer keeps track of positions, normals, albedo, specular intensity, and motion/depth information
    unsigned int gPosition{}, gNormal{}, gAlbedoSpec{}, gMotionDepthVec{};

    // Position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Constants::SCR_WIDTH, Constants::SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // Normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Constants::SCR_WIDTH, Constants::SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // Alebdo + Spec color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Constants::SCR_WIDTH, Constants::SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    // Motion Vector Texture
    glGenTextures(1, &gMotionDepthVec);
    glBindTexture(GL_TEXTURE_2D, gMotionDepthVec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Constants::SCR_WIDTH, Constants::SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gMotionDepthVec, 0);

    // All 4 should be color attachments
    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    
    // Output from our fragment shader will be written into the 4 buffers
    glDrawBuffers(4, attachments);

    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth{};
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Constants::SCR_WIDTH, Constants::SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        PLOGE << "Framebuffer not complete!";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // We need to keep track of previous position and previous normal in order to implement SVGF
    unsigned int prevPositionTex, prevNormalTex;

    // Position
    glGenTextures(1, &prevPositionTex);
    glBindTexture(GL_TEXTURE_2D, prevPositionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
        Constants::SCR_WIDTH, Constants::SCR_HEIGHT,
        0, GL_RGB, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Normal
    glGenTextures(1, &prevNormalTex);
    glBindTexture(GL_TEXTURE_2D, prevNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
        Constants::SCR_WIDTH, Constants::SCR_HEIGHT,
        0, GL_RGB, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    // Create Ray Tracing Shadow Textures
    // We want to create multiple textures, since we need to understand the shadow created by EACH light source
    // E.g. we can't just say that if it's in the shadow of one light source then it's in shadow period, because
    // while it could be in the shadow of one light, it might be illuminated by another. 
    unsigned int gRayTracedShadowsArray;
    glGenTextures(1, &gRayTracedShadowsArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, gRayTracedShadowsArray);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R16F, Constants::SCR_WIDTH, Constants::SCR_HEIGHT, Constants::NR_LIGHTS);

    // Visibility History Array keeps track of 2 texture arrays which are used for the temporal part of SVGF
    // Each texel of a texture keeps track of the accumulated shadow visibility up to the current frame for a light
    // We need 2 in order to properly ping-pong
    unsigned int visibilityHistoryArray[2];
    glGenTextures(2, visibilityHistoryArray);

    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, visibilityHistoryArray[i]);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY,
            1,
            GL_R16F,
            Constants::SCR_WIDTH,
            Constants::SCR_HEIGHT,
            Constants::NR_LIGHTS);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // History Length Array keeps track of 2 textures which are used for the temporal part of SVGF
    // Each texel of the texture keeps track of the number of frames that have contributed to this pixel
    // We need 2 in order to properly ping-pong
    unsigned int historyLengthArray[2];
    glGenTextures(2, historyLengthArray);

    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, historyLengthArray[i]);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY,
            1,
            GL_R16F,
            Constants::SCR_WIDTH,
            Constants::SCR_HEIGHT,
            Constants::NR_LIGHTS);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Moments Array
    unsigned int momentsArray[2];
    glGenTextures(2, momentsArray);

    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, momentsArray[i]);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY,
            1,
            GL_RG16F,
            Constants::SCR_WIDTH,
            Constants::SCR_HEIGHT,
            Constants::NR_LIGHTS);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Spatial Filtered Array
    unsigned int spatialFilteredArray[2];
    glGenTextures(2, spatialFilteredArray);

    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, spatialFilteredArray[i]);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY,
            1,
            GL_R16F,
            Constants::SCR_WIDTH,
            Constants::SCR_HEIGHT,
            Constants::NR_LIGHTS);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Variables used to index arrays for ping-ponging
    // 'ping' should always be used as the previous and 'pong'
    // should always be used as the current
    int ping{ 0 }; // prev
    int pong{ 1 }; // curr

    // setting up the lights
    std::vector<glm::vec3> lightPositions{};
    std::vector<glm::vec3> lightColors{};
    
    /*
    srand(13); // TODO change to mt
    for (unsigned int i{ 0 }; i < Constants::NR_LIGHTS; ++i)
    {
        // calculate slightly random offsets
        float xPos{ static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0) };
        float yPos{ static_cast<float>(((rand() % 100) / 100.0) * 4.5 - 1.0) };
        float zPos{ static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0) };
        lightPositions.emplace_back(xPos, yPos, zPos);

        // also calculate random color
        float rColor{ static_cast<float>(((rand() % 100) / 200.0f) + 0.5) }; // between 0.5 and 1.)
        float gColor{ static_cast<float>(((rand() % 100) / 200.0f) + 0.5) }; // between 0.5 and 1.)
        float bColor{ static_cast<float>(((rand() % 100) / 200.0f) + 0.5) }; // between 0.5 and 1.)
        lightColors.emplace_back(rColor, gColor, bColor);
    }
    */

    
    lightPositions.emplace_back(0.0f, 0.05f, 2.0f);
    lightColors.emplace_back(1.0f, 1.0f, 1.0f);

    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);
    shaderLightingPass.setInt("shadowMaps", 3);

    // The previous frames MVP matrices used for temporal accumulation
    // glm::mat4 prevModel{ glm::mat4(1.0f) };
    glm::mat4 prevView{ glm::mat4(1.0f) };
    glm::mat4 prevProjection{ glm::mat4(1.0f) };

    // =================================================================================================
    // RENDER LOOP
    // =================================================================================================
    PLOGD << "Entering render loop";
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        Utility::processInput(window, renderSettings, deltaTime);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        Utility::setupImguiWindow(renderSettings);

        // geometry pass: render scene's geometry/color data into gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection{ glm::perspective(glm::radians(renderSettings.camera.Zoom), static_cast<float>(Constants::SCR_WIDTH) / static_cast<float>(Constants::SCR_HEIGHT), 0.1f, 100.0f) };
        glm::mat4 view { renderSettings.camera.GetViewMatrix() };
        glm::mat4 model { glm::mat4(1.0f) };

        if (firstFrame) {
            // We'd like to avoid any big jumps on the first frame, so we just
            // set it equal to the current MVP matrices
            prevProjection = projection;
            prevView = view;
            //prevModel = model;
        }

        shaderGeometryPass.use();
        shaderGeometryPass.setVec3("CameraPosition", renderSettings.camera.Position);
        shaderGeometryPass.setMat4("projection", projection);
        shaderGeometryPass.setMat4("view", view);
        shaderGeometryPass.setMat4("prevView", prevView);
        shaderGeometryPass.setMat4("prevProjection", prevProjection);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, crateDiffuseMap);

        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, crateSpecularMap);

        // Defines how we render the objects, see gbuffer.frag for details
        shaderGeometryPass.setInt("renderingMode", static_cast<int>(renderSettings.gBufferRenderMode));

        // Drawing the 9 boxes in the scene
        for (unsigned int i{ 0 }; i < objectPositions.size(); ++i)
        {
            shaderGeometryPass.setMat4("model", objectTransforms[i]);

            Utility::renderCube();
        }

        // Drawing the floor
        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorDiffuseMap);

        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, floorSpecularMap);

        shaderGeometryPass.setMat4("model", floorModel);

        Utility::renderFloor();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        prevView = view;
        prevProjection = projection;

        // Ray Tracer Pass
        for (unsigned int i = 0; i < Constants::NR_LIGHTS; ++i)
        {
            rayTraceShader.use();

            // bind G-buffer textures
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gPosition);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gNormal);

            // bind Blue Noise texture
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, blueNoise);

            rayTraceShader.setInt("randomSeed", static_cast<int>(rand()));

            // send uniforms for only this light
            rayTraceShader.setVec3("light.Position", lightPositions[i]);
            rayTraceShader.setVec3("light.Color", lightColors[i]);

            const float constant{ 1.0f };
            const float linear{ 0.22f };
            const float quadratic{ 0.20f };
            rayTraceShader.setFloat("light.Linear", linear);
            rayTraceShader.setFloat("light.Quadratic", quadratic);

            const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
            float maxDistance{ (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic) };
            rayTraceShader.setFloat("light.MaxDistance", maxDistance);
            rayTraceShader.setFloat("light.Radius", Constants::LIGHT_RADIUS);

            rayTraceShader.setVec3("viewPos", renderSettings.camera.Position);

            // bind shadow texture for this light
            glBindImageTexture(0, gRayTracedShadowsArray, 0, GL_FALSE, i, GL_WRITE_ONLY, GL_R16F);

            // bind triangles SSBO
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, triangleSSBO);

            // dispatch compute shader
            rayTraceShader.dispatch((Constants::SCR_WIDTH + 16 - 1) / 16, (Constants::SCR_HEIGHT + 16 - 1) / 16);

            // make sure writes are visible before next light
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        // Temporal Accumulation pass: Apply temporal SVGF
        if (renderSettings.svgfRenderMode == Settings::SVGFRenderMode::on || renderSettings.svgfRenderMode == Settings::SVGFRenderMode::temporal) {
            for (unsigned int i = 0; i < Constants::NR_LIGHTS; ++i)
            {
                temporalAccumulationShader.use();

                temporalAccumulationShader.setInt("lightIndex", i);
                temporalAccumulationShader.setInt("maxHistory", 32);
                temporalAccumulationShader.setBool("firstFrameBool", firstFrame);

                // 0 — Raw noisy shadows
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D_ARRAY, gRayTracedShadowsArray);

                // 1 — Previous accumulated visibility
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D_ARRAY, visibilityHistoryArray[ping]);

                // 2 — Previous history length
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D_ARRAY, historyLengthArray[ping]);

                // 3 — Motion and depth information
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, gMotionDepthVec);

                // 4 — Current frame positions (G-buffer)
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, gPosition);

                // 5 — Current frame normals (G-buffer)
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, gNormal);

                // 6 — Previous frame positions
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, prevPositionTex);

                // 7 — Previous frame normals
                glActiveTexture(GL_TEXTURE7);
                glBindTexture(GL_TEXTURE_2D, prevNormalTex);

                // 8 - Previous Moments
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D_ARRAY, momentsArray[ping]);

                // ----------- OUTPUTS (image stores) -----------
                // Remember, we want to write to Pong and read from Ping
                
                // 9 — Write new visibility
                glBindImageTexture(
                    9,
                    visibilityHistoryArray[pong],
                    0,
                    GL_FALSE,
                    i,
                    GL_WRITE_ONLY,
                    GL_R16F);

                // 10 — Write new history length
                glBindImageTexture(
                    10,
                    historyLengthArray[pong],
                    0,
                    GL_FALSE,
                    i,
                    GL_WRITE_ONLY,
                    GL_R16F);

                // 11 — Write new moments
                glBindImageTexture(
                    11,
                    momentsArray[pong],
                    0,
                    GL_FALSE,
                    i,
                    GL_WRITE_ONLY,
                    GL_RG16F);

                temporalAccumulationShader.dispatch(
                    (Constants::SCR_WIDTH + 16 - 1) / 16,
                    (Constants::SCR_HEIGHT + 16 - 1) / 16
                );

                // make sure writes are visible before next light
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            }
        }
        
        // Spatial Filtering (SVGF A-Trous)
        if (renderSettings.svgfRenderMode == Settings::SVGFRenderMode::on || renderSettings.svgfRenderMode == Settings::SVGFRenderMode::spatial) {
            for (unsigned int i = 0; i < Constants::NR_LIGHTS; ++i)
            {
                int readIndex = ping;
                int writeIndex = pong;

                for (int pass = 0; pass < 5; ++pass)
                {
                    spatialFilteringShader.use();

                    int stepWidth = 1 << pass;

                    spatialFilteringShader.setInt("lightIndex", i);
                    spatialFilteringShader.setInt("stepWidth", stepWidth);
                    spatialFilteringShader.setFloat("phiColor", 1.0f);
                    spatialFilteringShader.setFloat("phiNormal", 32.0f);
                    spatialFilteringShader.setFloat("phiDepth", 1.0f);

                    // binding = 0
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D_ARRAY,
                        (pass == 0) ? visibilityHistoryArray[pong]
                        : spatialFilteredArray[readIndex]);

                    // binding = 1
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, momentsArray[pong]);

                    // binding = 2
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, gPosition);

                    // binding = 3
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, gNormal);

                    // binding = 4
                    glBindImageTexture(4,
                        spatialFilteredArray[writeIndex],
                        0,
                        GL_FALSE,
                        i,
                        GL_WRITE_ONLY,
                        GL_R16F);

                    spatialFilteringShader.dispatch(
                        (Constants::SCR_WIDTH + 16 - 1) / 16,
                        (Constants::SCR_HEIGHT + 16 - 1) / 16);

                    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

                    std::swap(readIndex, writeIndex);
                }
            }
        }

        // lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderLightingPass.use();

        // bind g buffer positions
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);

        // bind g buffer normals
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);

        // bind g buffer albedo + spec
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

        // Defines how we render the objects, see deferred_shading.frag for details
        shaderLightingPass.setInt("renderingMode", static_cast<int>(renderSettings.deferredShadingRenderMode));

        // send light relevant uniforms
        for (unsigned int i{ 0 }; i < lightPositions.size(); ++i)
        {
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);

            // update attenuation parameters and calculate radius
            const float constant{ 1.0f };
            const float linear{ 0.22f };
            const float quadratic{ 0.20f };
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);

            // then calculate radius of light volume/sphere
            const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
            float maxDistance{ (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic) };
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].MaxDistance", maxDistance);
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Radius", Constants::LIGHT_RADIUS);

            // bind ray tracer image
            glActiveTexture(GL_TEXTURE3);

            if (renderSettings.svgfRenderMode == Settings::SVGFRenderMode::off) {
                glBindTexture(GL_TEXTURE_2D_ARRAY, gRayTracedShadowsArray);
            }
            else if (renderSettings.svgfRenderMode == Settings::SVGFRenderMode::temporal) {
                glBindTexture(GL_TEXTURE_2D_ARRAY, visibilityHistoryArray[ping]);
            } else {
                glBindTexture(GL_TEXTURE_2D_ARRAY, spatialFilteredArray[ping]);
            }
        }

        shaderLightingPass.setVec3("viewPos", renderSettings.camera.Position);

        // finally render quad
        Utility::renderQuad();

        // copy content of geometry's depth buffer to default framebuffer's depth buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer

        // blit to default framebuffer
        glBlitFramebuffer(0, 0, Constants::SCR_WIDTH, Constants::SCR_HEIGHT, 0, 0, Constants::SCR_WIDTH, Constants::SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // render lights on top of scene
        shaderLightBox.use();
        shaderLightBox.setMat4("projection", projection);
        shaderLightBox.setMat4("view", view);

        for (unsigned int i{ 0 }; i < lightPositions.size(); ++i)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPositions[i]);
            model = glm::scale(model, glm::vec3(0.125f));
            shaderLightBox.setMat4("model", model);
            shaderLightBox.setVec3("lightColor", lightColors[i]);
            Utility::renderCube();
        }

        // Copy current position/normal information into previous textures
        glCopyImageSubData(
            gPosition, GL_TEXTURE_2D, 0, 0, 0, 0,
            prevPositionTex, GL_TEXTURE_2D, 0, 0, 0, 0,
            Constants::SCR_WIDTH, Constants::SCR_HEIGHT, 1);

        glCopyImageSubData(
            gNormal, GL_TEXTURE_2D, 0, 0, 0, 0,
            prevNormalTex, GL_TEXTURE_2D, 0, 0, 0, 0,
            Constants::SCR_WIDTH, Constants::SCR_HEIGHT, 1);

        std::swap(ping, pong);

        // Render Dear ImGui Menu
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        if (firstRenderPass) {
            PLOGD << "Num Triangles in Scene: " << gpuTriangles.size();
            firstRenderPass = false;
        }

        if (firstFrame) {
            firstFrame = false;
        }

    }
    PLOGD << "Render Loop Terminated";

    PLOGD << "Initiating Shutdown";
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwTerminate();
    PLOGD << "Successfully Shutdown";
	return 0;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);

    if (!renderSettings.enableMouseLook || ImGui::GetIO().WantCaptureMouse)
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    float& lastX = renderSettings.lastX;
    float& lastY = renderSettings.lastY;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    renderSettings.camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    renderSettings.camera.ProcessMouseScroll(static_cast<float>(yoffset));
}