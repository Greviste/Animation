#ifndef MODEL_H
#define MODEL_H

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <Eigen/Dense>
#include <QGLViewer/camera.h>
#include "Skeleton.h"
#include "SafeGl.h"
#include "Animation.h"


struct Vertex
{
    Eigen::Vector3f pos;
    Eigen::Vector3f normal;
    std::array<BoneIndex, 4> bones;
    std::array<float, 4> bone_weights;
};

struct Triangle
{
    std::array<unsigned, 3> indexes;
};

struct ModelData
{
    std::vector<Vertex> vertices;
    std::vector<Triangle> faces;
    std::shared_ptr<const Skeleton> skeleton;
};

class Model
{
public:
    Model(std::shared_ptr<const ModelData> data);

    void draw(const qglviewer::Camera& camera, const Animation* anim = nullptr);
    void setDualQuatPart(float part); //Part is 0-1

    const ModelData& data() const;
private:
    void buildModel();

    SafeGl::VertexArray _vao;
    SafeGl::Buffer _vertex_buffer;
    SafeGl::Buffer _index_buffer;
    SafeGl::Program _program;
    std::size_t _size = 0;

    float _dual_quat_part = 0;
    std::shared_ptr<const ModelData> _data;
};

#endif
