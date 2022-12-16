#ifndef SKELETON_H
#define SKELETON_H

#include <string>
#include <vector>
#include <optional>


using BoneIndex = unsigned short;

class Skeleton
{
public:
    BoneIndex boneFromName(std::string_view name);
    std::optional<BoneIndex> findBone(std::string_view name) const;
    BoneIndex boneCount() const;
    std::string_view boneName(BoneIndex index) const;
    
private:
    std::vector<std::string> _bones;
};

#endif
