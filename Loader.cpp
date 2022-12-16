#include "Loader.h"
#include <memory>
#include <numeric>
#include <algorithm>
#include "fbxsdk.h"


using fbxsdk::FbxManager, fbxsdk::FbxIOSettings, fbxsdk::FbxImporter, fbxsdk::FbxScene, fbxsdk::FbxMesh,
        fbxsdk::FbxGeometryElementNormal, fbxsdk::FbxVector4, fbxsdk::FbxSkin, fbxsdk::FbxCluster;

namespace
{
    FbxManager* manager;

    void addWeightToVertex(Vertex& v, BoneIndex bone, float w)
    {
        auto w_it = std::ranges::min_element(v.bone_weights);
        if(*w_it != 0) throw std::runtime_error("Non-zero weight found : make sure each vertex has at most 4 bones affecting it");
        *w_it = w;
        *(v.bones.begin() + (w_it - v.bone_weights.begin())) = bone;
    }

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

    bool operator==(const std::array<float, 3>& l, const FbxVector4& r)
    {
        for(int i = 0; i < 3; ++i) if(l[i] != r[i]) return false;
        return true;
    }

    auto setupUniqueManager()
    {
        auto u_manager = CreateUnique<FbxManager>();
        manager = u_manager.get();
        return u_manager;
    }

    FbxScene& importScene(const std::filesystem::path& file)
    {
        FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
        ios->SetBoolProp(IMP_FBX_MATERIAL, false);
        ios->SetBoolProp(IMP_FBX_TEXTURE, false);
        auto importer = CreateUnique<FbxImporter>(manager, "");
        if(!importer->Initialize(file.c_str(), -1, ios))
            throw std::runtime_error("Unable to open fbx");
        FbxScene* scene = FbxScene::Create(manager, "scene");
        importer->Import(scene);

        return *scene;
    }


    auto decodeMeshGeometry(const FbxMesh& mesh, LoadedData& load)
    {
        ModelData& data = get<ModelData>(load);

        int size = mesh.GetControlPointsCount();
        int faces = mesh.GetPolygonCount();
        data.vertices.reserve(data.vertices.size() + size);
        data.faces.reserve(data.faces.size() + faces);
        const FbxVector4* vectors = mesh.GetControlPoints();
        std::vector<std::vector<unsigned>> vertex_instances(size);

        auto getVertexInstance = [&](int polygon, int i)
        {
            int index = mesh.GetPolygonVertex(polygon, i);
            FbxVector4 normal; mesh.GetPolygonVertexNormal(polygon, i, normal);
            for(unsigned other_index : vertex_instances[index])
            {
                if(data.vertices[other_index].normal == normal) return other_index;
            }
            Vertex v{};
            for(int i = 0; i < 3; ++i) v.pos[i] = vectors[index][i];
            for(int i = 0; i < 3; ++i) v.normal[i] = normal[i];
            vertex_instances[index].push_back(data.vertices.size());
            data.vertices.push_back(std::move(v));
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
                data.faces.emplace_back(Triangle{{first_i, previous_i, current_i}});
                previous_i = current_i;
            }
        }

        return vertex_instances;
    }

    void decodeMeshSkinning(const FbxMesh& mesh, LoadedData& load, const std::vector<std::vector<unsigned>>& vertex_instances)
    {
        if(mesh.GetDeformerCount(FbxSkin::eSkin) != 1) throw std::runtime_error("Model should have exactly one skin!");

        ModelData& data = get<ModelData>(load);
        Skeleton& skeleton = get<Skeleton>(load);
        const FbxSkin& skin = *static_cast<FbxSkin*>(mesh.GetDeformer(0, FbxSkin::eSkin));
        int clusters = skin.GetClusterCount();
        for(int i = 0; i < clusters; ++i)
        {
            const FbxCluster& cluster = *skin.GetCluster(i);
            int weights = cluster.GetControlPointIndicesCount();
            BoneIndex bone = skeleton.boneFromName(cluster.GetLink()->GetName());
            for(int j = 0; j < weights; ++j)
            {
                for(unsigned index : vertex_instances[cluster.GetControlPointIndices()[j]])
                {
                    addWeightToVertex(data.vertices[index], bone, cluster.GetControlPointWeights()[j]);
                }
            }
        }
    }

    void decodeMesh(const FbxMesh& mesh, LoadedData& load)
    {
        auto vertex_instances = decodeMeshGeometry(mesh, load);
        decodeMeshSkinning(mesh, load, vertex_instances);
    }

    LoadedData decodeAllMeshes(FbxScene& scene)
    {
        int n_geom = scene.GetGeometryCount();
        if(n_geom == 0)
            throw std::runtime_error("Scene should have at least one geometry!");

        LoadedData result;
        for(int i = 0; i < n_geom; ++i)
        {
            decodeMesh(dynamic_cast<FbxMesh&>(*scene.GetGeometry(i)), result);
        }

        return result;
    }
}

LoadedData decodeFbx(const std::filesystem::path& file)
{
    auto u_manager = setupUniqueManager();
    FbxScene& scene = importScene(file);

    LoadedData result = decodeAllMeshes(scene);

    return result;
}
