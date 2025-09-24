#ifndef UTILS_H
#define UTILS_H

#include "MotionController.h"
#include "Mesh.h"
#include <string>
#include <vector>

// Keyframe parsing utilities
bool parseKeyFramesFromString(const std::string &keyframeStr, OptimizedMotionController *controller);
void setupDefaultKeyFrames(OptimizedMotionController *controller);

// Mesh loading utilities
bool loadMeshFromOBJ(const std::string &filename, Mesh **currentMesh);

// Command line parsing
struct ProgramConfig
{
    bool useQuaternions = true;
    bool useBSpline = false;
    std::string objFilename = "";
    std::string keyframeString = "";
    bool keyframesProvided = false;
    bool showHelp = false;

    std::string vertexShaderPath = "assets/shaders/vertex.glsl";
    std::string fragmentShaderPath = "assets/shaders/fragment.glsl";
};

bool parseCommandLine(int argc, char *argv[], ProgramConfig &config);
void printHelp(const char *programName);

#endif // UTILS_H