#ifndef ANIMATION_H
#define ANIMATION_H

#include <vector>
#include <chrono>
#include <Eigen/Geometry>


using Seconds = std::chrono::duration<float>;
using Frames = std::chrono::duration<int, std::ratio<1, 60>>;

struct Keyframe
{
    Seconds time;
    Eigen::Quaternionf rotation;
};

struct KeyframeTimeSort
{
    bool operator()(const Keyframe& l, const Keyframe& r) const
    {
        return l.time < r.time;
    }
};

struct AnimationCurve
{
    std::vector<Keyframe> keyframes;
    Eigen::Quaternionf sample(Seconds at) const;
};

struct AnimationData
{
    std::vector<AnimationCurve> curves;
    Frames duration;
};

#endif
