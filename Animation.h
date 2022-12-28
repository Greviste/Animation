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
    std::string name;
    std::vector<AnimationCurve> curves;
    Seconds duration;
    std::shared_ptr<const Skeleton> skeleton;
};

std::shared_ptr<const AnimationData> retarget(std::shared_ptr<const AnimationData> old, std::shared_ptr<const Skeleton> target);

class Animation
{
public:
    std::tuple<std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix4f>, std::vector<Eigen::Matrix<float, 4, 2>>> buildBoneMats() const;

    virtual std::string name() const = 0;
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
    std::string name() const override { return "Null"; }
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

    std::string name() const override;
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

class CompositeAnimation : public Animation
{
public:
    using size_t = std::size_t;
    CompositeAnimation(std::shared_ptr<const Skeleton> ref);
    CompositeAnimation(std::unique_ptr<Animation> ref);

    size_t addAnimation(std::unique_ptr<Animation> anim);
    std::unique_ptr<Animation> releaseAnimation(size_t index);
    std::unique_ptr<Animation> swapAnimation(size_t index, std::unique_ptr<Animation> new_anim);
    size_t count() const;
    const Animation& operator[](size_t i) const;
    Animation& operator[](size_t i);

    std::shared_ptr<const Skeleton> skeleton() const final override;

private:
    void sanitize(std::unique_ptr<Animation>& anim) const;
    virtual void rebuild() {}

    std::vector<std::unique_ptr<Animation>> _children;
};

class AdditiveAnimation : public CompositeAnimation
{
public:
    using CompositeAnimation::CompositeAnimation;

    std::string name() const override;
    void reset(Seconds at={}) override;
    void tick(Seconds delta) override;
    Eigen::Quaternionf getBoneRot(BoneIndex index) const override;
    Seconds time() const override;
    Seconds duration() const override;
};

class BlendedAnimation : public CompositeAnimation
{
public:
    using CompositeAnimation::CompositeAnimation;

    float blendFactor() const;
    void setBlendFactor(float f);

    std::string name() const override;
    void reset(Seconds at={}) override;
    void tick(Seconds delta) override;
    Eigen::Quaternionf getBoneRot(BoneIndex index) const override;
    Seconds time() const override;
    Seconds duration() const override;

private:
    float speedFactor(size_t i) const;
    float localBlendFactor() const;
    std::vector<size_t> getTargets() const;
    void resync(size_t i);

    void rebuild() override;

    float _blend_factor = 0;
    float _normalized_timer = 0;
};

#endif
