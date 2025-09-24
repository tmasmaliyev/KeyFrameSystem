#include "motion/Utils.h"
#include <iostream>
#include <sstream>
#include <fstream>

bool parseKeyFramesFromString(const std::string &keyframeStr, OptimizedMotionController *controller)
{
    if (keyframeStr.empty() || !controller)
        return false;

    std::vector<KeyFrame> parsedFrames;
    std::stringstream ss(keyframeStr);
    std::string keyframeData;
    float time = 0.0f;
    float timeStep = 2.0f; // 2 seconds between keyframes by default

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
    controller->clearKeyFrames();
    controller->addMultipleKeyFrames(parsedFrames);

    std::cout << "Parsed " << parsedFrames.size() << " keyframes from command line" << std::endl;
    return true;
}

void setupDefaultKeyFrames(OptimizedMotionController *controller)
{
    if (!controller)
        return;

    // Define a complex motion path as default
    std::vector<KeyFrame> defaultFrames;
    defaultFrames.emplace_back(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), 0.0f);
    defaultFrames.emplace_back(glm::vec3(3, 2, 0), glm::vec3(45, 90, 0), 2.0f);
    defaultFrames.emplace_back(glm::vec3(0, 4, 3), glm::vec3(90, 180, 45), 4.0f);
    defaultFrames.emplace_back(glm::vec3(-3, 2, 0), glm::vec3(135, 270, 90), 6.0f);
    defaultFrames.emplace_back(glm::vec3(0, 0, -3), glm::vec3(180, 360, 135), 8.0f);
    defaultFrames.emplace_back(glm::vec3(0, 0, 0), glm::vec3(360, 720, 360), 10.0f);

    controller->addMultipleKeyFrames(defaultFrames);
    std::cout << "Using default keyframes" << std::endl;
}

bool loadMeshFromOBJ(const std::string &filename, Mesh **currentMesh)
{
    if (!currentMesh)
        return false;

    OBJLoader loader;
    Mesh *newMesh = new Mesh();

    if (loader.loadOBJ(filename, *newMesh))
    {
        if (*currentMesh)
        {
            delete *currentMesh;
        }
        *currentMesh = newMesh;
        (*currentMesh)->setupBuffers();
        return true;
    }
    else
    {
        delete newMesh;
        return false;
    }
}

bool parseCommandLine(int argc, char *argv[], ProgramConfig &config)
{
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-ot" && i + 1 < argc)
        {
            std::string orientationType = argv[i + 1];
            if (orientationType == "quat" || orientationType == "quaternion" || orientationType == "0")
            {
                config.useQuaternions = true;
            }
            else if (orientationType == "euler" || orientationType == "1")
            {
                config.useQuaternions = false;
            }
            else
            {
                std::cerr << "Invalid orientation type: " << orientationType << std::endl;
                return false;
            }
            i++; // Skip next argument
        }
        else if (arg == "-it" && i + 1 < argc)
        {
            std::string interpolationType = argv[i + 1];
            if (interpolationType == "crspline" || interpolationType == "catmullrom" || interpolationType == "0")
            {
                config.useBSpline = false;
            }
            else if (interpolationType == "bspline" || interpolationType == "1")
            {
                config.useBSpline = true;
            }
            else
            {
                std::cerr << "Invalid interpolation type: " << interpolationType << std::endl;
                return false;
            }
            i++; // Skip next argument
        }
        else if (arg == "-kf" && i + 1 < argc)
        {
            config.keyframeString = argv[i + 1];
            config.keyframesProvided = true;
            i++; // Skip next argument
        }
        else if (arg == "-m" && i + 1 < argc)
        {
            config.objFilename = argv[i + 1];
            i++; // Skip next argument
        }
        else if (arg == "-h" || arg == "--help")
        {
            config.showHelp = true;
            return true;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            std::cout << "Use -h or --help for usage information" << std::endl;
            return false;
        }
    }

    return true;
}

void printHelp(const char *programName)
{
    std::cout << "Key-framing Motion Control System (Optimized)" << std::endl;
    std::cout << "Usage: " << programName << " [OPTIONS]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -ot <type>     Orientation type: quat/quaternion/0 (default), euler/1" << std::endl;
    std::cout << "  -it <type>     Interpolation type: crspline/catmullrom/0 (default), bspline/1" << std::endl;
    std::cout << "  -kf <keyframes> Keyframes, format: \"x,y,z:e1,e2,e3;...\" (Euler angles in degrees)" << std::endl;
    std::cout << "  -m <filepath>   File path, loads models with .obj extension (default: cube or teapot.obj if it exists)" << std::endl;
    std::cout << "  -h, --help     Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " -ot quat -it bspline -kf \"0,0,0:0,0,0;3,2,1:45,90,0;0,4,2:90,180,45\"" << std::endl;
    std::cout << "  " << programName << " -m teapot.obj -ot euler -it crspline" << std::endl;
    std::cout << "  " << programName << " -kf \"0,0,0:0,0,0;5,0,0:0,90,0;0,5,0:0,180,0;0,0,5:0,270,0\"" << std::endl;
}