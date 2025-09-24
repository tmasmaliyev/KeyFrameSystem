#include "motion/Renderer.h"
#include "motion/Mesh.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>

Renderer::Renderer() 
    : shaderProgram(0)
    , cameraDistance(8.0f)
    , cameraYaw(45.0f)
    , cameraPitch(35.0f)
    , cameraTarget(0.0f, 0.0f, 0.0f)
    , firstMouse(true)
    , mousePressed(false)
    , lastX(400.0f)
    , lastY(300.0f)
    , windowWidth(800)
    , windowHeight(600)
{
}

Renderer::~Renderer()
{
    cleanup();
}

std::string Renderer::loadShaderFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Failed to open shader file: " << filepath << std::endl;
        std::cerr << "Please ensure the file exists and is readable." << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string content = buffer.str();
    if (content.empty())
    {
        std::cerr << "WARNING: Shader file is empty: " << filepath << std::endl;
        return "";
    }
    
    std::cout << "Successfully loaded shader file: " << filepath << " (" << content.length() << " characters)" << std::endl;
    return content;
}

unsigned int Renderer::compileShader(const std::string& source, GLenum type)
{
    if (source.empty())
    {
        std::cerr << "ERROR: Cannot compile empty shader source" << std::endl;
        return 0;
    }

    const char* src = source.c_str();
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        const char* shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << "ERROR::SHADER::" << shaderType << "::COMPILATION_FAILED" << std::endl;
        std::cerr << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    std::cout << "Successfully compiled " << ((type == GL_VERTEX_SHADER) ? "vertex" : "fragment") << " shader" << std::endl;
    return shader;
}

unsigned int Renderer::loadShadersFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::cout << "Loading shaders from files..." << std::endl;
    std::cout << "  Vertex shader: " << vertexPath << std::endl;
    std::cout << "  Fragment shader: " << fragmentPath << std::endl;

    // Load shader source code from files
    std::string vertexSource = loadShaderFromFile(vertexPath);
    std::string fragmentSource = loadShaderFromFile(fragmentPath);
    
    // Check if both files were loaded successfully
    if (vertexSource.empty())
    {
        std::cerr << "ERROR: Failed to load vertex shader from: " << vertexPath << std::endl;
        return 0;
    }
    
    if (fragmentSource.empty())
    {
        std::cerr << "ERROR: Failed to load fragment shader from: " << fragmentPath << std::endl;
        return 0;
    }
    
    // Compile shaders
    unsigned int vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    if (vertexShader == 0 || fragmentShader == 0)
    {
        std::cerr << "ERROR: Shader compilation failed" << std::endl;
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return 0;
    }
    
    // Create program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // Check linking
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << std::endl;
        std::cerr << infoLog << std::endl;
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }
    
    // Clean up individual shaders (they're now part of the program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    std::cout << "Successfully created shader program" << std::endl;
    return program;
}

void Renderer::setupShaders()
{
    // Always try to load from files - no fallbacks
    shaderProgram = loadShadersFromFiles("shaders/vertex.glsl", "shaders/fragment.glsl");
    
    if (shaderProgram == 0)
    {
        std::cerr << "CRITICAL ERROR: Failed to load shaders!" << std::endl;
        std::cerr << "Please ensure the following files exist:" << std::endl;
        std::cerr << "  - shaders/vertex.glsl" << std::endl;
        std::cerr << "  - shaders/fragment.glsl" << std::endl;
    }
}

bool Renderer::initialize()
{
    std::cout << "Initializing renderer..." << std::endl;
    
    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Setup shaders from files
    setupShaders();
    
    if (shaderProgram == 0)
    {
        std::cerr << "ERROR: Renderer initialization failed - could not load shaders" << std::endl;
        return false;
    }
    
    std::cout << "Renderer initialized successfully" << std::endl;
    return true;
}

bool Renderer::initializeWithShaderFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::cout << "Initializing renderer with custom shaders..." << std::endl;
    
    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Load custom shaders
    shaderProgram = loadShadersFromFiles(vertexPath, fragmentPath);
    
    if (shaderProgram == 0)
    {
        std::cerr << "ERROR: Renderer initialization failed - could not load custom shaders" << std::endl;
        return false;
    }
    
    std::cout << "Renderer initialized successfully with custom shaders" << std::endl;
    return true;
}

void Renderer::cleanup()
{
    if (shaderProgram != 0)
    {
        std::cout << "Cleaning up shader program" << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

glm::vec3 Renderer::getCameraPosition() const
{
    float x = cameraDistance * cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
    float y = cameraDistance * sin(glm::radians(cameraPitch));
    float z = cameraDistance * cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));
    return cameraTarget + glm::vec3(x, y, z);
}

void Renderer::resetCamera()
{
    cameraDistance = 8.0f;
    cameraYaw = 45.0f;
    cameraPitch = 35.0f;
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    std::cout << "Camera reset to default position" << std::endl;
}

void Renderer::onFramebufferSize(int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    std::cout << "Framebuffer resized to: " << width << "x" << height << std::endl;
}

void Renderer::onMouseMove(double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    if (mousePressed)
    {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.5f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        cameraYaw += xoffset;
        cameraPitch += yoffset;

        // Constrain pitch
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
    }
    else
    {
        lastX = xpos;
        lastY = ypos;
    }
}

void Renderer::onMouseButton(int button, int action)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        mousePressed = (action == GLFW_PRESS);
    }
}

void Renderer::onScroll(double yoffset)
{
    cameraDistance -= yoffset * 0.5f;
    if (cameraDistance < 1.0f)
        cameraDistance = 1.0f;
    if (cameraDistance > 50.0f)
        cameraDistance = 50.0f;
}

void Renderer::clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderMesh(Mesh* mesh, const glm::mat4& model)
{
    if (!mesh)
    {
        std::cerr << "WARNING: Attempting to render null mesh" << std::endl;
        return;
    }
    
    if (shaderProgram == 0)
    {
        std::cerr << "ERROR: Cannot render - no valid shader program" << std::endl;
        return;
    }
    
    glUseProgram(shaderProgram);

    // Calculate camera position
    glm::vec3 cameraPos = getCameraPosition();

    // Set up view and projection matrices with current window aspect ratio
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0, 1, 0));
    float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Lighting uniforms (light position relative to camera for better visibility)
    glm::vec3 lightPos = cameraPos + glm::vec3(2.0f, 2.0f, 2.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.8f, 0.4f, 0.2f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

    // Render mesh
    mesh->render();
}