#include "Animation.h"
#include <algorithm>


namespace
{
//Needed for type inferrence because Eigen's function returns an intermediary type
Eigen::Matrix4f identity()
{
    return Eigen::Matrix4f::Identity();
}

auto buildDualQuat(Eigen::Quaternionf rotation, Eigen::Vector4f translation)
{
    using DualQuat = Eigen::Matrix<float, 4, 2>;
    translation.w() = 0;
    DualQuat result{};
    result.col(0) = rotation.coeffs();
    result.col(1) = (Eigen::Quaternionf{translation} * rotation).coeffs() / 2;
    return result;
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

std::tuple<std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix<float, 4, 2>>> Animation::buildBoneMats() const
{
    std::tuple<std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix<float, 4, 2>>> result;
    auto& [bone_mats, norm_bone_mats, dual_quats] = result;
    const Skeleton* skeleton = this->skeleton().get();
    bone_mats.resize(skeleton->boneCount());
    norm_bone_mats.resize(skeleton->boneCount());
    dual_quats.resize(skeleton->boneCount());
    
    skeleton->exploreTree(0,
    [&](BoneIndex index, Eigen::Matrix4f& mat_skinned, Eigen::Matrix4f& inv_mat_skinned, Eigen::Matrix4f& mat_unskinned, Eigen::Matrix4f& inv_mat_unskinned,
        Eigen::Quaternionf& unskinned_rotation, Eigen::Quaternionf& skinned_rotation)
    {
        Transform transform = skeleton->boneTransform(index);

        mat_unskinned.applyOnTheRight(transform.matrix());
        inv_mat_unskinned.applyOnTheLeft(transform.inverseMatrix());
        unskinned_rotation *= transform.rotation;

        transform.rotation *= getBoneRot(index);

        mat_skinned.applyOnTheRight(transform.matrix());
        inv_mat_skinned.applyOnTheLeft(transform.inverseMatrix());
        skinned_rotation *= transform.rotation;

        bone_mats[index] = mat_skinned * inv_mat_unskinned;
        norm_bone_mats[index] = mat_unskinned * inv_mat_skinned;
        dual_quats[index] = buildDualQuat(skinned_rotation * unskinned_rotation.conjugate(), bone_mats[index].col(3));
    }, identity(), identity(), identity(), identity(), Eigen::Quaternionf::Identity(), Eigen::Quaternionf::Identity());

    return result;
}

SimpleAnimation::SimpleAnimation(std::shared_ptr<const AnimationData> data)
    :_data{std::move(data)}
{
    if(!_data) throw std::invalid_argument("Animation built without data");
}

void SimpleAnimation::reset(Seconds at)
{
    _timer = at;
    loopBack();
}

void SimpleAnimation::tick(Seconds delta)
{
    _timer += delta;
    loopBack();
}

Eigen::Quaternionf SimpleAnimation::getBoneRot(BoneIndex index) const
{
    return _data->curves[index].sample(_timer);
}

Seconds SimpleAnimation::time() const
{
    return _timer;
}

Seconds SimpleAnimation::duration() const
{
    return _data->duration;
}

std::shared_ptr<const Skeleton> SimpleAnimation::skeleton() const
{
    return _data->skeleton;
}

std::shared_ptr<const AnimationData> SimpleAnimation::data() const
{
    return _data;
}

void SimpleAnimation::loopBack()
{
    Seconds d = duration();
    while(_timer > d) _timer -= d;
}
