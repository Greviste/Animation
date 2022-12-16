#ifndef MODEL_H
#define MODEL_H

#include <array>
#include <vector>
#include <string>
#include "Skeleton.h"


struct Vertex
{
    std::array<float, 3> pos;
    std::array<float, 3> normal;
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
};

#endif
