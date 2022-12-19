#include "Skeleton.h"
#include <algorithm>


Eigen::Matrix4f Transform::matrix() const
{
    return (Eigen::Translation3f{translation} * rotation * Eigen::AlignedScaling3f{scale}).matrix();
}

Eigen::Matrix4f Transform::inverseMatrix() const
{
    return (Eigen::AlignedScaling3f{scale}.inverse() * rotation.conjugate() * Eigen::Translation3f{-translation}).matrix();
}

BoneIndex Skeleton::addBone(std::string name, Transform local_origin, std::optional<BoneIndex> parent)
{
    BoneIndex index = _bones.size();
    if(parent) _bones[*parent].children.push_back(index);
    _bones.emplace_back(Bone{.parent=parent.value_or(index), .name=std::move(name), .local_origin=std::move(local_origin)});

    return index;
}

std::optional<BoneIndex> Skeleton::findBone(std::string_view name) const
{
    auto it = std::ranges::find(_bones, name, &Bone::name);
    if(it == _bones.end())
    {
        return std::nullopt;
    }
    return it - _bones.begin();
}

BoneIndex Skeleton::boneCount() const
{
    return _bones.size();
}

std::string_view Skeleton::boneName(BoneIndex index) const
{
    return _bones[index].name;
}

const std::vector<BoneIndex>& Skeleton::boneChildren(BoneIndex index) const
{
    return _bones[index].children;
}

const Transform& Skeleton::boneTransform(BoneIndex index) const
{
    return _bones[index].local_origin;
}
