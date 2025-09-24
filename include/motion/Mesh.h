#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Forward declaration for friend function
class Mesh;
Mesh *createDefaultCube();

// Mesh structure
class Mesh
{
private:
    std::vector<float> vertices; // Interleaved: position(3) + normal(3) + texcoord(2)
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

public:
    Mesh();
    ~Mesh();

    void cleanup();
    void setupBuffers();
    void render();

    // Allow OBJLoader to access vertices and indices
    friend class OBJLoader;

    // Allow createDefaultCube to access private members
    friend Mesh *createDefaultCube();
};

// OBJ Loader class
class OBJLoader
{
private:
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    glm::vec3 calculateNormal(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2);

public:
    bool loadOBJ(const std::string &path, Mesh &mesh);
};

// Utility function to create default cube
Mesh *createDefaultCube();

#endif // MESH_H