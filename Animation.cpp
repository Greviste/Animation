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

template<typename T, typename F>
T lerp(T a, T b, F t)
{
    return a + (b - a) * t;
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

std::shared_ptr<const AnimationData> retarget(std::shared_ptr<const AnimationData> old, std::shared_ptr<const Skeleton> target)
{
    auto result = std::make_shared<AnimationData>();
    result->name = old->name;
    result->duration = old->duration;
    result->skeleton = target;
    result->curves.resize(target->boneCount());
    for(BoneIndex i = 0; i < result->curves.size(); ++i)
    {
        auto found_index = old->skeleton->findBone(target->boneName(i));
        if(!found_index)
            continue;
        result->curves[i] = old->curves[*found_index];
    }

    return result;
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

std::string SimpleAnimation::name() const
{
    return _data->name;
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

CompositeAnimation::CompositeAnimation(std::shared_ptr<const Skeleton> ref)
    :CompositeAnimation(std::make_unique<NullAnimation>(ref))
{
}

CompositeAnimation::CompositeAnimation(std::unique_ptr<Animation> ref)
{
    if(!ref)
        throw std::invalid_argument("CompositeAnimation can't be built with null");
    
    _children.emplace_back(std::move(ref));
}

auto CompositeAnimation::addAnimation(std::unique_ptr<Animation> anim) -> size_t
{
    sanitize(anim);
    _children.emplace_back(std::move(anim));
    rebuild();
    return _children.size() - 1;
}

std::unique_ptr<Animation> CompositeAnimation::releaseAnimation(size_t index)
{
    auto old = std::move(_children[index]);
    _children.erase(_children.begin() + index);
    if(_children.empty())
        _children.emplace_back(std::make_unique<NullAnimation>(old->skeleton()));
    rebuild();
    return old;
}

std::unique_ptr<Animation> CompositeAnimation::swapAnimation(size_t index, std::unique_ptr<Animation> new_anim)
{
    sanitize(new_anim);
    swap(_children[index], new_anim);
    rebuild();
    return new_anim;
}

auto CompositeAnimation::count() const -> size_t
{
    return _children.size();
}

const Animation& CompositeAnimation::operator[](size_t i) const
{
    return *_children[i];
}

Animation& CompositeAnimation::operator[](size_t i)
{
    return const_cast<Animation&>(const_cast<const CompositeAnimation&>(*this)[i]);
}

std::shared_ptr<const Skeleton> CompositeAnimation::skeleton() const
{
    return _children.front()->skeleton();
}

void CompositeAnimation::sanitize(std::unique_ptr<Animation>& anim) const
{
    if(!anim)
        anim = std::make_unique<NullAnimation>(skeleton());
    
    if(anim->skeleton() != skeleton())
        throw std::invalid_argument("Can't mix skeletons in composite animation");
}

std::string AdditiveAnimation::name() const
{
    return "Add";
}

void AdditiveAnimation::reset(Seconds at)
{
    if(at.count())
    {
        (*this)[0].reset();
    }
    else
    {
        for(size_t i = 0; i < count(); ++i)
        {
            (*this)[i].reset(at);
        }
    }
}

void AdditiveAnimation::tick(Seconds delta)
{
    for(size_t i = 0; i < count(); ++i)
    {
        (*this)[i].tick(delta);
    }
}

Eigen::Quaternionf AdditiveAnimation::getBoneRot(BoneIndex index) const
{
    Eigen::Quaternionf rot = Eigen::Quaternionf::Identity();
    for(size_t i = 0; i < count(); ++i)
    {
        rot *= (*this)[i].getBoneRot(index);
    }
    return rot;
}

Seconds AdditiveAnimation::time() const
{
    return (*this)[0].time();
}

Seconds AdditiveAnimation::duration() const
{
    return (*this)[0].duration();
}

float BlendedAnimation::blendFactor() const
{
    return _blend_factor;
}

void BlendedAnimation::setBlendFactor(float f)
{
    auto old_targets = getTargets();
    _blend_factor = std::clamp<float>(f, 0, count() - 1);
    for(size_t target : getTargets())
    {
        if(std::ranges::find(old_targets, target) != old_targets.end()) continue;
        resync(target);
    }
}

std::string BlendedAnimation::name() const
{
    return "Blend";
}

void BlendedAnimation::reset(Seconds at)
{
    auto dur = duration();
    _normalized_timer = dur.count() ? at / dur : 0;
    if(float t = std::trunc(_normalized_timer)) _normalized_timer -= t;

    for(size_t i : getTargets())
    {
        resync(i);
    }
}

void BlendedAnimation::tick(Seconds delta)
{
    for(size_t i : getTargets())
    {
        (*this)[i].tick(delta * speedFactor(i));
    }
    if(auto dur = duration(); dur.count()) _normalized_timer += delta / dur;
    if(float t = std::trunc(_normalized_timer)) _normalized_timer -= t;
}

Eigen::Quaternionf BlendedAnimation::getBoneRot(BoneIndex index) const
{
    auto targets = getTargets();
    return (*this)[targets.front()].getBoneRot(index).slerp(localBlendFactor(), (*this)[targets.back()].getBoneRot(index));
}

Seconds BlendedAnimation::time() const
{
    return _normalized_timer * duration();
}

Seconds BlendedAnimation::duration() const
{
    auto targets = getTargets();
    return lerp((*this)[targets.front()].duration(), (*this)[targets.back()].duration(), localBlendFactor());
}

float BlendedAnimation::speedFactor(size_t i) const
{
    const Seconds from_duration = (*this)[i].duration();
    Seconds to_duration = duration();
    if(!from_duration.count() || !to_duration.count())
        return 1;

    return from_duration / to_duration;
}

float BlendedAnimation::localBlendFactor() const
{
    return _blend_factor - std::trunc(_blend_factor);
}

std::vector<size_t> BlendedAnimation::getTargets() const
{
    std::vector<size_t> result{static_cast<size_t>(std::trunc(_blend_factor))};

    if(_blend_factor != result.front())
        result.push_back(result.front() + 1);
        
    return result;
}

void BlendedAnimation::resync(size_t i)
{
    (*this)[i].reset(_normalized_timer * (*this)[i].duration());
}

void BlendedAnimation::rebuild()
{
    setBlendFactor(_blend_factor);
}
