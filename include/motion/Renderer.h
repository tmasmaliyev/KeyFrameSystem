#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

// Forward declaration
class Mesh;

// Renderer class to handle OpenGL rendering
class Renderer
{
private:
    unsigned int shaderProgram;

    // Camera state
    float cameraDistance;
    float cameraYaw;
    float cameraPitch;
    glm::vec3 cameraTarget;

    // Mouse state
    bool firstMouse;
    bool mousePressed;
    float lastX, lastY;

    // Window dimensions
    int windowWidth, windowHeight;

    // Shader compilation and loading
    std::string loadShaderFromFile(const std::string &filepath);
    unsigned int compileShader(const std::string &source, GLenum type);
    unsigned int loadShadersFromFiles(const std::string &vertexPath, const std::string &fragmentPath);
    void setupShaders();

public:
    Renderer();
    ~Renderer();

    bool initialize();
    bool initializeWithShaderFiles(const std::string &vertexPath, const std::string &fragmentPath);
    void cleanup();

    // Camera controls
    glm::vec3 getCameraPosition() const;
    void resetCamera();

    // Event callbacks
    void onFramebufferSize(int width, int height);
    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action);
    void onScroll(double yoffset);

    // Rendering
    void clear();
    void renderMesh(Mesh *mesh, const glm::mat4 &model);

    // Getters
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
};

// Default shader source code (fallback if files not found)
extern const char *defaultVertexShaderSource;
extern const char *defaultFragmentShaderSource;

#endif // RENDERER_H