#ifndef ANIMATION_H
#define ANIMATION_H

#include <vector>
#include <chrono>
#include <memory>
#include <Eigen/Geometry>
#include "Skeleton.h"


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
    Seconds duration;
    std::shared_ptr<const Skeleton> skeleton;
};

class Animation
{
public:
    Animation(std::shared_ptr<const AnimationData> data);
    std::tuple<std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix<float, 4, 2>>> buildBoneMats(Seconds at) const;

    Seconds duration() const;
    const AnimationData& data() const;
private:
    std::shared_ptr<const AnimationData> _data;
};

#endif
