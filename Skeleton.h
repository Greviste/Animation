#ifndef SKELETON_H
#define SKELETON_H

#include <string>
#include <vector>
#include <optional>
#include <Eigen/Geometry>


using BoneIndex = unsigned short;

struct Transform
{
    Eigen::Vector3f translation;
    Eigen::Quaternionf rotation;
    Eigen::Vector3f scale;

    Eigen::Matrix4f matrix() const;
    Eigen::Matrix4f inverseMatrix() const;
};

struct Bone
{
    BoneIndex parent;
    std::vector<BoneIndex> children;
    std::string name;
    Transform local_origin;
};

class Skeleton
{
public:
    BoneIndex addBone(std::string name, Transform local_origin, std::optional<BoneIndex> parent = std::nullopt);
    std::optional<BoneIndex> findBone(std::string_view name) const;
    BoneIndex boneCount() const;
    std::string_view boneName(BoneIndex index) const;
    const std::vector<BoneIndex>& boneChildren(BoneIndex index) const;
    const Transform& boneTransform(BoneIndex index) const;
    //Explores the skeleton depth-first, and calls func(index, args...) where index is the const BoneIndex of the current node and args are a local copy
    //You can modify args before they get sent to the child nodes by using references in func
    template<typename F, typename... Args>
    void exploreTree(const BoneIndex root, F&& func, Args... args) const;
    
private:
    std::vector<Bone> _bones;
};

template<typename F, typename... Args>
void Skeleton::exploreTree(const BoneIndex root, F&& func, Args... args) const
{
    func(root, args...);
    for(BoneIndex child : boneChildren(root))
    {
        exploreTree(child, func, args...);
    }
}

#endif
