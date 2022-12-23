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
    std::tuple<std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix<float, 4, 2>>> buildBoneMats() const;

    virtual void reset(Seconds at={}) = 0;
    virtual void tick(Seconds delta) = 0;
    virtual Eigen::Quaternionf getBoneRot(BoneIndex index) const = 0;
    virtual Seconds time() const = 0;
    virtual Seconds duration() const = 0;
    virtual std::shared_ptr<const Skeleton> skeleton() const = 0;

    virtual ~Animation() = default;
protected:
    Animation() = default;
};

class NullAnimation : public Animation
{
public:
    NullAnimation(std::shared_ptr<const Skeleton> skeleton)
        :_skeleton(std::move(skeleton))
    {
        if(!_skeleton) throw std::invalid_argument("Animation built without skeleton");
    }
    void reset(Seconds at={}) override {}
    void tick(Seconds delta) override {}
    Eigen::Quaternionf getBoneRot(BoneIndex index) const override
    {
        return Eigen::Quaternionf::Identity();
    }
    Seconds time() const override
    {
        return {};
    }
    Seconds duration() const override
    {
        return {};
    }
    std::shared_ptr<const Skeleton> skeleton() const override
    {
        return _skeleton;
    }

private:
    std::shared_ptr<const Skeleton> _skeleton;
};

class SimpleAnimation : public Animation
{
public:
    SimpleAnimation(std::shared_ptr<const AnimationData> data);

    void reset(Seconds at={}) override;
    void tick(Seconds delta) override;
    Eigen::Quaternionf getBoneRot(BoneIndex index) const override;
    Seconds time() const override;
    Seconds duration() const override;
    std::shared_ptr<const Skeleton> skeleton() const override;

    std::shared_ptr<const AnimationData> data() const;
private:
    void loopBack();
    std::shared_ptr<const AnimationData> _data;
    Seconds _timer{};
};

#endif
