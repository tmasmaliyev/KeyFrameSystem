#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

// Keyframe structure
struct KeyFrame
{
    glm::vec3 position;
    glm::vec3 eulerAngles; // In degrees
    glm::quat quaternion;
    float time;

    KeyFrame(glm::vec3 pos, glm::vec3 euler, float t);
    KeyFrame(glm::vec3 pos, glm::quat quat, float t);
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

    void precomputeSegments() const;
    int findSegment(float time) const;

    // Optimized spline calculations
    glm::vec3 fastCatmullRomPosition(float t, const SegmentData &seg) const;
    glm::vec3 fastCatmullRomEuler(float t, const SegmentData &seg) const;
    glm::vec3 fastBSplinePosition(float t, const SegmentData &seg) const;
    glm::vec3 fastBSplineEuler(float t, const SegmentData &seg) const;
    glm::vec3 normalizeAngles(glm::vec3 angles, glm::vec3 reference) const;

public:
    void addKeyFrame(const KeyFrame &kf);
    void addMultipleKeyFrames(const std::vector<KeyFrame> &kfs);
    void clearKeyFrames();
    glm::mat4 getTransformationMatrix(float time, bool useQuat, bool useBSplines) const;
    float getTotalTime() const;
    size_t getKeyFrameCount() const;
};

#endif // MOTIONCONTROLLER_H