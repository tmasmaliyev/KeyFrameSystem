#include "motion/Mesh.h"

// Mesh implementation
Mesh::Mesh() : VAO(0), VBO(0), EBO(0) {}

Mesh::~Mesh()
{
    cleanup();
}

void Mesh::cleanup()
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

void Mesh::setupBuffers()
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

void Mesh::render()
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

// OBJLoader implementation
glm::vec3 OBJLoader::calculateNormal(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2)
{
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    return glm::normalize(glm::cross(edge1, edge2));
}

bool OBJLoader::loadOBJ(const std::string &path, Mesh &mesh)
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

// Create a default cube mesh
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