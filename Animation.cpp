#include "Animation.h"
#include <algorithm>


Eigen::Quaternionf AnimationCurve::sample(Seconds at) const
{
    if(keyframes.empty())
        return Eigen::Quaternionf::Identity();
    if(auto it = std::ranges::find(keyframes, at, &Keyframe::time); it != keyframes.end())
        return it->rotation;
    throw std::runtime_error("Keyframe interpolation not supported yet");
}
