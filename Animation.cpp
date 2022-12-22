#include "Animation.h"
#include <algorithm>


namespace
{
//Needed for type inferrence because Eigen's function returns an intermediary type
Eigen::Matrix4f identity()
{
    return Eigen::Matrix4f::Identity();
}
}

Eigen::Quaternionf AnimationCurve::sample(Seconds at) const
{
    if(keyframes.empty())
        return Eigen::Quaternionf::Identity();
    auto it = std::ranges::upper_bound(keyframes, at, std::ranges::less{}, &Keyframe::time);
    if(it == keyframes.begin())
        return it->rotation;
    if(it == keyframes.end())
        return keyframes.back().rotation;
    auto previous = it - 1;
    return previous->rotation.slerp((at - previous->time) / (it->time - previous->time), it->rotation);
}

Animation::Animation(std::shared_ptr<const AnimationData> data)
    :_data{std::move(data)}
{
    if(!_data) throw std::invalid_argument("Animation built without data");
}

std::vector<Eigen::Matrix4f> Animation::buildBoneMats(Seconds at) const
{
    std::vector<Eigen::Matrix4f> bone_mats(_data->skeleton->boneCount());
    
    _data->skeleton->exploreTree(0, [&](BoneIndex index, Eigen::Matrix4f& parent_mat_skinned, Eigen::Matrix4f& inv_parent_mat_unskinned) {
        Transform transform = _data->skeleton->boneTransform(index);
        inv_parent_mat_unskinned.applyOnTheLeft(transform.inverseMatrix());
        transform.rotation *= _data->curves[index].sample(at);
        parent_mat_skinned.applyOnTheRight(transform.matrix());
        bone_mats[index] = parent_mat_skinned * inv_parent_mat_unskinned;
    }, identity(), identity());

    return bone_mats;
}

Seconds Animation::duration() const
{
    return _data->duration;
}

const AnimationData& Animation::data() const
{
    return *_data;
}
