#define GLM_ENABLE_EXPERIMENTAL

// Imports
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>

// Forward declarations
struct KeyFrame;
struct Mesh;
class OptimizedMotionController;
class OBJLoader;
void renderMesh();
void setupShaders();

// Global variables
unsigned int shaderProgram;
unsigned int VAO, VBO, EBO;
OptimizedMotionController *motionController;
Mesh *currentMesh;
float currentTime = 0.0f;
bool useQuaternions = true;
bool useBSpline = false;
std::string objFilename = "";

// Camera variables
float cameraDistance = 8.0f;
float cameraYaw = 45.0f;
float cameraPitch = 35.0f;
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
bool firstMouse = true;
bool mousePressed = false;
float lastX = 400.0f, lastY = 300.0f;

// Window size for proper aspect ratio
int windowWidth = 800;
int windowHeight = 600;

// Performance monitoring
bool showPerformanceStats = false;
float frameTime = 0.0f;
int frameCount = 0;
float fpsTimer = 0.0f;
float averageFPS = 0.0f;

// Mesh structure
struct Mesh
{
    std::vector<float> vertices; // Interleaved: position(3) + normal(3) + texcoord(2)
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    Mesh() : VAO(0), VBO(0), EBO(0) {}

    ~Mesh()
    {
        cleanup();
    }

    void cleanup()
    {
        if (VAO != 0)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            if (EBO != 0)
                glDeleteBuffers(1, &EBO);
            VAO = VBO = EBO = 0;
        }
    }

    void setupBuffers()
    {
        cleanup();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        // Normal attribute (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute (location = 2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        if (!indices.empty())
        {
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
    }

    void render()
    {
        glBindVertexArray(VAO);
        if (!indices.empty())
        {
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 8);
        }
        glBindVertexArray(0);
    }
};

// OBJ Loader class
class OBJLoader
{
private:
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    // Calculate normals for faces that don't have them
    glm::vec3 calculateNormal(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2)
    {
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        return glm::normalize(glm::cross(edge1, edge2));
    }

public:
    bool loadOBJ(const std::string &path, Mesh &mesh)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open OBJ file: " << path << std::endl;
            return false;
        }

        temp_vertices.clear();
        temp_uvs.clear();
        temp_normals.clear();
        mesh.vertices.clear();
        mesh.indices.clear();

        std::string line;
        std::vector<std::vector<int>> vertexIndices, uvIndices, normalIndices;
        bool hasNormals = false;
        bool hasUVs = false;

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v")
            {
                glm::vec3 vertex;
                iss >> vertex.x >> vertex.y >> vertex.z;
                temp_vertices.push_back(vertex);
            }
            else if (prefix == "vt")
            {
                glm::vec2 uv;
                iss >> uv.x >> uv.y;
                temp_uvs.push_back(uv);
                hasUVs = true;
            }
            else if (prefix == "vn")
            {
                glm::vec3 normal;
                iss >> normal.x >> normal.y >> normal.z;
                temp_normals.push_back(normal);
                hasNormals = true;
            }
            else if (prefix == "f")
            {
                std::vector<int> vIndices, uvIdx, nIdx;
                std::string vertex;

                while (iss >> vertex)
                {
                    std::replace(vertex.begin(), vertex.end(), '/', ' ');
                    std::istringstream viss(vertex);

                    int vi, uvi = 0, ni = 0;
                    viss >> vi;
                    if (viss >> uvi)
                    {
                        if (viss >> ni)
                        {
                            // Format: v/vt/vn
                        }
                        // Format: v/vt or v/vt/vn
                    }
                    else
                    {
                        // Format: v or v//vn
                        std::istringstream checkNormal(vertex);
                        std::string temp;
                        checkNormal >> temp >> temp >> ni;
                    }

                    vIndices.push_back(vi - 1); // OBJ indices are 1-based
                    uvIdx.push_back(uvi > 0 ? uvi - 1 : 0);
                    nIdx.push_back(ni > 0 ? ni - 1 : 0);
                }

                // Triangulate faces (assuming they are quads or triangles)
                for (size_t i = 1; i < vIndices.size() - 1; i++)
                {
                    vertexIndices.push_back({vIndices[0], vIndices[i], vIndices[i + 1]});
                    uvIndices.push_back({uvIdx[0], uvIdx[i], uvIdx[i + 1]});
                    normalIndices.push_back({nIdx[0], nIdx[i], nIdx[i + 1]});
                }
            }
        }

        file.close();

        if (temp_vertices.empty())
        {
            std::cerr << "No vertices found in OBJ file" << std::endl;
            return false;
        }

        // Process faces
        for (size_t i = 0; i < vertexIndices.size(); i++)
        {
            glm::vec3 faceNormal;

            if (!hasNormals)
            {
                // Calculate face normal
                faceNormal = calculateNormal(
                    temp_vertices[vertexIndices[i][0]],
                    temp_vertices[vertexIndices[i][1]],
                    temp_vertices[vertexIndices[i][2]]);
            }

            for (int j = 0; j < 3; j++)
            {
                int vIdx = vertexIndices[i][j];
                int uvIdx = hasUVs ? uvIndices[i][j] : 0;
                int nIdx = hasNormals ? normalIndices[i][j] : 0;

                // Add vertex position
                mesh.vertices.push_back(temp_vertices[vIdx].x);
                mesh.vertices.push_back(temp_vertices[vIdx].y);
                mesh.vertices.push_back(temp_vertices[vIdx].z);

                // Add normal
                if (hasNormals && nIdx < static_cast<int>(temp_normals.size()))
                {
                    mesh.vertices.push_back(temp_normals[nIdx].x);
                    mesh.vertices.push_back(temp_normals[nIdx].y);
                    mesh.vertices.push_back(temp_normals[nIdx].z);
                }
                else
                {
                    mesh.vertices.push_back(faceNormal.x);
                    mesh.vertices.push_back(faceNormal.y);
                    mesh.vertices.push_back(faceNormal.z);
                }

                // Add texture coordinates
                if (hasUVs && uvIdx < static_cast<int>(temp_uvs.size()))
                {
                    mesh.vertices.push_back(temp_uvs[uvIdx].x);
                    mesh.vertices.push_back(temp_uvs[uvIdx].y);
                }
                else
                {
                    mesh.vertices.push_back(0.0f);
                    mesh.vertices.push_back(0.0f);
                }
            }
        }

        std::cout << "Loaded OBJ file: " << path << std::endl;
        std::cout << "Vertices: " << temp_vertices.size() << std::endl;
        std::cout << "Faces: " << vertexIndices.size() << std::endl;
        std::cout << "Has normals: " << (hasNormals ? "Yes" : "No (calculated)") << std::endl;
        std::cout << "Has UVs: " << (hasUVs ? "Yes" : "No") << std::endl;

        return true;
    }
};

// Keyframe structure
struct KeyFrame
{
    glm::vec3 position;
    glm::vec3 eulerAngles; // In degrees
    glm::quat quaternion;
    float time;

    KeyFrame(glm::vec3 pos, glm::vec3 euler, float t)
        : position(pos), eulerAngles(euler), time(t)
    {
        // Convert Euler angles to quaternion
        quaternion = glm::quat(glm::radians(euler));
    }

    KeyFrame(glm::vec3 pos, glm::quat quat, float t)
        : position(pos), quaternion(quat), time(t)
    {
        // Convert quaternion to Euler angles
        eulerAngles = glm::degrees(glm::eulerAngles(quat));
    }
};

// Optimized Motion Controller
class OptimizedMotionController
{
private:
    std::vector<KeyFrame> keyframes;

    // Cache for optimization
    mutable int lastSegment = 0;
    mutable float lastTime = -1.0f;
    mutable glm::mat4 cachedTransform = glm::mat4(1.0f);

    // Pre-computed values for segments
    struct SegmentData
    {
        glm::vec3 p0, p1, p2, p3; // Control points for position
        glm::vec3 e0, e1, e2, e3; // Control points for euler angles
        glm::quat q1, q2;         // Quaternions for this segment
        float startTime, endTime;
        float duration;
    };

    mutable std::vector<SegmentData> segments;
    mutable bool segmentsCached = false;

    void precomputeSegments() const
    {
        if (segmentsCached || keyframes.size() < 2)
            return;

        segments.clear();
        segments.reserve(keyframes.size() - 1);

        for (size_t i = 0; i < keyframes.size() - 1; i++)
        {
            SegmentData seg;

            // Position control points
            seg.p0 = keyframes[std::max(0, (int)i - 1)].position;
            seg.p1 = keyframes[i].position;
            seg.p2 = keyframes[i + 1].position;
            seg.p3 = keyframes[std::min(i + 2, keyframes.size() - 1)].position;

            // Euler control points with angle normalization
            seg.e0 = keyframes[std::max(0, (int)i - 1)].eulerAngles;
            seg.e1 = keyframes[i].eulerAngles;
            seg.e2 = keyframes[i + 1].eulerAngles;
            seg.e3 = keyframes[std::min(i + 2, keyframes.size() - 1)].eulerAngles;

            // Normalize angles to avoid discontinuities
            seg.e0 = normalizeAngles(seg.e0, seg.e1);
            seg.e2 = normalizeAngles(seg.e2, seg.e1);
            seg.e3 = normalizeAngles(seg.e3, seg.e2);

            // Quaternions
            seg.q1 = keyframes[i].quaternion;
            seg.q2 = keyframes[i + 1].quaternion;

            // Time data
            seg.startTime = keyframes[i].time;
            seg.endTime = keyframes[i + 1].time;
            seg.duration = seg.endTime - seg.startTime;

            segments.push_back(seg);
        }

        segmentsCached = true;
    }

    // Fast segment finder using cached last position
    int findSegment(float time) const
    {
        if (keyframes.size() < 2)
            return 0;

        // Check if we're still in the same segment
        if (lastSegment < static_cast<int>(segments.size()) &&
            time >= segments[lastSegment].startTime &&
            time <= segments[lastSegment].endTime)
        {
            return lastSegment;
        }

        // Check adjacent segments first (common case)
        if (lastSegment + 1 < static_cast<int>(segments.size()) &&
            time >= segments[lastSegment + 1].startTime &&
            time <= segments[lastSegment + 1].endTime)
        {
            return lastSegment + 1;
        }

        if (lastSegment > 0 &&
            time >= segments[lastSegment - 1].startTime &&
            time <= segments[lastSegment - 1].endTime)
        {
            return lastSegment - 1;
        }

        // Binary search for distant segments
        int left = 0, right = segments.size() - 1;
        while (left <= right)
        {
            int mid = (left + right) / 2;
            if (time < segments[mid].startTime)
            {
                right = mid - 1;
            }
            else if (time > segments[mid].endTime)
            {
                left = mid + 1;
            }
            else
            {
                return mid;
            }
        }

        return std::max(0, std::min(right, static_cast<int>(segments.size()) - 1));
    }

    // Optimized spline calculations with precomputed control points
    glm::vec3 fastCatmullRomPosition(float t, const SegmentData &seg) const
    {
        float t2 = t * t;
        float t3 = t2 * t;

        return 0.5f * (2.0f * seg.p1 +
                       (-seg.p0 + seg.p2) * t +
                       (2.0f * seg.p0 - 5.0f * seg.p1 + 4.0f * seg.p2 - seg.p3) * t2 +
                       (-seg.p0 + 3.0f * seg.p1 - 3.0f * seg.p2 + seg.p3) * t3);
    }

    glm::vec3 fastCatmullRomEuler(float t, const SegmentData &seg) const
    {
        float t2 = t * t;
        float t3 = t2 * t;

        return 0.5f * (2.0f * seg.e1 +
                       (-seg.e0 + seg.e2) * t +
                       (2.0f * seg.e0 - 5.0f * seg.e1 + 4.0f * seg.e2 - seg.e3) * t2 +
                       (-seg.e0 + 3.0f * seg.e1 - 3.0f * seg.e2 + seg.e3) * t3);
    }

    glm::vec3 fastBSplinePosition(float t, const SegmentData &seg) const
    {
        float t2 = t * t;
        float t3 = t2 * t;

        return (1.0f / 6.0f) * ((-t3 + 3.0f * t2 - 3.0f * t + 1.0f) * seg.p0 +
                                (3.0f * t3 - 6.0f * t2 + 4.0f) * seg.p1 +
                                (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * seg.p2 +
                                t3 * seg.p3);
    }

    glm::vec3 fastBSplineEuler(float t, const SegmentData &seg) const
    {
        float t2 = t * t;
        float t3 = t2 * t;

        return (1.0f / 6.0f) * ((-t3 + 3.0f * t2 - 3.0f * t + 1.0f) * seg.e0 +
                                (3.0f * t3 - 6.0f * t2 + 4.0f) * seg.e1 +
                                (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * seg.e2 +
                                t3 * seg.e3);
    }

    glm::vec3 normalizeAngles(glm::vec3 angles, glm::vec3 reference) const
    {
        glm::vec3 result = angles;
        for (int i = 0; i < 3; i++)
        {
            while (result[i] - reference[i] > 180.0f)
                result[i] -= 360.0f;
            while (result[i] - reference[i] < -180.0f)
                result[i] += 360.0f;
        }
        return result;
    }

public:
    void addKeyFrame(const KeyFrame &kf)
    {
        keyframes.push_back(kf);
        segmentsCached = false; // Invalidate cache
        lastTime = -1.0f;       // Force recalculation
    }

    void addMultipleKeyFrames(const std::vector<KeyFrame> &kfs)
    {
        keyframes.reserve(keyframes.size() + kfs.size());
        for (const auto &kf : kfs)
        {
            keyframes.push_back(kf);
        }
        segmentsCached = false; // Invalidate cache once
        lastTime = -1.0f;       // Force recalculation
    }

    void clearKeyFrames()
    {
        keyframes.clear();
        segments.clear();
        segmentsCached = false;
        lastSegment = 0;
        lastTime = -1.0f;
    }

    glm::mat4 getTransformationMatrix(float time, bool useQuat, bool useBSplines) const
    {
        // Early return for empty keyframes
        if (keyframes.empty())
            return glm::mat4(1.0f);

        // Cache check - if time hasn't changed much, return cached result
        const float TIME_EPSILON = 0.0001f;
        if (std::abs(time - lastTime) < TIME_EPSILON && lastTime >= 0.0f)
        {
            return cachedTransform;
        }

        // Single keyframe case
        if (keyframes.size() == 1)
        {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), keyframes[0].position);
            glm::mat4 rotation = useQuat ? glm::mat4_cast(keyframes[0].quaternion) : glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(keyframes[0].eulerAngles.x), glm::vec3(1, 0, 0)), glm::radians(keyframes[0].eulerAngles.y), glm::vec3(0, 1, 0)), glm::radians(keyframes[0].eulerAngles.z), glm::vec3(0, 0, 1));
            lastTime = time;
            return cachedTransform = translation * rotation;
        }

        // Precompute segments if needed
        precomputeSegments();

        // Find current segment
        int currentSegment = findSegment(time);
        lastSegment = currentSegment;

        // Handle end of animation
        if (currentSegment >= static_cast<int>(segments.size()))
        {
            const KeyFrame &lastKF = keyframes.back();
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), lastKF.position);
            glm::mat4 rotation = useQuat ? glm::mat4_cast(lastKF.quaternion) : glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(lastKF.eulerAngles.x), glm::vec3(1, 0, 0)), glm::radians(lastKF.eulerAngles.y), glm::vec3(0, 1, 0)), glm::radians(lastKF.eulerAngles.z), glm::vec3(0, 0, 1));
            lastTime = time;
            return cachedTransform = translation * rotation;
        }

        const SegmentData &seg = segments[currentSegment];

        // Calculate interpolation parameter
        float t = (seg.duration > 0.0f) ? glm::clamp((time - seg.startTime) / seg.duration, 0.0f, 1.0f) : 0.0f;

        // Interpolate position
        glm::vec3 position = useBSplines ? fastBSplinePosition(t, seg) : fastCatmullRomPosition(t, seg);

        // Interpolate orientation
        glm::mat4 rotation;
        if (useQuat)
        {
            glm::quat quat = glm::slerp(seg.q1, seg.q2, t);
            rotation = glm::mat4_cast(quat);
        }
        else
        {
            glm::vec3 euler = useBSplines ? fastBSplineEuler(t, seg) : fastCatmullRomEuler(t, seg);
            rotation = glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f),
                                                           glm::radians(euler.x), glm::vec3(1, 0, 0)),
                                               glm::radians(euler.y), glm::vec3(0, 1, 0)),
                                   glm::radians(euler.z), glm::vec3(0, 0, 1));
        }

        lastTime = time;
        return cachedTransform = glm::translate(glm::mat4(1.0f), position) * rotation;
    }

    float getTotalTime() const
    {
        return keyframes.empty() ? 0.0f : keyframes.back().time;
    }

    size_t getKeyFrameCount() const
    {
        return keyframes.size();
    }
};

// Vertex shader source
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Fragment shader source
const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

void main() {
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

// Create a default cube mesh if no OBJ is provided
Mesh *createDefaultCube()
{
    Mesh *cube = new Mesh();

    // Cube vertices with normals and texture coordinates
    float cubeData[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};

    cube->vertices.assign(cubeData, cubeData + sizeof(cubeData) / sizeof(float));
    return cube;
}

unsigned int compileShader(const char *source, GLenum type)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    return shader;
}

void setupShaders()
{
    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// Camera and viewport functions
glm::vec3 getCameraPosition()
{
    float x = cameraDistance * cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
    float y = cameraDistance * sin(glm::radians(cameraPitch));
    float z = cameraDistance * cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));
    return cameraTarget + glm::vec3(x, y, z);
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
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

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
        }
    }
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    cameraDistance -= yoffset * 0.5f;
    if (cameraDistance < 1.0f)
        cameraDistance = 1.0f;
    if (cameraDistance > 50.0f)
        cameraDistance = 50.0f;
}

bool loadMeshFromOBJ(const std::string &filename)
{
    OBJLoader loader;
    Mesh *newMesh = new Mesh();

    if (loader.loadOBJ(filename, *newMesh))
    {
        if (currentMesh)
        {
            delete currentMesh;
        }
        currentMesh = newMesh;
        currentMesh->setupBuffers();
        return true;
    }
    else
    {
        delete newMesh;
        return false;
    }
}

// Parse keyframes from command line format: "x,y,z:e1,e2,e3;..."
bool parseKeyFramesFromString(const std::string &keyframeStr)
{
    if (keyframeStr.empty())
        return false;

    std::vector<KeyFrame> parsedFrames;
    std::stringstream ss(keyframeStr);
    std::string keyframeData;
    float time = 0.0f;
    float timeStep = 1.0f; // 1 seconds between keyframes by default

    // Split by semicolon
    while (std::getline(ss, keyframeData, ';'))
    {
        if (keyframeData.empty())
            continue;

        // Find position of colon
        size_t colonPos = keyframeData.find(':');
        if (colonPos == std::string::npos)
        {
            std::cerr << "Invalid keyframe format: " << keyframeData << std::endl;
            continue;
        }

        std::string posStr = keyframeData.substr(0, colonPos);
        std::string rotStr = keyframeData.substr(colonPos + 1);

        // Parse position (x,y,z)
        std::stringstream posStream(posStr);
        std::string coord;
        glm::vec3 position;
        int coordIndex = 0;

        while (std::getline(posStream, coord, ',') && coordIndex < 3)
        {
            position[coordIndex++] = std::stof(coord);
        }

        // Parse rotation (e1,e2,e3)
        std::stringstream rotStream(rotStr);
        std::string angle;
        glm::vec3 euler;
        int angleIndex = 0;

        while (std::getline(rotStream, angle, ',') && angleIndex < 3)
        {
            euler[angleIndex++] = std::stof(angle);
        }

        // Add keyframe to parsed list
        parsedFrames.emplace_back(position, euler, time);
        time += timeStep;
    }

    if (parsedFrames.empty())
    {
        return false;
    }

    // Clear existing and add all at once for better performance
    motionController->clearKeyFrames();
    motionController->addMultipleKeyFrames(parsedFrames);

    std::cout << "Parsed " << parsedFrames.size() << " keyframes from command line" << std::endl;
    return true;
}

// Create default keyframes if no file is provided
void setupDefaultKeyFrames()
{
    motionController = new OptimizedMotionController();

    // Define a complex motion path as default
    std::vector<KeyFrame> defaultFrames;
    defaultFrames.emplace_back(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), 0.0f);
    defaultFrames.emplace_back(glm::vec3(3, 2, 0), glm::vec3(45, 90, 0), 2.0f);
    defaultFrames.emplace_back(glm::vec3(0, 4, 3), glm::vec3(90, 180, 45), 4.0f);
    defaultFrames.emplace_back(glm::vec3(-3, 2, 0), glm::vec3(135, 270, 90), 6.0f);
    defaultFrames.emplace_back(glm::vec3(0, 0, -3), glm::vec3(180, 360, 135), 8.0f);
    defaultFrames.emplace_back(glm::vec3(0, 0, 0), glm::vec3(360, 720, 360), 10.0f);

    motionController->addMultipleKeyFrames(defaultFrames);
    std::cout << "Using default keyframes" << std::endl;
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
            // Reset camera
            cameraDistance = 8.0f;
            cameraYaw = 45.0f;
            cameraPitch = 35.0f;
            cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
            std::cout << "Reset camera" << std::endl;
            break;
        case GLFW_KEY_P:
            showPerformanceStats = !showPerformanceStats;
            std::cout << "Performance stats: " << (showPerformanceStats ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        }
    }
}

void render()
{
    // Performance timing
    auto renderStart = std::chrono::high_resolution_clock::now();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!currentMesh)
        return;

    glUseProgram(shaderProgram);

    // Calculate camera position
    glm::vec3 cameraPos = getCameraPosition();

    // Set up view and projection matrices with current window aspect ratio
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0, 1, 0));
    float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

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
    currentMesh->render();

    // Performance timing
    auto renderEnd = std::chrono::high_resolution_clock::now();
    if (showPerformanceStats)
    {
        auto renderDuration = std::chrono::duration_cast<std::chrono::microseconds>(renderEnd - renderStart);
        frameTime = renderDuration.count() / 1000.0f; // Convert to milliseconds
    }
}

int main(int argc, char *argv[])
{
    std::string keyframeString = "";
    bool keyframesProvided = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-ot" && i + 1 < argc)
        {
            std::string orientationType = argv[i + 1];
            if (orientationType == "quat" || orientationType == "quaternion" || orientationType == "0")
            {
                useQuaternions = true;
            }
            else if (orientationType == "euler" || orientationType == "1")
            {
                useQuaternions = false;
            }
            else
            {
                std::cerr << "Invalid orientation type: " << orientationType << std::endl;
                return -1;
            }
            i++; // Skip next argument
        }
        else if (arg == "-it" && i + 1 < argc)
        {
            std::string interpolationType = argv[i + 1];
            if (interpolationType == "crspline" || interpolationType == "catmullrom" || interpolationType == "0")
            {
                useBSpline = false;
            }
            else if (interpolationType == "bspline" || interpolationType == "1")
            {
                useBSpline = true;
            }
            else
            {
                std::cerr << "Invalid interpolation type: " << interpolationType << std::endl;
                return -1;
            }
            i++; // Skip next argument
        }
        else if (arg == "-kf" && i + 1 < argc)
        {
            keyframeString = argv[i + 1];
            keyframesProvided = true;
            i++; // Skip next argument
        }
        else if (arg == "-m" && i + 1 < argc)
        {
            objFilename = argv[i + 1];
            i++; // Skip next argument
        }
        else if (arg == "-h" || arg == "--help")
        {
            std::cout << "Key-framing Motion Control System (Optimized)" << std::endl;
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -ot <type>     Orientation type: quat/quaternion/0 (default), euler/1" << std::endl;
            std::cout << "  -it <type>     Interpolation type: crspline/catmullrom/0 (default), bspline/1" << std::endl;
            std::cout << "  -kf <keyframes> Keyframes, format: \"x,y,z:e1,e2,e3;...\" (Euler angles in degrees)" << std::endl;
            std::cout << "  -m <filepath>   File path, loads models with .obj extension (default: cube)" << std::endl;
            std::cout << "  -h, --help     Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  " << argv[0] << " -ot quat -it bspline -kf \"0,0,0:0,0,0;3,2,1:45,90,0;0,4,2:90,180,45\"" << std::endl;
            std::cout << "  " << argv[0] << " -m teapot.obj -ot euler -it crspline" << std::endl;
            std::cout << "  " << argv[0] << " -kf \"0,0,0:0,0,0;5,0,0:0,90,0;0,5,0:0,180,0;0,0,5:0,270,0\"" << std::endl;
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            std::cout << "Use -h or --help for usage information" << std::endl;
            return -1;
        }
    }

    std::cout << "Key-framing Motion Control System (Optimized)" << std::endl;
    std::cout << "Settings:" << std::endl;
    std::cout << "  Orientation: " << (useQuaternions ? "Quaternions" : "Euler Angles") << std::endl;
    std::cout << "  Interpolation: " << (useBSpline ? "B-Spline" : "Catmull-Rom") << std::endl;

    if (!objFilename.empty())
    {
        std::cout << "  Model: " << objFilename << std::endl;
    }

    if (keyframesProvided)
    {
        std::cout << "  Custom keyframes provided" << std::endl;
    }

    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow *window = glfwCreateWindow(800, 600, "Key-frame Motion Control System", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Setup shaders
    setupShaders();

    // Setup keyframes
    motionController = new OptimizedMotionController();
    if (keyframesProvided)
    {
        if (!parseKeyFramesFromString(keyframeString))
        {
            std::cout << "Failed to parse keyframes, using defaults" << std::endl;
            setupDefaultKeyFrames();
        }
    }
    else
    {
        setupDefaultKeyFrames();
    }

    if (!objFilename.empty())
    {
        if (loadMeshFromOBJ(objFilename))
        {
            std::cout << "Successfully loaded model: " << objFilename << std::endl;
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

    std::cout << "\nInteractive Controls:" << std::endl;
    std::cout << "Q - Toggle Quaternions (" << (useQuaternions ? "ON" : "OFF") << ") / Euler Angles (" << (!useQuaternions ? "ON" : "OFF") << ")" << std::endl;
    std::cout << "S - Toggle B-Spline (" << (useBSpline ? "ON" : "OFF") << ") / Catmull-Rom (" << (!useBSpline ? "ON" : "OFF") << ")" << std::endl;
    std::cout << "R - Reset animation" << std::endl;
    std::cout << "C - Reset camera" << std::endl;
    std::cout << "P - Toggle performance stats" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "\nCamera Controls:" << std::endl;
    std::cout << "Mouse Drag - Rotate camera" << std::endl;
    std::cout << "Mouse Wheel - Zoom in/out" << std::endl;
    std::cout << "Window Resize - Adjusts viewport" << std::endl;

    std::cout << "\nLoaded " << motionController->getKeyFrameCount() << " keyframes" << std::endl;
    std::cout << "Animation duration: " << motionController->getTotalTime() << " seconds" << std::endl;

    // Main loop
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update animation time with fixed speed
        currentTime += deltaTime * 0.5f; // Fixed animation speed
        if (currentTime > motionController->getTotalTime())
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

    // Cleanup
    if (currentMesh)
    {
        delete currentMesh;
    }
    glDeleteProgram(shaderProgram);

    delete motionController;
    glfwTerminate();

    return 0;
}