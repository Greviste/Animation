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

std::pair<std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix4f>> Animation::buildBoneMats(Seconds at) const
{
    std::pair<std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix4f>> result;
    auto& [bone_mats, norm_bone_mats] = result;
    bone_mats.resize(_data->skeleton->boneCount());
    norm_bone_mats.resize(_data->skeleton->boneCount());
    
    _data->skeleton->exploreTree(0,
    [&](BoneIndex index, Eigen::Matrix4f& mat_skinned, Eigen::Matrix4f& inv_mat_skinned, Eigen::Matrix4f& mat_unskinned, Eigen::Matrix4f& inv_mat_unskinned)
    {
        Transform transform = _data->skeleton->boneTransform(index);

        mat_unskinned.applyOnTheRight(transform.matrix());
        inv_mat_unskinned.applyOnTheLeft(transform.inverseMatrix());

        transform.rotation *= _data->curves[index].sample(at);

        mat_skinned.applyOnTheRight(transform.matrix());
        inv_mat_skinned.applyOnTheLeft(transform.inverseMatrix());

        bone_mats[index] = mat_skinned * inv_mat_unskinned;
        norm_bone_mats[index] = mat_unskinned * inv_mat_skinned;
    }, identity(), identity(), identity(), identity());

    return result;
}

Seconds Animation::duration() const
{
    return _data->duration;
}

const AnimationData& Animation::data() const
{
    return *_data;
}
