#include "Loader.h"
#include <memory>
#include <numeric>
#include <algorithm>
#include "fbxsdk.h"

#include <iostream>
using fbxsdk::FbxManager, fbxsdk::FbxIOSettings, fbxsdk::FbxImporter, fbxsdk::FbxScene, fbxsdk::FbxMesh,
        fbxsdk::FbxGeometryElementNormal, fbxsdk::FbxVector4, fbxsdk::FbxSkin, fbxsdk::FbxCluster,
        fbxsdk::FbxAnimStack, fbxsdk::FbxAnimLayer, fbxsdk::FbxNode, fbxsdk::FbxAnimCurve,
        fbxsdk::FbxTime;

namespace
{
template<typename T, typename... Args>
auto CreateUnique(Args&&... args)
{
    struct Deleter
    {
        void operator()(T* ptr)
        {
            ptr->Destroy();
        }
    };
    return std::unique_ptr<T, Deleter>{T::Create(std::forward<Args>(args)...)};
}

bool operator==(const Eigen::Vector3f& l, const FbxVector4& r)
{
    for(int i = 0; i < 3; ++i) if(l[i] != r[i]) return false;
    return true;
}

Eigen::Vector3f toVector(fbxsdk::FbxDouble3 v)
{
    return {v[0], v[1], v[2]};
}

float deg2rad(float x)
{
    return x / 180 * M_PIf;
}

Eigen::Quaternionf toRotation(fbxsdk::FbxDouble3 r)
{
    return Eigen::AngleAxisf{deg2rad(r[2]), Eigen::Vector3f::UnitZ()}
        * Eigen::AngleAxisf{deg2rad(r[1]), Eigen::Vector3f::UnitY()}
        * Eigen::AngleAxisf{deg2rad(r[0]), Eigen::Vector3f::UnitX()};
}

Eigen::Quaternionf toRotation(FbxNode& node, fbxsdk::FbxDouble3 r)
{
    return toRotation(node.PreRotation.Get()) * toRotation(r) * toRotation(node.PostRotation.Get());
}

struct Loader
{
    std::shared_ptr<ModelData> data;
    std::vector<std::shared_ptr<AnimationData>> anims;
    std::shared_ptr<Skeleton> skeleton;

    void load(const std::filesystem::path& file);

private:
    void addWeightToVertex(Vertex& v, BoneIndex bone, float w)
    {
        auto w_it = std::ranges::min_element(v.bone_weights);
        if(*w_it != 0)
            throw std::runtime_error("Non-zero weight found : make sure each vertex has at most 4 bones affecting it");
        *w_it = w;
        *(v.bones.begin() + (w_it - v.bone_weights.begin())) = bone;
    }

    auto setupUniqueManager()
    {
        auto u_manager = CreateUnique<FbxManager>();
        manager = u_manager.get();
        return u_manager;
    }

    void exportAsAscii(const std::filesystem::path& dest, FbxScene& scene)
    {
        auto plugin_registry = manager->GetIOPluginRegistry();
        int n = plugin_registry->GetWriterFormatCount();
        for ( int i = 0; i < n; i++ )
        {
            if (!plugin_registry->WriterIsFBX(i)) continue;
            std::string_view description = plugin_registry->GetWriterFormatDescription(i);
            if (description != "FBX ascii (*.fbx)") continue;

            auto ios = manager->GetIOSettings();
            ios->SetBoolProp(EXP_ASCIIFBX, true);
            auto exporter = CreateUnique<fbxsdk::FbxExporter>(manager, "");
            exporter->Initialize(dest.c_str(), i, ios);
            exporter->Export(&scene);
            break;
        }
    }

    FbxScene& importScene(const std::filesystem::path& file)
    {
        FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
        manager->SetIOSettings(ios);
        ios->SetBoolProp(IMP_FBX_MATERIAL, false);
        ios->SetBoolProp(IMP_FBX_TEXTURE, false);
        auto importer = CreateUnique<FbxImporter>(manager, "");
        if(!importer->Initialize(file.c_str(), -1, ios))
            throw std::runtime_error("Unable to open fbx");
        FbxScene* scene = FbxScene::Create(manager, "scene");
        importer->Import(scene);

        return *scene;
    }

    void decodeMeshGeometry(const FbxMesh& mesh)
    {
        int size = mesh.GetControlPointsCount();
        int faces = mesh.GetPolygonCount();
        data->vertices.reserve(data->vertices.size() + size);
        data->faces.reserve(data->faces.size() + faces);
        const FbxVector4* vectors = mesh.GetControlPoints();
        vertex_instances.clear();
        vertex_instances.resize(size);

        auto getVertexInstance = [&](int polygon, int i)
        {
            int index = mesh.GetPolygonVertex(polygon, i);
            FbxVector4 normal; mesh.GetPolygonVertexNormal(polygon, i, normal);
            for(unsigned other_index : vertex_instances[index])
            {
                if(data->vertices[other_index].normal == normal) return other_index;
            }
            Vertex v{};
            for(int i = 0; i < 3; ++i) v.pos[i] = vectors[index][i];
            for(int i = 0; i < 3; ++i) v.normal[i] = normal[i];
            vertex_instances[index].push_back(data->vertices.size());
            data->vertices.push_back(std::move(v));
            return vertex_instances[index].back();
        };

        for(int i = 0; i < faces; ++i)
        {
            int polygon_size = mesh.GetPolygonSize(i);
            unsigned first_i = getVertexInstance(i, 0);
            unsigned previous_i = getVertexInstance(i, 1);
            for(int j = 2; j < polygon_size; ++j)
            {
                unsigned current_i = getVertexInstance(i, j);
                data->faces.emplace_back(Triangle{{first_i, previous_i, current_i}});
                previous_i = current_i;
            }
        }
    }

    BoneIndex registerBone(FbxNode& node)
    {
        auto it = std::ranges::find(armature, &node);
        BoneIndex index = it - armature.begin();
        if(it == armature.end())
        {
            armature.emplace_back(&node);
        }

        return index;
    }

    void decodeMeshSkinning(const FbxMesh& mesh)
    {
        if(mesh.GetDeformerCount(FbxSkin::eSkin) != 1) throw std::runtime_error("Model should have exactly one skin!");

        const FbxSkin& skin = *static_cast<FbxSkin*>(mesh.GetDeformer(0, FbxSkin::eSkin));
        int clusters = skin.GetClusterCount();
        for(int i = 0; i < clusters; ++i)
        {
            const FbxCluster& cluster = *skin.GetCluster(i);
            int weights = cluster.GetControlPointIndicesCount();
            BoneIndex bone = registerBone(const_cast<FbxNode&>(*cluster.GetLink()));
            for(int j = 0; j < weights; ++j)
            {
                for(unsigned index : vertex_instances[cluster.GetControlPointIndices()[j]])
                {
                    addWeightToVertex(data->vertices[index], bone, cluster.GetControlPointWeights()[j]);
                }
            }
        }
    }

    void decodeMesh(const FbxMesh& mesh)
    {
        decodeMeshGeometry(mesh);
        decodeMeshSkinning(mesh);
    }

    void decodeAllMeshes(FbxScene& scene)
    {
        int n_geom = scene.GetGeometryCount();
        if(n_geom == 0)
            throw std::runtime_error("Scene should have at least one geometry!");

        for(int i = 0; i < n_geom; ++i)
        {
            decodeMesh(dynamic_cast<FbxMesh&>(*scene.GetGeometry(i)));
        }
    }

    void buildSkeletonBranch(BoneIndex root, std::optional<BoneIndex> parent, std::vector<BoneIndex>& remap)
    {
        FbxNode& node = *armature[root];
        remap[root] = skeleton->addBone(node.GetName(),
            {toVector(node.LclTranslation.Get()), toRotation(node, node.LclRotation.Get()), toVector(node.LclScaling.Get())}, parent);
        int n_child = node.GetChildCount();
        for(int i = 0; i < n_child; ++i)
        {
            auto it = std::ranges::find(armature, node.GetChild(i));
            if(it == armature.end()) continue;
            BoneIndex child = it - armature.begin();
            buildSkeletonBranch(child, root, remap);
        }
    }

    void remapVertexBones(const std::vector<BoneIndex>& remap)
    {
        for(Vertex& v : data->vertices)
        {
            for(BoneIndex& b : v.bones)
            {
                b = remap[b];
            }
        }
    }

    void reorderArmature(const std::vector<BoneIndex>& remap)
    {
        //Could probably be done inplace if I was smarter
        std::vector<FbxNode*> remapped(armature.size());
        for(std::size_t i = 0; i < armature.size(); ++i)
        {
            remapped[remap[i]] = armature[i];
        }

        armature = std::move(remapped);
    }

    std::vector<BoneIndex> buildSkeleton()
    {
        std::vector<BoneIndex> remap(armature.size());
        std::vector<BoneIndex> roots;
        for(std::size_t i = 0; i < armature.size(); ++i)
        {
            if(std::ranges::find(armature, armature[i]->GetParent()) == armature.end())
            {
                roots.push_back(i);
            }
        }
        if(roots.size() != 1)
        {
            throw std::runtime_error("More than one root");
        }
        BoneIndex root = roots.front();
        buildSkeletonBranch(root, std::nullopt, remap);

        return remap;
    }

    Seconds getAnimationDuration(FbxAnimLayer& layer)
    {
        const char* const components[] = {FBXSDK_CURVENODE_COMPONENT_X, FBXSDK_CURVENODE_COMPONENT_Y, FBXSDK_CURVENODE_COMPONENT_Z};

        FbxTime duration;
        for(std::size_t i = 0; i < armature.size(); ++i) for(const char* component : components)
        {
            FbxAnimCurve* curve = armature[i]->LclRotation.GetCurve(&layer, component);
            if(!curve) continue;
            FbxTime last = curve->KeyGetTime(curve->KeyGetCount() - 1);
            if(last > duration) duration = last;
        }
        return Seconds{duration.GetSecondDouble()};
    }

    AnimationData decodeAnimation(FbxAnimLayer& layer)
    {
        const char* const components[] = {FBXSDK_CURVENODE_COMPONENT_X, FBXSDK_CURVENODE_COMPONENT_Y, FBXSDK_CURVENODE_COMPONENT_Z};

        AnimationData anim;
        anim.name = layer.GetName();
        anim.skeleton = skeleton;
        anim.curves.resize(armature.size());
        anim.duration = getAnimationDuration(layer);
        for(BoneIndex i = 0; i < armature.size(); ++i)
        {
            FbxAnimCurve* curves[3];
            std::ranges::transform(components, curves, [&](auto component) { return armature[i]->LclRotation.GetCurve(&layer, component); });
            auto extract = [&](Seconds t)
            {
                FbxDouble3 euler{};
                FbxTime time;
                time.SetSecondDouble(t.count());
                for(int i = 0; i < 3; ++i)
                {
                    if(curves[i]) euler[i] = curves[i]->Evaluate(time);
                }
                anim.curves[i].keyframes.emplace_back(t, skeleton->boneTransform(i).rotation.conjugate() * toRotation(*armature[i], euler));
            };
            Frames frame{};
            do
                extract(frame);
            while(++frame < anim.duration);
            if(frame != anim.duration)
                extract(anim.duration);
        }

        return anim;
    }

    void decodeAnimations(const FbxScene& scene)
    {
        if(scene.GetSrcObjectCount<FbxAnimStack>() == 0)
            throw std::runtime_error("No animation");
        
        FbxAnimStack& anim_stack = *scene.GetSrcObject<FbxAnimStack>();
        int n = anim_stack.GetMemberCount<FbxAnimLayer>();
        if(!n)
            throw std::runtime_error("No animation");
        
        anims.resize(n);
        for(int i = 0; i < n; ++i)
        {
            FbxAnimLayer& layer = *anim_stack.GetMember<FbxAnimLayer>(i);
            anims[i] = std::make_shared<AnimationData>(decodeAnimation(layer));
        }
    }

    FbxManager* manager;
    std::vector<std::vector<unsigned>> vertex_instances;
    std::vector<FbxNode*> armature;
};

void Loader::load(const std::filesystem::path& file)
{
    data = std::make_shared<ModelData>();
    skeleton = std::make_shared<Skeleton>();

    auto u_manager = setupUniqueManager();
    FbxScene& scene = importScene(file);

    decodeAllMeshes(scene);
    auto remap = buildSkeleton();
    remapVertexBones(remap);
    reorderArmature(remap);
    data->skeleton = skeleton;
    decodeAnimations(scene);
}
}

LoadedData decodeFbx(const std::filesystem::path& file)
{
    Loader loader;
    loader.load(file);

    LoadedData result;
    get<0>(result) = std::move(loader.data);
    auto& anims = get<1>(result);
    anims.resize(loader.anims.size());
    std::ranges::move(loader.anims, anims.begin());

    return result;
}
