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

    void addWeightToVertex(Vertex& v, std::string bone, float w)
    {
        auto w_it = std::ranges::min_element(v.bone_weights);
        if(*w_it != 0) throw std::runtime_error("Non-zero weight found : make sure each vertex has at most 4 bones affecting it");
        *w_it = 0;
        *(v.bones.begin() + (w_it - v.bone_weights.begin())) = std::move(bone);
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

    ModelData loadModel(const FbxMesh& mesh)
    {
        double max = 0;
        int size = mesh.GetControlPointsCount();
        int faces = mesh.GetPolygonCount();
        const FbxVector4* vectors = mesh.GetControlPoints();

        ModelData result;
        result.vertices.reserve(size);
        result.faces.reserve(faces);
        std::vector<std::vector<unsigned>> vertex_instances(size);
        auto findOrBuild = [&](int polygon, int i)
        {
            int index = mesh.GetPolygonVertex(polygon, i);
            FbxVector4 normal; mesh.GetPolygonVertexNormal(polygon, i, normal);
            for(unsigned other_index : vertex_instances[index])
            {
                if(result.vertices[other_index].normal == normal) return other_index;
            }
            Vertex v{};
            for(int i = 0; i < 3; ++i) v.pos[i] = vectors[index][i];
            for(int i = 0; i < 3; ++i) v.normal[i] = normal[i];
            vertex_instances[index].push_back(result.vertices.size());
            result.vertices.push_back(std::move(v));
            return vertex_instances[index].back();
        };
        for(int i = 0; i < faces; ++i)
        {
            int ps = mesh.GetPolygonSize(i);
            unsigned first_i = findOrBuild(i, 0);
            unsigned previous_i = findOrBuild(i, 1);
            for(int j = 2; j < ps; ++j)
            {
                unsigned current_i = findOrBuild(i, j);
                result.faces.emplace_back(Triangle{{first_i, previous_i, current_i}});
                previous_i = current_i;
            }
        }

        if(mesh.GetDeformerCount(FbxSkin::eSkin) != 1) throw std::runtime_error("Model should have exactly one skin!");
        const FbxSkin& skin = *static_cast<FbxSkin*>(mesh.GetDeformer(0, FbxSkin::eSkin));
        int clusters = skin.GetClusterCount();
        for(int i = 0; i < clusters; ++i)
        {
            const FbxCluster& cluster = *skin.GetCluster(i);
            int weights = cluster.GetControlPointIndicesCount();
            std::string bone_name = cluster.GetLink()->GetName();
            for(int j = 0; j < weights; ++j)
            {
                for(int index : vertex_instances[cluster.GetControlPointIndices()[j]])
                {
                    addWeightToVertex(result.vertices[index], bone_name, cluster.GetControlPointWeights()[j]);
                }
            }
        }

        return result;
    }
}

ModelData loadFbx(const std::filesystem::path& file)
{
    auto u_manager = CreateUnique<FbxManager>();
    manager = u_manager.get();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    ios->SetBoolProp(IMP_FBX_MATERIAL, false);
    ios->SetBoolProp(IMP_FBX_TEXTURE, false);
    auto importer = CreateUnique<FbxImporter>(manager, "");
    if(!importer->Initialize(file.c_str(), -1, ios))
        throw std::runtime_error("Unable to open fbx");
    FbxScene* scene = FbxScene::Create(manager, "scene");
    importer->Import(scene);

    int n_geom = scene->GetGeometryCount();
    if(n_geom == 0)
        throw std::runtime_error("Scene should have at least one geometry!");

    ModelData model = loadModel(dynamic_cast<FbxMesh&>(*scene->GetGeometry(0)));
    for(int i = 1; i < n_geom; ++i)
    {
        ModelData other_model = loadModel(dynamic_cast<FbxMesh&>(*scene->GetGeometry(i)));
        int offset = model.vertices.size();
        model.vertices.reserve(model.vertices.size() + other_model.vertices.size());
        model.faces.reserve(model.faces.size() + other_model.faces.size());

        std::ranges::move(other_model.vertices, std::back_inserter(model.vertices));
        std::ranges::transform(other_model.faces, std::back_inserter(model.faces), [&](Triangle& tri) {
            for(auto& id : tri.indexes) id += offset;
            return tri;
        });
    }

    return model;
}
