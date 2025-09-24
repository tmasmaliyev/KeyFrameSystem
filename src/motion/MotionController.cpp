#include "motion/MotionController.h"
#include <algorithm>
#include <iostream>
#include <cmath>

// KeyFrame implementation
KeyFrame::KeyFrame(glm::vec3 pos, glm::vec3 euler, float t)
    : position(pos), eulerAngles(euler), time(t)
{
    // Convert Euler angles to quaternion
    quaternion = glm::quat(glm::radians(euler));
}

KeyFrame::KeyFrame(glm::vec3 pos, glm::quat quat, float t)
    : position(pos), quaternion(quat), time(t)
{
    // Convert quaternion to Euler angles
    eulerAngles = glm::degrees(glm::eulerAngles(quat));
}

// OptimizedMotionController implementation
void OptimizedMotionController::precomputeSegments() const {
    if (segmentsCached || keyframes.size() < 2) return;
    
    segments.clear();
    segments.reserve(keyframes.size() - 1);
    
    for (size_t i = 0; i < keyframes.size() - 1; i++) {
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

int OptimizedMotionController::findSegment(float time) const {
    if (keyframes.size() < 2) return 0;
    
    // Check if we're still in the same segment
    if (lastSegment < static_cast<int>(segments.size()) &&
        time >= segments[lastSegment].startTime && 
        time <= segments[lastSegment].endTime) {
        return lastSegment;
    }
    
    // Check adjacent segments first (common case)
    if (lastSegment + 1 < static_cast<int>(segments.size()) &&
        time >= segments[lastSegment + 1].startTime && 
        time <= segments[lastSegment + 1].endTime) {
        return lastSegment + 1;
    }
    
    if (lastSegment > 0 &&
        time >= segments[lastSegment - 1].startTime && 
        time <= segments[lastSegment - 1].endTime) {
        return lastSegment - 1;
    }
    
    // Binary search for distant segments
    int left = 0, right = segments.size() - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (time < segments[mid].startTime) {
            right = mid - 1;
        } else if (time > segments[mid].endTime) {
            left = mid + 1;
        } else {
            return mid;
        }
    }
    
    return std::max(0, std::min(right, static_cast<int>(segments.size()) - 1));
}

glm::vec3 OptimizedMotionController::fastCatmullRomPosition(float t, const SegmentData& seg) const {
    float t2 = t * t;
    float t3 = t2 * t;
    
    return 0.5f * (2.0f * seg.p1 +
                   (-seg.p0 + seg.p2) * t +
                   (2.0f * seg.p0 - 5.0f * seg.p1 + 4.0f * seg.p2 - seg.p3) * t2 +
                   (-seg.p0 + 3.0f * seg.p1 - 3.0f * seg.p2 + seg.p3) * t3);
}

glm::vec3 OptimizedMotionController::fastCatmullRomEuler(float t, const SegmentData& seg) const {
    float t2 = t * t;
    float t3 = t2 * t;
    
    return 0.5f * (2.0f * seg.e1 +
                   (-seg.e0 + seg.e2) * t +
                   (2.0f * seg.e0 - 5.0f * seg.e1 + 4.0f * seg.e2 - seg.e3) * t2 +
                   (-seg.e0 + 3.0f * seg.e1 - 3.0f * seg.e2 + seg.e3) * t3);
}

glm::vec3 OptimizedMotionController::fastBSplinePosition(float t, const SegmentData& seg) const {
    float t2 = t * t;
    float t3 = t2 * t;
    
    return (1.0f / 6.0f) * ((-t3 + 3.0f * t2 - 3.0f * t + 1.0f) * seg.p0 +
                            (3.0f * t3 - 6.0f * t2 + 4.0f) * seg.p1 +
                            (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * seg.p2 +
                            t3 * seg.p3);
}

glm::vec3 OptimizedMotionController::fastBSplineEuler(float t, const SegmentData& seg) const {
    float t2 = t * t;
    float t3 = t2 * t;
    
    return (1.0f / 6.0f) * ((-t3 + 3.0f * t2 - 3.0f * t + 1.0f) * seg.e0 +
                            (3.0f * t3 - 6.0f * t2 + 4.0f) * seg.e1 +
                            (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * seg.e2 +
                            t3 * seg.e3);
}

glm::vec3 OptimizedMotionController::normalizeAngles(glm::vec3 angles, glm::vec3 reference) const {
    glm::vec3 result = angles;
    for (int i = 0; i < 3; i++) {
        while (result[i] - reference[i] > 180.0f) result[i] -= 360.0f;
        while (result[i] - reference[i] < -180.0f) result[i] += 360.0f;
    }
    return result;
}

void OptimizedMotionController::addKeyFrame(const KeyFrame &kf) {
    keyframes.push_back(kf);
    segmentsCached = false; // Invalidate cache
    lastTime = -1.0f; // Force recalculation
}

void OptimizedMotionController::addMultipleKeyFrames(const std::vector<KeyFrame>& kfs) {
    keyframes.reserve(keyframes.size() + kfs.size());
    for (const auto& kf : kfs) {
        keyframes.push_back(kf);
    }
    segmentsCached = false; // Invalidate cache once
    lastTime = -1.0f; // Force recalculation
}

void OptimizedMotionController::clearKeyFrames() {
    keyframes.clear();
    segments.clear();
    segmentsCached = false;
    lastSegment = 0;
    lastTime = -1.0f;
}

glm::mat4 OptimizedMotionController::getTransformationMatrix(float time, bool useQuat, bool useBSplines) const {
    // Early return for empty keyframes
    if (keyframes.empty()) return glm::mat4(1.0f);
    
    // Cache check - if time hasn't changed much, return cached result
    const float TIME_EPSILON = 0.0001f;
    if (std::abs(time - lastTime) < TIME_EPSILON && lastTime >= 0.0f) {
        return cachedTransform;
    }
    
    // Single keyframe case
    if (keyframes.size() == 1) {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), keyframes[0].position);
        glm::mat4 rotation = useQuat ? 
            glm::mat4_cast(keyframes[0].quaternion) : 
            glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f),
                glm::radians(keyframes[0].eulerAngles.x), glm::vec3(1, 0, 0)),
                glm::radians(keyframes[0].eulerAngles.y), glm::vec3(0, 1, 0)),
                glm::radians(keyframes[0].eulerAngles.z), glm::vec3(0, 0, 1));
        lastTime = time;
        return cachedTransform = translation * rotation;
    }
    
    // Precompute segments if needed
    precomputeSegments();
    
    // Find current segment
    int currentSegment = findSegment(time);
    lastSegment = currentSegment;
    
    // Handle end of animation
    if (currentSegment >= static_cast<int>(segments.size())) {
        const KeyFrame& lastKF = keyframes.back();
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), lastKF.position);
        glm::mat4 rotation = useQuat ? 
            glm::mat4_cast(lastKF.quaternion) : 
            glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f),
                glm::radians(lastKF.eulerAngles.x), glm::vec3(1, 0, 0)),
                glm::radians(lastKF.eulerAngles.y), glm::vec3(0, 1, 0)),
                glm::radians(lastKF.eulerAngles.z), glm::vec3(0, 0, 1));
        lastTime = time;
        return cachedTransform = translation * rotation;
    }
    
    const SegmentData& seg = segments[currentSegment];
    
    // Calculate interpolation parameter
    float t = (seg.duration > 0.0f) ? 
              glm::clamp((time - seg.startTime) / seg.duration, 0.0f, 1.0f) : 0.0f;
    
    // Interpolate position
    glm::vec3 position = useBSplines ? 
                        fastBSplinePosition(t, seg) : 
                        fastCatmullRomPosition(t, seg);
    
    // Interpolate orientation
    glm::mat4 rotation;
    if (useQuat) {
        glm::quat quat = glm::slerp(seg.q1, seg.q2, t);
        rotation = glm::mat4_cast(quat);
    } else {
        glm::vec3 euler = useBSplines ? 
                         fastBSplineEuler(t, seg) : 
                         fastCatmullRomEuler(t, seg);
        rotation = glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f),
            glm::radians(euler.x), glm::vec3(1, 0, 0)),
            glm::radians(euler.y), glm::vec3(0, 1, 0)),
            glm::radians(euler.z), glm::vec3(0, 0, 1));
    }
    
    lastTime = time;
    return cachedTransform = glm::translate(glm::mat4(1.0f), position) * rotation;
}

float OptimizedMotionController::getTotalTime() const {
    return keyframes.empty() ? 0.0f : keyframes.back().time;
}

size_t OptimizedMotionController::getKeyFrameCount() const {
    return keyframes.size();
}