#include "Skeleton.h"
#include <algorithm>


BoneIndex Skeleton::boneFromName(std::string_view name)
{
    auto result = findBone(name);
    if(result)
    {
        return *result;
    }
    _bones.emplace_back(name);
    return _bones.size() - 1;
}

std::optional<BoneIndex> Skeleton::findBone(std::string_view name) const
{
    auto it = std::ranges::find(_bones, name);
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
    return _bones[index];
}
