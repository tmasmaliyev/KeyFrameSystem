#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <iomanip>

#include "motion/Mesh.h"
#include "motion/MotionController.h"
#include "motion/Renderer.h"
#include "motion/Utils.h"

// Global state
OptimizedMotionController *motionController = nullptr;
Mesh *currentMesh = nullptr;
Renderer *renderer = nullptr;

// Animation state
float currentTime = 0.0f;
bool useQuaternions = true;
bool useBSpline = false;

// Performance monitoring
bool showPerformanceStats = false;
float frameTime = 0.0f;
int frameCount = 0;
float fpsTimer = 0.0f;
float averageFPS = 0.0f;

// Configuration
ProgramConfig config;

// GLFW callback wrappers
void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    if (renderer)
        renderer->onFramebufferSize(width, height);
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    if (renderer)
        renderer->onMouseMove(xpos, ypos);
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (renderer)
        renderer->onMouseButton(button, action);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (renderer)
        renderer->onScroll(yoffset);
}

void keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_Q:
            useQuaternions = !useQuaternions;
            std::cout << "Using " << (useQuaternions ? "Quaternions" : "Euler Angles") << std::endl;
            break;
        case GLFW_KEY_S:
            useBSpline = !useBSpline;
            std::cout << "Using " << (useBSpline ? "B-Spline" : "Catmull-Rom") << " interpolation" << std::endl;
            break;
        case GLFW_KEY_R:
            currentTime = 0.0f;
            std::cout << "Reset animation" << std::endl;
            break;
        case GLFW_KEY_C:
            if (renderer)
            {
                renderer->resetCamera();
                std::cout << "Reset camera" << std::endl;
            }
            break;
        case GLFW_KEY_P:
            showPerformanceStats = !showPerformanceStats;
            std::cout << "Performance stats: " << (showPerformanceStats ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_L:
        {
            std::cout << "Enter OBJ file path: ";
            std::string filename;
            std::getline(std::cin, filename);
            if (!filename.empty())
            {
                if (loadMeshFromOBJ(filename, &currentMesh))
                {
                    std::cout << "Successfully loaded: " << filename << std::endl;
                }
                else
                {
                    std::cout << "Failed to load: " << filename << std::endl;
                }
            }
        }
        break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        }
    }
}

void render()
{
    if (!renderer || !motionController || !currentMesh)
        return;

    // Performance timing
    auto renderStart = std::chrono::high_resolution_clock::now();

    renderer->clear();

    // Get the transformation matrix from motion controller with timing
    auto transformStart = std::chrono::high_resolution_clock::now();
    glm::mat4 model = motionController->getTransformationMatrix(currentTime, useQuaternions, useBSpline);
    auto transformEnd = std::chrono::high_resolution_clock::now();

    if (showPerformanceStats)
    {
        auto transformDuration = std::chrono::duration_cast<std::chrono::microseconds>(transformEnd - transformStart);
        if (transformDuration.count() > 50)
        { // Only show if > 50 microseconds
            std::cout << "Transform calculation: " << transformDuration.count() << "µs" << std::endl;
        }
    }

    // Render mesh
    renderer->renderMesh(currentMesh, model);

    // Performance timing
    auto renderEnd = std::chrono::high_resolution_clock::now();
    if (showPerformanceStats)
    {
        auto renderDuration = std::chrono::duration_cast<std::chrono::microseconds>(renderEnd - renderStart);
        frameTime = renderDuration.count() / 1000.0f; // Convert to milliseconds
    }
}

bool initializeSystem()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return true;
}

GLFWwindow *createWindow()
{
    GLFWwindow *window = glfwCreateWindow(800, 600, "Key-frame Motion Control System (Optimized)", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    return window;
}

bool setupGraphics()
{
    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    // Initialize renderer with custom shader paths from configuration
    renderer = new Renderer();

    std::cout << "Loading shaders:" << std::endl;
    std::cout << "  Vertex shader: " << config.vertexShaderPath << std::endl;
    std::cout << "  Fragment shader: " << config.fragmentShaderPath << std::endl;

    if (!renderer->initializeWithShaderFiles(config.vertexShaderPath, config.fragmentShaderPath))
    {
        std::cerr << "Failed to initialize renderer with shader files:" << std::endl;
        std::cerr << "  Vertex: " << config.vertexShaderPath << std::endl;
        std::cerr << "  Fragment: " << config.fragmentShaderPath << std::endl;
        std::cerr << "Please check that these files exist and are valid GLSL shaders." << std::endl;
        return false;
    }

    std::cout << "Successfully initialized renderer with custom shaders" << std::endl;
    return true;
}

void setupMotionSystem()
{
    // Initialize motion controller
    motionController = new OptimizedMotionController();

    if (config.keyframesProvided)
    {
        if (!parseKeyFramesFromString(config.keyframeString, motionController))
        {
            std::cout << "Failed to parse keyframes, using defaults" << std::endl;
            setupDefaultKeyFrames(motionController);
        }
    }
    else
    {
        setupDefaultKeyFrames(motionController);
    }
}

void setupMesh()
{
    // Load mesh - check for teapot.obj first if no model specified
    if (config.objFilename.empty())
    {
        // Try to load teapot.obj if it exists
        std::ifstream teapotFile("teapot.obj");
        if (teapotFile.good())
        {
            config.objFilename = "teapot.obj";
            teapotFile.close();
        }
    }

    if (!config.objFilename.empty())
    {
        if (loadMeshFromOBJ(config.objFilename, &currentMesh))
        {
            std::cout << "Successfully loaded model: " << config.objFilename << std::endl;
        }
        else
        {
            std::cout << "Failed to load model, using default cube" << std::endl;
            currentMesh = createDefaultCube();
            currentMesh->setupBuffers();
        }
    }
    else
    {
        std::cout << "Using default cube" << std::endl;
        currentMesh = createDefaultCube();
        currentMesh->setupBuffers();
    }
}

void printSystemInfo()
{
    std::cout << "Key-framing Motion Control System (Optimized)" << std::endl;
    std::cout << "Settings:" << std::endl;
    std::cout << "  Orientation: " << (config.useQuaternions ? "Quaternions" : "Euler Angles") << std::endl;
    std::cout << "  Interpolation: " << (config.useBSpline ? "B-Spline" : "Catmull-Rom") << std::endl;
    std::cout << "  Vertex Shader: " << config.vertexShaderPath << std::endl;
    std::cout << "  Fragment Shader: " << config.fragmentShaderPath << std::endl;

    if (!config.objFilename.empty())
    {
        std::cout << "  Model: " << config.objFilename << std::endl;
    }

    if (config.keyframesProvided)
    {
        std::cout << "  Custom keyframes provided" << std::endl;
    }

    std::cout << "\nInteractive Controls:" << std::endl;
    std::cout << "Q - Toggle Quaternions (" << (useQuaternions ? "ON" : "OFF") << ") / Euler Angles (" << (!useQuaternions ? "ON" : "OFF") << ")" << std::endl;
    std::cout << "S - Toggle B-Spline (" << (useBSpline ? "ON" : "OFF") << ") / Catmull-Rom (" << (!useBSpline ? "ON" : "OFF") << ")" << std::endl;
    std::cout << "R - Reset animation" << std::endl;
    std::cout << "C - Reset camera" << std::endl;
    std::cout << "P - Toggle performance stats" << std::endl;
    std::cout << "L - Load OBJ file (interactive)" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "\nCamera Controls:" << std::endl;
    std::cout << "Mouse Drag - Rotate camera" << std::endl;
    std::cout << "Mouse Wheel - Zoom in/out" << std::endl;
    std::cout << "Window Resize - Adjusts viewport" << std::endl;

    if (motionController)
    {
        std::cout << "\nLoaded " << motionController->getKeyFrameCount() << " keyframes" << std::endl;
        std::cout << "Animation duration: " << motionController->getTotalTime() << " seconds" << std::endl;
    }
}

void cleanup()
{
    if (currentMesh)
    {
        delete currentMesh;
        currentMesh = nullptr;
    }

    if (renderer)
    {
        delete renderer;
        renderer = nullptr;
    }

    if (motionController)
    {
        delete motionController;
        motionController = nullptr;
    }
}

void mainLoop(GLFWwindow *window)
{
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update animation time with fixed speed
        currentTime += deltaTime * 0.5f; // Fixed animation speed
        if (motionController && currentTime > motionController->getTotalTime())
        {
            currentTime = 0.0f; // Loop animation
        }

        // Update performance stats
        if (showPerformanceStats)
        {
            frameCount++;
            fpsTimer += deltaTime;
            if (fpsTimer >= 1.0f)
            {
                averageFPS = frameCount / fpsTimer;
                std::cout << "FPS: " << std::fixed << std::setprecision(1) << averageFPS
                          << " | Frame time: " << std::setprecision(2) << frameTime << "ms" << std::endl;
                frameCount = 0;
                fpsTimer = 0.0f;
            }
        }

        // Poll events
        glfwPollEvents();

        // Render
        render();

        // Swap buffers
        glfwSwapBuffers(window);
    }
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (!parseCommandLine(argc, argv, config))
    {
        return -1;
    }

    if (config.showHelp)
    {
        printHelp(argv[0]);
        return 0;
    }

    // Apply configuration
    useQuaternions = config.useQuaternions;
    useBSpline = config.useBSpline;

    // Initialize systems
    if (!initializeSystem())
    {
        return -1;
    }

    // Create window
    GLFWwindow *window = createWindow();
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Setup graphics (this now uses the shader paths from config)
    if (!setupGraphics())
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Setup motion system and mesh
    setupMotionSystem();
    setupMesh();

    // Print system information
    printSystemInfo();

    // Main application loop
    mainLoop(window);

    // Cleanup
    cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}